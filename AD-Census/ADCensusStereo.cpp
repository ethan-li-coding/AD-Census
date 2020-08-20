/* -*-c++-*- AD-Census - Copyright (C) 2020.
* Author	: Ethan Li <ethan.li.whu@gmail.com>
*			  https://github.com/ethan-li-coding/AD-Census
* Describe	: implement of ad-census stereo class
*/
#include "ADCensusStereo.h"
#include "adcensus_util.h"
#include <algorithm>

#include "scanline_optimizer.h"

ADCensusStereo::ADCensusStereo(): width_(0), height_(0), img_left_(nullptr), img_right_(nullptr),
                                  gray_left_(nullptr), gray_right_(nullptr),
                                  census_left_(nullptr), census_right_(nullptr),
                                  cost_init_(nullptr), cost_aggr_(nullptr), 
                                  disp_left_(nullptr), disp_right_(nullptr),
                                  is_initialized_(false) { }

ADCensusStereo::~ADCensusStereo()
{
	Release();
	is_initialized_ = false;
}

bool ADCensusStereo::Initialize(const sint32& width, const sint32& height, const ADCensusOption& option)
{
	// ··· 赋值

	// 影像尺寸
	width_ = width;
	height_ = height;
	// 算法参数
	option_ = option;

	if (width <= 0 || height <= 0) {
		return false;
	}

	//··· 开辟内存空间
	const sint32 img_size = width * height;
	const sint32 disp_range = option.max_disparity - option.min_disparity;
	if (disp_range <= 0) {
		return false;
	}

	// 灰度数据
	gray_left_ = new uint8[img_size];
	gray_right_ = new uint8[img_size];
	// census数据（左右影像）
	census_left_ = new uint64[img_size]();
	census_right_ = new uint64[img_size]();
	// 代价数据
	cost_init_ = new float32[img_size*disp_range];
	cost_aggr_ = new float32[img_size*disp_range];

	// 视差图
	disp_left_ = new float32[img_size];
	disp_right_ = new float32[img_size];
	
	// 初始化代价计算器
	if(!aggregator_.Initialize(width_, height_)) {
		return false;
	}

	is_initialized_ = gray_left_ && gray_right_ && cost_init_ && cost_aggr_  && disp_left_ && disp_right_;

	return is_initialized_;
}

bool ADCensusStereo::Match(const uint8* img_left, const uint8* img_right, float32* disp_left)
{
	if (!is_initialized_) {
		return false;
	}
	if (img_left == nullptr || img_right == nullptr) {
		return false;
	}

	img_left_ = img_left;
	img_right_ = img_right;

	// 计算灰度图
	ComputeGray();

	// Census变换
	CensusTransform();

	// 代价计算
	ComputeCost();

	// 代价聚合
	CostAggregation();

	// 扫描线优化
	ScanlineOptimize();

	// 计算左右视图视差
	ComputeDisparity();
	ComputeDisparityRight();

	// 多步骤视差优化
	MultiStepRefine();

	// 输出视差图
	memcpy(disp_left, disp_left_, height_ * width_ * sizeof(float32));

	return true;
}

bool ADCensusStereo::Reset(const uint32& width, const uint32& height, const ADCensusOption& option)
{
	// 释放内存
	Release();

	// 重置初始化标记
	is_initialized_ = false;

	// 初始化
	return Initialize(width, height, option);
}

void ADCensusStereo::ComputeGray() const
{
	const sint32 width = width_;
	const sint32 height = height_;
	if (width <= 0 || height <= 0 ||
		img_left_ == nullptr || img_right_ == nullptr ||
		gray_left_ == nullptr || gray_right_ == nullptr) {
		return;
	}

	// 彩色转灰度
	for (sint32 n = 0; n < 2; n++) {
		auto* color = (n == 0) ? img_left_ : img_right_;
		auto* gray = (n == 0) ? gray_left_ : gray_right_;
		for (sint32 i = 0; i < height; i++) {
			for (sint32 j = 0; j < width; j++) {
				const auto b = color[i * width * 3 + 3 * j];
				const auto g = color[i * width * 3 + 3 * j + 1];
				const auto r = color[i * width * 3 + 3 * j + 2];
				gray[i * width + j] = uint8(r * 0.299 + g * 0.587 + b * 0.114);
			}
		}
	}
}

void ADCensusStereo::CensusTransform() const
{
	const sint32 width = width_;
	const sint32 height = height_;
	if (width <= 0 || height <= 0 ||
		gray_left_ == nullptr || gray_right_ == nullptr ||
		census_left_ == nullptr || census_right_ == nullptr) {
		return;
	}
	// 左右影像census变换
	adcensus_util::census_transform_9x7(gray_left_, census_left_, width, height);
	adcensus_util::census_transform_9x7(gray_right_, census_right_, width, height);
}

