/* -*-c++-*- AD-Census - Copyright (C) 2020.
* Author	: Yingsong Li(Ethan Li) <ethan.li.whu@gmail.com>
* https://github.com/ethan-li-coding/AD-Census
* Describe	: header of class CrossAggregator
*/

#ifndef AD_CENSUS_CROSS_AGGREGATOR_H_
#define AD_CENSUS_CROSS_AGGREGATOR_H_

#include "adcensus_types.h"
#include <algorithm>

/**
* \brief 交叉十字臂结构
* 为了限制内存占用，臂长类型设置为uint8，这意味着臂长最长不能超过255
*/
struct CrossArm {
	uint8 left, right, top, bottom;
	CrossArm() : left(0), right(0), top(0), bottom(0) { }
};
/**\brief 最大臂长 */
#define MAX_ARM_LENGTH 255 

/**
 * \brief 十字交叉域代价聚合器
 */
class CrossAggregator {
public:
	CrossAggregator();
	~CrossAggregator();

	/**
	 * \brief 初始化代价聚合器
	 * \param width		影像宽
	 * \param height	影像高
	 * \return true:初始化成功
	 */
	bool Initialize(const sint32& width, const sint32& height, const sint32& min_disparity, const sint32& max_disparity);

	/**
	 * \brief 设置代价聚合器的数据
	 * \param img_left		// 左影像数据，三通道
	 * \param img_right		// 右影像数据，三通道
	 * \param cost_init		// 初始代价数组
	 */
	void SetData(const uint8* img_left, const uint8* img_right, const float32* cost_init);

	/**
	 * \brief 设置代价聚合器的参数
	 * \param cross_L1		// L1
	 * \param cross_L2		// L2
	 * \param cross_t1		// t1
	 * \param cross_t2		// t2
	 */
	void SetParams(const sint32& cross_L1, const sint32& cross_L2, const sint32& cross_t1, const sint32& cross_t2);

	/** \brief 聚合 */
	void Aggregate(const sint32& num_iters);

	/** \brief 获取所有像素的十字交叉臂数据指针 */
	CrossArm* get_arms_ptr();

	/** \brief 获取聚合代价数组指针 */
	float32* get_cost_ptr();
private:
	/** \brief 构建十字交叉臂 */
	void BuildArms();
	/** \brief 搜索水平臂 */
	void FindHorizontalArm(const sint32& x, const sint32& y, uint8& left, uint8& right) const;
	/** \brief 搜索竖直臂 */
	void FindVerticalArm(const sint32& x, const sint32& y, uint8& top, uint8& bottom) const;
	/** \brief 计算像素的支持区像素数量 */
	void ComputeSupPixelCount();
	/** \brief 聚合某个视差 */
	void AggregateInArms(const sint32& disparity, const bool& horizontal_first);

	/** \brief 计算颜色距离 */
	inline sint32 ColorDist(const ADColor& c1,const ADColor& c2) const {
		return std::max(abs(c1.r - c2.r), std::max(abs(c1.g - c2.g), abs(c1.b - c2.b)));
	}
private:
	/** \brief 图像尺寸 */
	sint32	width_;
	sint32	height_;

	/** \brief 交叉臂 */
	vector<CrossArm> vec_cross_arms_;

	/** \brief 影像数据 */
	const uint8* img_left_;
	const uint8* img_right_;

	/** \brief 初始代价数组指针 */
	const float32* cost_init_;
	/** \brief 聚合代价数组 */
	vector<float32> cost_aggr_;

	/** \brief 临时代价数据 */
	vector<float32> vec_cost_tmp_[2];
	/** \brief 支持区像素数量数组 0：水平臂优先 1：竖直臂优先 */
	vector<uint16> vec_sup_count_[2];
	vector<uint16> vec_sup_count_tmp_;

	sint32	cross_L1_;			// 十字交叉窗口的空间域参数：L1
	sint32  cross_L2_;			// 十字交叉窗口的空间域参数：L2
	sint32	cross_t1_;			// 十字交叉窗口的颜色域参数：t1
	sint32  cross_t2_;			// 十字交叉窗口的颜色域参数：t2
	sint32  min_disparity_;			// 最小视差
	sint32	max_disparity_;			// 最大视差

	/** \brief 是否成功初始化标志	*/
	bool is_initialized_;
};
#endif
