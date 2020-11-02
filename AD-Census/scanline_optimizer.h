/* -*-c++-*- AD-Census - Copyright (C) 2020.
* Author	: Yingsong Li(Ethan Li) <ethan.li.whu@gmail.com>
* https://github.com/ethan-li-coding/AD-Census
* Describe	: header of class ScanlineOptimizer
*/

#ifndef AD_CENSUS_SCANLNE_OPTIMIZER_H_
#define AD_CENSUS_SCANLNE_OPTIMIZER_H_

#include <algorithm>

#include "adcensus_types.h"

/**
 * \brief 扫描线优化器
 */
class ScanlineOptimizer {
public:
	ScanlineOptimizer();
	~ScanlineOptimizer();


	/**
	 * \brief 设置数据
	 * \param img_left		// 左影像数据，三通道 
	 * \param img_right 	// 右影像数据，三通道
	 * \param cost_init 	// 初始代价数组
	 * \param cost_aggr 	// 聚合代价数组
	 */
	void SetData(const uint8* img_left, const uint8* img_right, float32* cost_init, float32* cost_aggr);

	/**
	 * \brief 
	 * \param width			// 影像宽
	 * \param height		// 影像高
	 * \param min_disparity	// 最小视差
	 * \param max_disparity // 最大视差
	 * \param p1			// p1
	 * \param p2			// p2
	 * \param tso			// tso
	 */
	void SetParam(const sint32& width,const sint32& height, const sint32& min_disparity, const sint32& max_disparity, const float32& p1, const float32& p2, const sint32& tso);

	/**
	 * \brief 优化 */
	void Optimize();

private:
	/**
	* \brief 左右路径优化 → ←
	* \param cost_so_src		输入，SO前代价数据
	* \param cost_so_dst		输出，SO后代价数据
	* \param is_forward			输入，是否为正方向（正方向为从左到右，反方向为从右到左）
	*/
	void ScanlineOptimizeLeftRight(const float32* cost_so_src, float32* cost_so_dst, bool is_forward = true);

	/**
	* \brief 上下路径优化 ↓ ↑
	* \param cost_so_src		输入，SO前代价数据
	* \param cost_so_dst		输出，SO后代价数据
	* \param is_forward			输入，是否为正方向（正方向为从上到下，反方向为从下到上）
	*/
	void ScanlineOptimizeUpDown(const float32* cost_so_src, float32* cost_so_dst, bool is_forward = true);

	/** \brief 计算颜色距离 */
	inline sint32 ColorDist(const ADColor& c1, const ADColor& c2) {
		return std::max(abs(c1.r - c2.r), std::max(abs(c1.g - c2.g), abs(c1.b - c2.b)));
	}
	
private:
	/** \brief 图像尺寸 */
	sint32	width_;
	sint32	height_;

	/** \brief 影像数据 */
	const uint8* img_left_;
	const uint8* img_right_;
	
	/** \brief 初始代价数组 */
	float32* cost_init_;
	/** \brief 聚合代价数组 */
	float32* cost_aggr_;

	/** \brief 最小视差值 */
	sint32 min_disparity_;
	/** \brief 最大视差值 */
	sint32 max_disparity_;
	/** \brief 初始的p1值 */
	float32 so_p1_;
	/** \brief 初始的p2值 */
	float32 so_p2_;
	/** \brief tso阈值 */
	sint32 so_tso_;
};
#endif