void ADCensusStereo::ComputeCost() const
{
	const sint32& min_disparity = option_.min_disparity;
	const sint32& max_disparity = option_.max_disparity;
	const sint32 width = width_;
	const sint32 height = height_;
	const sint32 disp_range = max_disparity - min_disparity;
	if (disp_range <= 0 || width <= 0 || height <= 0 ||
		img_left_ == nullptr || img_right_ == nullptr || 
		census_left_ == nullptr || census_right_ == nullptr) {
		return;
	}

	// 预设参数
	const auto lambda_ad = option_.lambda_ad;
	const auto lambda_census = option_.lambda_census;

	// 计算代价
	for (sint32 i = 0; i < height; i++) {
		for (sint32 j = 0; j < width; j++) {
			const auto bl = img_left_[i*width * 3 + 3*j];
			const auto gl = img_left_[i*width * 3 + 3*j + 1];
			const auto rl = img_left_[i*width * 3 + 3*j + 2];
			const auto& census_val_l = census_left_[i * width_ + j];
			// 逐视差计算代价值
			for (sint32 d = min_disparity; d < max_disparity; d++) {
				auto& cost = cost_init_[i * width_ * disp_range + j * disp_range + (d - min_disparity)];
				const sint32 jr = j - d;
				if (jr < 0 || jr >= width_) {
					cost = 1.0f;
					continue;
				}

				// ad代价
				const auto br = img_right_[i*width * 3 + 3 * jr];
				const auto gr = img_right_[i*width * 3 + 3 * jr + 1];
				const auto rr = img_right_[i*width * 3 + 3 * jr + 2];
				const float32 cost_ad = (abs(bl - br) + abs(gl - gr) + abs(rl - rr)) / 3.0f;

				// census代价
				const auto& census_val_r = census_right_[i * width_ + jr];
				const auto cost_census = static_cast<float32>(adcensus_util::Hamming64(census_val_l, census_val_r));

				// ad-census代价
				cost = 1 - exp(-cost_ad / lambda_ad) + 1 - exp(-cost_census / lambda_census);
			}
		}
	}
}

void ADCensusStereo::CostAggregation()
{
	// 设置聚合器数据
	aggregator_.SetData(img_left_, img_right_, cost_init_, cost_aggr_);
	// 设置聚合器参数
	aggregator_.SetParams(option_.cross_L1, option_.cross_L2, option_.cross_t1, option_.cross_t2, option_.min_disparity, option_.max_disparity);
	// 聚合器计算聚合臂
	aggregator_.BuildArms();
	// 代价聚合
	aggregator_.Aggregate(4);
}

void ADCensusStereo::ScanlineOptimize() const
{
	ScanlineOptimizer scan_line;
	// 设置优化器数据
	scan_line.SetData(img_left_, img_right_, cost_init_, cost_aggr_);
	// 设置优化器参数
	scan_line.SetParam(width_, height_, option_.min_disparity, option_.max_disparity, option_.so_p1, option_.so_p2, option_.so_tso);
	// 扫描线优化
	scan_line.Optimize();
}

void ADCensusStereo::MultiStepRefine()
{
	if (option_.is_check_lr) {
		OutlierDetection();
	}
}

void ADCensusStereo::ComputeDisparity() const
{
	const sint32& min_disparity = option_.min_disparity;
	const sint32& max_disparity = option_.max_disparity;
	const sint32 disp_range = max_disparity - min_disparity;
	if (disp_range <= 0) {
		return;
	}

	// 左影像视差图
	const auto disparity = disp_left_;
	// 左影像聚合代价数组
	const auto cost_ptr = cost_aggr_;

	const sint32 width = width_;
	const sint32 height = height_;

	// 为了加快读取效率，把单个像素的所有代价值存储到局部数组里
	std::vector<float32> cost_local(disp_range);

	// ---逐像素计算最优视差
	for (sint32 i = 0; i < height; i++) {
		for (sint32 j = 0; j < width; j++) {
			float32 min_cost = Large_Float;
			sint32 best_disparity = 0;

			// ---遍历视差范围内的所有代价值，输出最小代价值及对应的视差值
			for (sint32 d = min_disparity; d < max_disparity; d++) {
				const sint32 d_idx = d - min_disparity;
				const auto& cost = cost_local[d_idx] = cost_ptr[i * width * disp_range + j * disp_range + d_idx];
				if (min_cost > cost) {
					min_cost = cost;
					best_disparity = d;
				}
			}
			// ---子像素拟合
			if (best_disparity == min_disparity || best_disparity == max_disparity - 1) {
				disparity[i * width + j] = Invalid_Float;
				continue;
			}
			// 最优视差前一个视差的代价值cost_1，后一个视差的代价值cost_2
			const sint32 idx_1 = best_disparity - 1 - min_disparity;
			const sint32 idx_2 = best_disparity + 1 - min_disparity;
			const float32 cost_1 = cost_local[idx_1];
			const float32 cost_2 = cost_local[idx_2];
			// 解一元二次曲线极值
			const float32 denom = cost_1 + cost_2 - 2 * min_cost;
			if (denom != 0.0f) {
				disparity[i * width + j] = static_cast<float32>(best_disparity) + (cost_1 - cost_2) / (denom * 2.0f);
			}
			else {
				disparity[i * width + j] = static_cast<float32>(best_disparity);
			}
		}
	}
}

