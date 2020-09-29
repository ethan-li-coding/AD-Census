/* -*-c++-*- AD-Census - Copyright (C) 2020.
* Author	: Yingsong Li(Ethan Li) <ethan.li.whu@gmail.com>
* https://github.com/ethan-li-coding/AD-Census
* Describe	: implement of class CrossAggregator
*/

#include "cross_aggregator.h"

CrossAggregator::CrossAggregator(): width_(0), height_(0), img_left_(nullptr), img_right_(nullptr),
                                    cost_init_(nullptr),
                                    cross_L1_(0), cross_L2_(0), cross_t1_(0), cross_t2_(0),
                                    min_disparity_(0), max_disparity_(0), is_initialized_(false) { }

CrossAggregator::~CrossAggregator()
{
	
}

bool CrossAggregator::Initialize(const sint32& width, const sint32& height, const sint32& min_disparity, const sint32& max_disparity)
{
	width_ = width;
	height_ = height;
	min_disparity_ = min_disparity;
	max_disparity_ = max_disparity;
	
	const sint32 img_size = width_ * height_;
	const sint32 disp_range = max_disparity_ - min_disparity_;
	if (img_size <= 0 || disp_range <= 0) {
		is_initialized_ = false;
		return is_initialized_;
	}

	// 为交叉十字臂数组分配内存
	vec_cross_arms_.clear();
	vec_cross_arms_.resize(img_size);

	// 为临时代价数组分配内存
	vec_cost_tmp_[0].clear();
	vec_cost_tmp_[0].resize(img_size);
	vec_cost_tmp_[1].clear();
	vec_cost_tmp_[1].resize(img_size);

	// 为存储每个像素支持区像素数量的数组分配内存
	vec_sup_count_[0].clear();
	vec_sup_count_[0].resize(img_size);
	vec_sup_count_[1].clear();
	vec_sup_count_[1].resize(img_size);
	vec_sup_count_tmp_.clear();
	vec_sup_count_tmp_.resize(img_size);

	// 为聚合代价数组分配内存
	cost_aggr_.resize(img_size * disp_range);

	is_initialized_ = !vec_cross_arms_.empty() && !vec_cost_tmp_[0].empty() && !vec_cost_tmp_[1].empty() 
					&& !vec_sup_count_[0].empty() && !vec_sup_count_[1].empty() 
					&& !vec_sup_count_tmp_.empty() && !cost_aggr_.empty();
	return is_initialized_;
}

void CrossAggregator::SetData(const uint8* img_left, const uint8* img_right, const float32* cost_init)
{
	img_left_ = img_left;
	img_right_ = img_right;
	cost_init_ = cost_init;
}

void CrossAggregator::SetParams(const sint32& cross_L1, const sint32& cross_L2, const sint32& cross_t1,
	const sint32& cross_t2)
{
	cross_L1_ = cross_L1;
	cross_L2_ = cross_L2;
	cross_t1_ = cross_t1;
	cross_t2_ = cross_t2;
}

void CrossAggregator::BuildArms() 
{
	// 逐像素计算十字交叉臂
	for (sint32 y = 0; y < height_; y++) {
		for (sint32 x = 0; x < width_; x++) {
			CrossArm& arm = vec_cross_arms_[y * width_ + x];
			FindHorizontalArm(x, y, arm.left, arm.right);
			FindVerticalArm(x, y, arm.top, arm.bottom);
		}
	}
}


void CrossAggregator::Aggregate(const sint32& num_iters)
{
	if (!is_initialized_) {
		return;
	}

	const sint32 disp_range = max_disparity_ - min_disparity_;

	// 构建像素的十字交叉臂
	BuildArms();

	// 代价聚合
	// horizontal_first 代表先水平方向聚合
	bool horizontal_first = true;

	// 计算两种聚合方向的各像素支持区像素数量
	ComputeSupPixelCount();

	// 先将聚合代价初始化为初始代价
	memcpy(&cost_aggr_[0], cost_init_, width_*height_*disp_range*sizeof(float32));

	// 多迭代聚合
	for (sint32 k = 0; k < num_iters; k++) {
		for (sint32 d = min_disparity_; d < max_disparity_; d++) {
			AggregateInArms(d, horizontal_first);
		}
		// 下一次迭代，调换顺序
		horizontal_first = !horizontal_first;
	}
}

