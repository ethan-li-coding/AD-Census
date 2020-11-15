/* -*-c++-*- AD-Census - Copyright (C) 2020.
* Author	: Yingsong Li(Ethan Li) <ethan.li.whu@gmail.com>
* https://github.com/ethan-li-coding/AD-Census
* Describe	: implement of class MultiStepRefiner
*/

#include "multistep_refiner.h"
#include "adcensus_util.h"

MultiStepRefiner::MultiStepRefiner(): width_(0), height_(0), img_left_(nullptr), cost_(nullptr),
                                      cross_arms_(nullptr),
                                      disp_left_(nullptr), disp_right_(nullptr),
                                      min_disparity_(0), max_disparity_(0),
                                      irv_ts_(0), irv_th_(0), lrcheck_thres_(0),
                                      do_lr_check_(false), do_region_voting_(false),
                                      do_interpolating_(false), do_discontinuity_adjustment_(false) { }

MultiStepRefiner::~MultiStepRefiner()
{
}

bool MultiStepRefiner::Initialize(const sint32& width, const sint32& height)
{
	width_ = width;
	height_ = height;
	if (width_ <= 0 || height_ <= 0) {
		return false;
	}

	// 初始化边缘数据
	vec_edge_left_.clear();
	vec_edge_left_.resize(width*height);
	
	return true;
}

void MultiStepRefiner::SetData(const uint8* img_left, float32* cost,const CrossArm* cross_arms, float32* disp_left, float32* disp_right)
{
	img_left_ = img_left;
	cost_ = cost; 
	cross_arms_ = cross_arms;
	disp_left_ = disp_left;
	disp_right_= disp_right;
}

void MultiStepRefiner::SetParam(const sint32& min_disparity, const sint32& max_disparity, const sint32& irv_ts, const float32& irv_th, const float32& lrcheck_thres,
								const bool& do_lr_check, const bool& do_region_voting, const bool& do_interpolating, const bool& do_discontinuity_adjustment)
{
	min_disparity_ = min_disparity;
	max_disparity_ = max_disparity;
	irv_ts_ = irv_ts;
	irv_th_ = irv_th;
	lrcheck_thres_ = lrcheck_thres;
	do_lr_check_ = do_lr_check;
	do_region_voting_ = do_region_voting;
	do_interpolating_ = do_interpolating;
	do_discontinuity_adjustment_ = do_discontinuity_adjustment;
}

void MultiStepRefiner::Refine()
{
	if (width_ <= 0 || height_ <= 0 ||
		disp_left_ == nullptr || disp_right_ == nullptr ||
		cost_ == nullptr || cross_arms_ == nullptr) {
		return;
	}

	// step1: outlier detection
	if (do_lr_check_) {
		OutlierDetection();
	}
	// step2: iterative region voting
	if (do_region_voting_) {
		IterativeRegionVoting();
	}
	// step3: proper interpolation
	if (do_interpolating_) {
		ProperInterpolation();
	}
	// step4: discontinuities adjustment
	if (do_discontinuity_adjustment_) {
		DepthDiscontinuityAdjustment();
	}

	// median filter
	adcensus_util::MedianFilter(disp_left_, disp_left_, width_, height_, 3);
}


void MultiStepRefiner::OutlierDetection()
{
	const sint32 width = width_;
	const sint32 height = height_;

	const float32& threshold = lrcheck_thres_;

	// 遮挡区像素和误匹配区像素
	auto& occlusions = occlusions_;
	auto& mismatches = mismatches_;
	occlusions.clear();
	mismatches.clear();

	// ---左右一致性检查
	for (sint32 y = 0; y < height; y++) {
		for (sint32 x = 0; x < width; x++) {
			// 左影像视差值
			auto& disp = disp_left_[y * width + x];
			if (disp == Invalid_Float) {
				mismatches.emplace_back(x, y);
				continue;
			}

			// 根据视差值找到右影像上对应的同名像素
			const auto col_right = lround(x - disp);
			if (col_right >= 0 && col_right < width) {
				// 右影像上同名像素的视差值
				const auto& disp_r = disp_right_[y * width + col_right];
				// 判断两个视差值是否一致（差值在阈值内）
				if (abs(disp - disp_r) > threshold) {
					// 区分遮挡区和误匹配区
					// 通过右影像视差算出在左影像的匹配像素，并获取视差disp_rl
					// if(disp_rl > disp) 
					//		pixel in occlusions
					// else 
					//		pixel in mismatches
					const sint32 col_rl = lround(col_right + disp_r);
					if (col_rl > 0 && col_rl < width) {
						const auto& disp_l = disp_left_[y * width + col_rl];
						if (disp_l > disp) {
							occlusions.emplace_back(x, y);
						}
						else {
							mismatches.emplace_back(x, y);
						}
					}
					else {
						mismatches.emplace_back(x, y);
					}

					// 让视差值无效
					disp = Invalid_Float;
				}
			}
			else {
				// 通过视差值在右影像上找不到同名像素（超出影像范围）
				disp = Invalid_Float;
				mismatches.emplace_back(x, y);
			}
		}
	}
}