void ADCensusStereo::ComputeDisparityRight() const
{
	const sint32& min_disparity = option_.min_disparity;
	const sint32& max_disparity = option_.max_disparity;
	const sint32 disp_range = max_disparity - min_disparity;
	if (disp_range <= 0) {
		return;
	}

	// 右影像视差图
	const auto disparity = disp_right_;
	// 左影像聚合代价数组
	const auto cost_ptr = cost_aggr_;

	const sint32 width = width_;
	const sint32 height = height_;

	// 为了加快读取效率，把单个像素的所有代价值存储到局部数组里
	std::vector<float32> cost_local(disp_range);

	// ---逐像素计算最优视差
	// 通过左影像的代价，获取右影像的代价
	// 右cost(xr,yr,d) = 左cost(xr+d,yl,d)
	for (sint32 i = 0; i < height; i++) {
		for (sint32 j = 0; j < width; j++) {
			float32 min_cost = Large_Float;
			sint32 best_disparity = 0;

			// ---统计候选视差下的代价值
			for (sint32 d = min_disparity; d < max_disparity; d++) {
				const sint32 d_idx = d - min_disparity;
				const sint32 col_left = j + d;
				if (col_left >= 0 && col_left < width) {
					const auto& cost = cost_local[d_idx] = cost_ptr[i * width * disp_range + col_left * disp_range + d_idx];
					if (min_cost > cost) {
						min_cost = cost;
						best_disparity = d;
					}
				}
				else {
					cost_local[d_idx] = Large_Float;
				}
			}

			// ---子像素拟合
			if (best_disparity == min_disparity || best_disparity == max_disparity - 1) {
				disparity[i * width + j] = Invalid_Float;
				continue;
			}

			// 最优视差前一个视差的代价值cost_1，后一个视差的代价值cost_2
			const sint32 idx_1 = best_disparity - 1 - min_disparity;
			const sint32 idx_2 = best_disparity + 1 - min_disparity;
			const float32 cost_1 = cost_local[idx_1];
			const float32 cost_2 = cost_local[idx_2];
			// 解一元二次曲线极值
			const float32 denom = cost_1 + cost_2 - 2 * min_cost;
			if (denom != 0.0f) {
				disparity[i * width + j] = static_cast<float32>(best_disparity) + (cost_1 - cost_2) / (denom * 2.0f);
			}
			else {
				disparity[i * width + j] = static_cast<float32>(best_disparity);
			}
		}
	}
}

void ADCensusStereo::Release()
{
	SAFE_DELETE(gray_left_);
	SAFE_DELETE(gray_right_);
	SAFE_DELETE(census_left_);
	SAFE_DELETE(census_right_);
	SAFE_DELETE(cost_init_);
	SAFE_DELETE(cost_aggr_);
	SAFE_DELETE(disp_left_);
	SAFE_DELETE(disp_right_);
}

void ADCensusStereo::OutlierDetection()
{
	const sint32 width = width_;
	const sint32 height = height_;

	const float32& threshold = option_.lrcheck_thres;

	// 遮挡区像素和误匹配区像素
	auto& occlusions = occlusions_;
	auto& mismatches = mismatches_;
	occlusions.clear();
	mismatches.clear();

	// ---左右一致性检查
	for (sint32 i = 0; i < height; i++) {
		for (sint32 j = 0; j < width; j++) {

			// 左影像视差值
			auto& disp = disp_left_[i * width + j];

			if (disp == Invalid_Float) {
				mismatches.emplace_back(i, j);
				continue;
			}

			// 根据视差值找到右影像上对应的同名像素
			const auto col_right = static_cast<sint32>(j - disp + 0.5);

			if (col_right >= 0 && col_right < width) {

				// 右影像上同名像素的视差值
				const auto& disp_r = disp_right_[i * width + col_right];

				// 判断两个视差值是否一致（差值在阈值内）
				if (abs(disp - disp_r) > threshold) {
					// 区分遮挡区和误匹配区
					// 通过右影像视差算出在左影像的匹配像素，并获取视差disp_rl
					// if(disp_rl > disp) 
					//		pixel in occlusions
					// else 
					//		pixel in mismatches
					const sint32 col_rl = static_cast<sint32>(col_right + disp_r + 0.5);
					if (col_rl > 0 && col_rl < width) {
						const auto& disp_l = disp_left_[i*width + col_rl];
						if (disp_l > disp) {
							occlusions.emplace_back(i, j);
						}
						else {
							mismatches.emplace_back(i, j);
						}
					}
					else {
						mismatches.emplace_back(i, j);
					}

					// 让视差值无效
					disp = Invalid_Float;
				}
			}
			else {
				// 通过视差值在右影像上找不到同名像素（超出影像范围）
				disp = Invalid_Float;
				mismatches.emplace_back(i, j);
			}
		}
	}
}

void ADCensusStereo::IterativeRegionVoting()
{
	
}

void ADCensusStereo::ProperInterpolation()
{
	
}

void ADCensusStereo::DepthDiscontinuityAdjustment()
{
	
}

void ADCensusStereo::SubpixelEnhancement()
{
	
}

