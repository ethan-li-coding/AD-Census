/* -*-c++-*- AD-Census - Copyright (C) 2020.
* Author	: Ethan Li<ethan.li.whu@gmail.com>
* https://github.com/ethan-li-coding/AD-Census
* Describe	: header of adcensus_util
*/

#pragma once
#include <algorithm>
#include "adcensus_types.h"


namespace adcensus_util
{
	/**
	* \brief census变换
	* \param source	输入，影像数据
	* \param census	输出，census值数组
	* \param width	输入，影像宽
	* \param height	输入，影像高
	*/
	void census_transform_9x7(const uint8* source, uint64* census, const sint32& width, const sint32& height);
	// Hamming距离
	uint8 Hamming64(const uint64& x, const uint64& y);

	/**
	* \brief 左右路径聚合 → ←
	* \param img_left			输入，左影像数据
	* \param img_right			输入，右影像数据
	* \param width				输入，影像宽
	* \param height				输入，影像高
	* \param min_disparity		输入，最小视差
	* \param max_disparity		输入，最大视差
	* \param p1					输入，惩罚项P1
	* \param p2					输入，惩罚项P2_Init
	* \param tso				输入，惩罚项阈值
	* \param cost_init			输入，初始代价数据
	* \param cost_aggr			输出，路径聚合代价数据
	* \param is_forward			输入，是否为正方向（正方向为从左到右，反方向为从右到左）
	*/
	void CostAggregateLeftRight(const uint8* img_left, const uint8* img_right, const sint32& width, const sint32& height, const sint32& min_disparity, const sint32& max_disparity,
		const float32& p1, const float32& p2, const sint32& tso, const float32* cost_init, float32* cost_aggr, bool is_forward = true);

	/**
	* \brief 上下路径聚合 ↓ ↑
	* \param img_left			输入，左影像数据
	* \param img_right			输入，右影像数据
	* \param width				输入，影像宽
	* \param height				输入，影像高
	* \param min_disparity		输入，最小视差
	* \param max_disparity		输入，最大视差
	* \param p1					输入，惩罚项P1
	* \param p2					输入，惩罚项P2_Init
	* \param tso				输入，惩罚项阈值
	* \param cost_init			输入，初始代价数据
	* \param cost_aggr			输出，路径聚合代价数据
	* \param is_forward			输入，是否为正方向（正方向为从上到下，反方向为从下到上）
	*/
	void CostAggregateUpDown(const uint8* img_left, const uint8* img_right, const sint32& width, const sint32& height, const sint32& min_disparity, const sint32& max_disparity,
		const float32& p1, const float32& p2, const sint32& tso, const float32* cost_init, float32* cost_aggr, bool is_forward = true);

	/** \brief 计算颜色距离 */
	inline sint32 ColorDist(const ADColor& c1, const ADColor& c2) {
		return std::max(abs(c1.r - c2.r), std::max(abs(c1.g - c2.g), abs(c1.b - c2.b)));
	}
}