CrossArm* CrossAggregator::get_arms_ptr()
{
	return &vec_cross_arms_[0];
}

float32* CrossAggregator::get_cost_ptr()
{
	if (!cost_aggr_.empty()) {
		return &cost_aggr_[0];
	}
	else {
		return nullptr;
	}
}

void CrossAggregator::FindHorizontalArm(const sint32& x, const sint32& y, uint8& left, uint8& right) const
{
	// 像素数据地址
	const auto img0 = img_left_ + y * width_ * 3 + 3 * x;
	// 像素颜色值
	const ADColor color0(img0[0], img0[1], img0[2]);
	
	left = right = 0;
	//计算左右臂,先左臂后右臂
	sint32 dir = -1;
	for (sint32 k = 0; k < 2; k++) {
		// 延伸臂直到条件不满足
		// 臂长不得超过cross_L1
		auto img = img0 + dir * 3;
		auto color_last = color0;
		sint32 xn = x + dir;
		for (sint32 n = 0; n < std::min(cross_L1_, MAX_ARM_LENGTH); n++) {

			// 边界处理
			if (k == 0) {
				if (xn < 0) {
					break;
				}
			}
			else {
				if (xn == width_) {
					break;
				}
			}

			// 获取颜色值
			const ADColor color(img[0], img[1], img[2]);

			// 颜色距离1（臂上像素和计算像素的颜色距离）
			const sint32 color_dist1 = ColorDist(color, color0);
			if (color_dist1 >= cross_t1_) {
				break;
			}

			// 颜色距离2（臂上像素和前一个像素的颜色距离）
			if (n > 0) {
				const sint32 color_dist2 = ColorDist(color, color_last);
				if (color_dist2 >= cross_t1_) {
					break;
				}
			}

			// 臂长大于L2后，颜色距离阈值减小为t2
			if (n + 1 > cross_L2_) {
				if (color_dist1 >= cross_t2_) {
					break;
				}
			}

			if (k == 0) {
				left++;
			}
			else {
				right++;
			}
			color_last = color;
			xn += dir;
			img += dir * 3;
		}
		dir = -dir;
	}
}

void CrossAggregator::FindVerticalArm(const sint32& x, const sint32& y, uint8& top, uint8& bottom) const
{
	// 像素数据地址
	const auto img0 = img_left_ + y * width_ * 3 + 3 * x;
	// 像素颜色值
	const ADColor color0(img0[0], img0[1], img0[2]);

	top = bottom = 0;
	//计算上下臂,先上臂后下臂
	sint32 dir = -1;
	for (sint32 k = 0; k < 2; k++) {
		// 延伸臂直到条件不满足
		// 臂长不得超过cross_L1
		auto img = img0 + dir * width_ * 3;
		auto color_last = color0;
		sint32 yn = y + dir;
		for (sint32 n = 0; n < std::min(cross_L1_, MAX_ARM_LENGTH); n++) {

			// 边界处理
			if (k == 0) {
				if (yn < 0) {
					break;
				}
			}
			else {
				if (yn == height_) {
					break;
				}
			}

			// 获取颜色值
			const ADColor color(img[0], img[1], img[2]);

			// 颜色距离1（臂上像素和计算像素的颜色距离）
			const sint32 color_dist1 = ColorDist(color, color0);
			if (color_dist1 >= cross_t1_) {
				break;
			}

			// 颜色距离2（臂上像素和前一个像素的颜色距离）
			if (n > 0) {
				const sint32 color_dist2 = ColorDist(color, color_last);
				if (color_dist2 >= cross_t1_) {
					break;
				}
			}

			// 臂长大于L2后，颜色距离阈值减小为t2
			if (n + 1 > cross_L2_) {
				if (color_dist1 >= cross_t2_) {
					break;
				}
			}

			if (k == 0) {
				top++;
			}
			else {
				bottom++;
			}
			color_last = color;
			yn += dir;
			img += dir * width_ * 3;
		}
		dir = -dir;
	}
}