void MultiStepRefiner::IterativeRegionVoting()
{
	const sint32 width = width_;

	const auto disp_range = max_disparity_ - min_disparity_;
	if(disp_range <= 0) {
		return;
	}
	const auto arms = cross_arms_;

	// 直方图
	vector<sint32> histogram(disp_range,0);

	// 迭代5次
	const sint32 num_iters = 5;
	
	for (sint32 it = 0; it < num_iters; it++) {
		for (sint32 k = 0; k < 2; k++) {
			auto& trg_pixels = (k == 0) ? mismatches_ : occlusions_;
			for (auto& pix : trg_pixels) {
				const sint32& x = pix.first;
				const sint32& y = pix.second;
				auto& disp = disp_left_[y * width + x];
				if(disp != Invalid_Float) {
					continue;
				}

				// init histogram
				memset(&histogram[0], 0, disp_range * sizeof(sint32));

				// 计算支持区的视差直方图
				// 获取arm
				auto& arm = arms[y * width + x];
				// 遍历支持区像素视差，统计直方图
				for (sint32 t = -arm.top; t <= arm.bottom; t++) {
					const sint32& yt = y + t;
					auto& arm2 = arms[yt * width_ + x];
					for (sint32 s = -arm2.left; s <= arm2.right; s++) {
						const auto& d = disp_left_[yt * width + x + s];
						if (d != Invalid_Float) {
							const auto di = lround(d);
							histogram[di - min_disparity_]++;
						}
					}
				}
				// 计算直方图峰值对应的视差
				sint32 best_disp = 0, count = 0;
				sint32 max_ht = 0;
				for (sint32 d = 0; d < disp_range; d++) {
					const auto& h = histogram[d];
					if (max_ht < h) {
						max_ht = h;
						best_disp = d;
					}
					count += h;
				}

				if (max_ht > 0) {
					if (count > irv_ts_ && max_ht * 1.0f / count > irv_th_) {
						disp = best_disp + min_disparity_;
					}
				}
			}
			// 删除已填充像素
			for (auto it = trg_pixels.begin(); it != trg_pixels.end();) {
				const sint32 x = it->first;
				const sint32 y = it->second;
				if(disp_left_[y * width + x]!=Invalid_Float) {
					it = trg_pixels.erase(it);
				}
				else { ++it; }
			}
		}
	}
}

