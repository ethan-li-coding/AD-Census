/* -*-c++-*- AD-Census - Copyright (C) 2020.
* Author	: Ethan Li<ethan.li.whu@gmail.com>
* https://github.com/ethan-li-coding/AD-Census
* Describe	: implement of class MultiStepRefiner
*/

#include "multistep_refiner.h"

MultiStepRefiner::MultiStepRefiner()
{
	
}

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

	return true;
}

void MultiStepRefiner::SetData(const float32* cost, const vector<CrossArm>* cross_arms, float32* disp_left, float32* disp_right)
{
	cost_ = cost; 
	vec_cross_arms_ = cross_arms;
	disp_left_ = disp_left;
	disp_right_= disp_right;
}

void MultiStepRefiner::SetParam(const sint32& min_disparity, const sint32& max_disparity, const sint32& irv_ts, const float32& irv_th, const float32& lrcheck_thres)
{
	min_disparity_ = min_disparity;
	max_disparity_ = max_disparity;
	irv_ts_ = irv_ts;
	irv_th_ = irv_th;
	lrcheck_thres_ = lrcheck_thres;
}

void MultiStepRefiner::Refine()
{
	if (width_ <= 0 || height_ <= 0 ||
		disp_left_ == nullptr || disp_right_ == nullptr ||
		cost_ == nullptr || vec_cross_arms_ == nullptr) {
		return;
	}

	// step1: outlier detection
	OutlierDetection();
	// step2: iterative region voting
	IterativeRegionVoting();
	// step3: proper interpolation
	ProperInterpolation();
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
	const auto& vec_arms = *vec_cross_arms_;

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
				auto& arm = vec_arms[y * width + x];
				// 遍历支持区像素视差，统计直方图
				for (sint32 t = -arm.top; t <= arm.bottom; t++) {
					const sint32& yt = y + t;
					auto& arm2 = vec_arms[yt * width_ + x];
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

	std::vector<float32> disp_collects;

	// 定义8个方向
	const float32 pi = 3.1415926f;
	float32 angle1[8] = { pi, 3 * pi / 4, pi / 2, pi / 4, 0, 7 * pi / 4, 3 * pi / 2, 5 * pi / 4 };
	float32 angle2[8] = { pi, 5 * pi / 4, 3 * pi / 2, 7 * pi / 4, 0, pi / 4, pi / 2, 3 * pi / 4 };
	float32* angle = angle1;
	// 最大搜索行程，没有必要搜索过远的像素
	const sint32 max_search_length = 2.0 * std::max(abs(max_disparity_), abs(min_disparity_));

	for (sint32 k = 0; k < 2; k++) {
		auto& trg_pixels = (k == 0) ? mismatches_ : occlusions_;
		if (trg_pixels.empty()) {
			continue;
		}
		std::vector<float32> fill_disps(trg_pixels.size());
		std::vector<std::pair<sint32, sint32>> inv_pixels;

		// 遍历待处理像素
		for (auto n = 0u; n < trg_pixels.size(); n++) {
			auto& pix = trg_pixels[n];
			const sint32 x = pix.first;
			const sint32 y = pix.second;

			// 收集8个方向上遇到的首个有效视差值
			disp_collects.clear();
			for (sint32 s = 0; s < 8; s++) {
				const float32 ang = angle[s];
				const float32 sina = float32(sin(ang));
				const float32 cosa = float32(cos(ang));
				for (sint32 m = 1; m < max_search_length; m++) {
					const sint32 yy = lround(y + m * sina);
					const sint32 xx = lround(x + m * cosa);
					if (yy < 0 || yy >= height || xx < 0 || xx >= width) {
						break;
					}
					const auto& disp = *(disp_left_ + yy * width + xx);
					if (disp != Invalid_Float) {
						disp_collects.push_back(disp);
						break;
					}
				}
			}
			if (disp_collects.empty()) {
				continue;
			}

			std::sort(disp_collects.begin(), disp_collects.end());

			// 如果是误匹配区，则选择中值
			// 如果是遮挡区，则选择第二小的视差值
			if (k == 0) {
				fill_disps[n] = disp_collects[disp_collects.size() / 2];
			}
			else {
				if (disp_collects.size() > 1) {
					fill_disps[n] = disp_collects[1];
				}
				else {
					fill_disps[n] = disp_collects[0];
				}
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

}
