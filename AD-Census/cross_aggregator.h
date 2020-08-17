/* -*-c++-*- AD-Census - Copyright (C) 2020.
* Author	: Ethan Li<ethan.li.whu@gmail.com>
* https://github.com/ethan-li-coding/AD-Census
* Describe	: header of class CrossAggregator
*/

#ifndef AD_CENSUS_CROSS_AGGREGATOR_H_
#define AD_CENSUS_CROSS_AGGREGATOR_H_

#include "adcensus_types.h"

class CrossAggregator {
public:
	CrossAggregator();
	~CrossAggregator();

	/**
	 * \brief 交叉十字臂结构
	 * 为了限制内存占用，臂长类型设置为uint8，这意味着臂长最长不能超过255
	*/
	struct CrossArm {
		uint8 left, right, top, bottom;
		CrossArm(): left(0), right(0), top(0), bottom(0) { }
	};

	bool Initialize(const sint32& width, const sint32& height);

	void SetData(const uint8* img_left, const uint8* img_right, const float32* cost_init, float32* cost_aggr);

	void SetParams(const sint32& cross_L1, const sint32& cross_L2, const sint32& cross_t1, const sint32& cross_t2, const sint32& min_disparity, const sint32& max_disparity);

	void BuildArms();
	
	void Aggregate(const sint32& num_iters);

	vector<CrossArm>& get_arms();
private:
	void FindHorizontalArm(const sint32& x, const sint32& y, uint8& left, uint8& right) const;
	void FindVerticalArm(const sint32& x, const sint32& y, uint8& top, uint8& bottom) const;
	void ComputeSupPixelCount();
	void AggregateInArms(const sint32& disparity, const bool& horizontal_first);
private:
	/** \brief 图像尺寸 */
	sint32	width_;
	sint32	height_;

	/** \brief 交叉臂 */
	vector<CrossArm> mat_cross_arms_;

	/** \brief 影像数据 */
	const uint8* img_left_;
	const uint8* img_right_;

	/** \brief 初始代价数据 */
	const float32* cost_init_;
	/** \brief 聚合代价数据 */
	float32* cost_aggr_;

	/** \brief 临时代价数据 */
	vector<float32> mat_cost_tmp_;
	/** \brief 支持区像素数量数组 0：水平臂优先 1：竖直臂优先 */
	vector<uint16> mat_sup_count_[2];
	vector<uint16> mat_sup_count_tmp_;

	sint32	cross_L1_;			// 十字交叉窗口的空间域参数：L1
	sint32  cross_L2_;			// 十字交叉窗口的空间域参数：L2
	sint32	cross_t1_;			// 十字交叉窗口的颜色域参数：t1
	sint32  cross_t2_;			// 十字交叉窗口的颜色域参数：t2
	sint32  min_disp_;			// 最小视差
	sint32	max_disp_;			// 最大视差
};
#endif