void MultiStepRefiner::ProperInterpolation()
{
	const sint32 width = width_;
	const sint32 height = height_;

	const float32 pi = 3.1415926f;
	// 最大搜索行程，没有必要搜索过远的像素
	const sint32 max_search_length = std::max(abs(max_disparity_), abs(min_disparity_));

	std::vector<pair<sint32, float32>> disp_collects;
	for (sint32 k = 0; k < 2; k++) {
		auto& trg_pixels = (k == 0) ? mismatches_ : occlusions_;
		if (trg_pixels.empty()) {
			continue;
		}
		std::vector<float32> fill_disps(trg_pixels.size());

		// 遍历待处理像素
		for (auto n = 0u; n < trg_pixels.size(); n++) {
			auto& pix = trg_pixels[n];
			const sint32 x = pix.first;
			const sint32 y = pix.second;

			// 收集16个方向上遇到的首个有效视差值
			disp_collects.clear();
			double ang = 0.0;
			for (sint32 s = 0; s < 16; s++) {
				const auto sina = sin(ang);
				const auto cosa = cos(ang);
				for (sint32 m = 1; m < max_search_length; m++) {
					const sint32 yy = lround(y + m * sina);
					const sint32 xx = lround(x + m * cosa);
					if (yy < 0 || yy >= height || xx < 0 || xx >= width) { break;}
					const auto& d = disp_left_[yy * width + xx];
					if (d != Invalid_Float) {
						disp_collects.emplace_back(yy * width * 3 + 3 * xx, d);
						break;
					}
				}
				ang += pi / 16;
			}
			if (disp_collects.empty()) {
				continue;
			}

			// 如果是误匹配区，则选择颜色最相近的像素视差值
			// 如果是遮挡区，则选择最小视差值
			if (k == 0) {
				sint32 min_dist = 9999;
				float32 d = 0.0f;
				const auto color = ADColor(img_left_[y*width * 3 + 3 * x], img_left_[y*width * 3 + 3 * x + 1], img_left_[y*width * 3 + 3 * x + 2]);
				for (auto& dc : disp_collects) {
					const auto color2 = ADColor(img_left_[dc.first], img_left_[dc.first + 1], img_left_[dc.first + 2]);
					const auto dist = abs(color.r - color2.r) + abs(color.g - color2.g) + abs(color.b - color2.b);
					if (min_dist > dist) {
						min_dist = dist;
						d = dc.second;
					}
				}
				fill_disps[n] = d;
			}
			else {
				float32 min_disp = Large_Float;
				for (auto& dc : disp_collects) {
					min_disp = std::min(min_disp, dc.second);
				}
				fill_disps[n] = min_disp;
			}
		}
		for (auto n = 0u; n < trg_pixels.size(); n++) {
			auto& pix = trg_pixels[n];
			const sint32 x = pix.first;
			const sint32 y = pix.second;
			disp_left_[y * width + x] = fill_disps[n];
		}
	}
}

void MultiStepRefiner::DepthDiscontinuityAdjustment()
{
	const sint32 width = width_;
	const sint32 height = height_;
	const auto disp_range = max_disparity_ - min_disparity_;
	if (disp_range <= 0) {
		return;
	}
	
	// 对视差图做边缘检测
	// 边缘检测的方法是灵活的，这里选择sobel算子
	const float32 edge_thres = 5.0f;
	EdgeDetect(&vec_edge_left_[0], disp_left_, width, height, edge_thres);

	// 调整边缘像素的视差
	for (sint32 y = 0; y < height; y++) {
		for (sint32 x = 1; x < width - 1; x++) {
			const auto& e_label = vec_edge_left_[y*width + x];
			if (e_label == 1) {
				const auto disp_ptr = disp_left_ + y*width;
				float32& d = disp_ptr[x];
				if (d != Invalid_Float) {
					const sint32& di = lround(d);
					const auto cost_ptr = cost_ + y*width*disp_range + x*disp_range;
					float32 c0 = cost_ptr[di];

					// 记录左右两边像素的视差值和代价值
					// 选择代价最小的像素视差值
					for (int k = 0; k<2; k++) {
						const sint32 x2 = (k == 0) ? x - 1 : x + 1;
						const float32& d2 = disp_ptr[x2];
						const sint32& d2i = lround(d2);
						if (d2 != Invalid_Float) {
							const auto& c = (k == 0) ? cost_ptr[-disp_range + d2i] : cost_ptr[disp_range + d2i];
							if (c < c0) {
								d = d2;
								c0 = c;
							}
						}
					}
				}
			}
		}
	}
	
}

void MultiStepRefiner::EdgeDetect(uint8* edge_mask, const float32* disp_ptr, const sint32& width, const sint32& height, const float32 threshold)
{
	memset(edge_mask, 0, width*height * sizeof(uint8));
	// sobel算子
	for (int y = 1; y < height - 1; y++) {
		for (int x = 1; x < width - 1; x++) {
			const auto grad_x = (-disp_ptr[(y - 1) * width + x - 1] + disp_ptr[(y - 1) * width + x + 1]) +
				(-2 * disp_ptr[y * width + x - 1] + 2 * disp_ptr[y * width + x + 1]) +
				(-disp_ptr[(y + 1) * width + x - 1] + disp_ptr[(y + 1) * width + x + 1]);
			const auto grad_y = (-disp_ptr[(y - 1) * width + x - 1] - 2 * disp_ptr[(y - 1) * width + x] - disp_ptr[(y - 1) * width + x + 1]) +
				(disp_ptr[(y + 1) * width + x - 1] + 2 * disp_ptr[(y + 1) * width + x] + disp_ptr[(y + 1) * width + x + 1]);
			const auto grad = abs(grad_x) + abs(grad_y);
			if (grad > threshold) {
				edge_mask[y*width + x] = 1;
			}
		}
	}
}