void CrossAggregator::ComputeSupPixelCount()
{
	// 计算每个像素的支持区像素数量
	// 注意：两种不同的聚合方向，像素的支持区像素是不同的，需要分开计算
	bool horizontal_first = true;
	for (sint32 n = 0; n < 2; n++) {
		// n=0 : horizontal_first; n=1 : vertical_first
		const sint32 id = horizontal_first ? 0 : 1;
		for (sint32 k = 0; k < 2; k++) {
			// k=0 : pass1; k=1 : pass2
			for (sint32 y = 0; y < height_; y++) {
				for (sint32 x = 0; x < width_; x++) {
					// 获取arm数值
					auto& arm = vec_cross_arms_[y*width_ + x];
					sint32 count = 0;
					if (horizontal_first) {
						if (k == 0) {
							// horizontal
							for (sint32 t = -arm.left; t <= arm.right; t++) {
								count++;
							}
						}
						else {
							// vertical
							for (sint32 t = -arm.top; t <= arm.bottom; t++) {
								count += vec_sup_count_tmp_[(y + t)*width_ + x];
							}
						}
					}
					else {
						if (k == 0) {
							// vertical
							for (sint32 t = -arm.top; t <= arm.bottom; t++) {
								count++;
							}
						}
						else {
							// horizontal
							for (sint32 t = -arm.left; t <= arm.right; t++) {
								count += vec_sup_count_tmp_[y*width_ + x + t];
							}
						}
					}
					if (k == 0) {
						vec_sup_count_tmp_[y*width_ + x] = count;
					}
					else {
						vec_sup_count_[id][y*width_ + x] = count;
					}
				}
			}
		}
		horizontal_first = !horizontal_first;
	}
}

void CrossAggregator::AggregateInArms(const sint32& disparity, const bool& horizontal_first)
{
	// 此函数聚合所有像素当视差为disparity时的代价

	if (disparity < min_disparity_ || disparity >= max_disparity_) {
		return;
	}
	const auto disp = disparity - min_disparity_;
	const sint32 disp_range = max_disparity_ - min_disparity_;
	if (disp_range <= 0) {
		return;
	}

	// 将disp层的代价存入临时数组vec_cost_tmp_[0]
	// 这样可以避免过多的访问更大的cost_aggr_,提高访问效率
	for (sint32 y = 0; y < height_; y++) {
		for (sint32 x = 0; x < width_; x++) {
			vec_cost_tmp_[0][y * width_ + x] = cost_aggr_[y * width_ * disp_range + x * disp_range + disp];
		}
	}

	// 逐像素聚合
	const sint32 ct_id = horizontal_first ? 0 : 1;
	for (sint32 k = 0; k < 2; k++) {
		// k==0: pass1
		// k==1: pass2
		for (sint32 y = 0; y < height_; y++) {
			for (sint32 x = 0; x < width_; x++) {
				// 获取arm数值
				auto& arm = vec_cross_arms_[y*width_ + x];
				// 聚合
				float32 cost = 0.0f;
				if (horizontal_first) {
					if (k == 0) {
						// horizontal
						for (sint32 t = -arm.left; t <= arm.right; t++) {
							cost += vec_cost_tmp_[0][y * width_ + x + t];
						}
					} else {
						// vertical
						for (sint32 t = -arm.top; t <= arm.bottom; t++) {
							cost += vec_cost_tmp_[1][(y + t)*width_ + x];
						}
					}
				}
				else {
					if (k == 0) {
						// vertical
						for (sint32 t = -arm.top; t <= arm.bottom; t++) {
							cost += vec_cost_tmp_[0][(y + t) * width_ + x];
						}
					} else {
						// horizontal
						for (sint32 t = -arm.left; t <= arm.right; t++) {
							cost += vec_cost_tmp_[1][y*width_ + x + t];
						}
					}
				}
				if (k == 0) {
					vec_cost_tmp_[1][y*width_ + x] = cost;
				}
				else {
					cost_aggr_[y*width_*disp_range + x*disp_range + disp] = cost / vec_sup_count_[ct_id][y*width_ + x];
				}
			}
		}
	}
}
