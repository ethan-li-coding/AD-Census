/* -*-c++-*- AD-Census - Copyright (C) 2020.
* Author	: Ethan Li<ethan.li.whu@gmail.com>
* https://github.com/ethan-li-coding/AD-Census
* Describe	: implement of class MultiStepRefiner
*/
#ifndef AD_CENSUS_MULTISTEP_REFINER_H_
#define AD_CENSUS_MULTISTEP_REFINER_H_

#include "adcensus_types.h"
#include "cross_aggregator.h"

class MultiStepRefiner
{
public:
	MultiStepRefiner();
	~MultiStepRefiner();

	/**
	 * \brief 初始化
	 * \param width		影像宽
	 * \param height	影像高
	 * \return true:初始化成功
	 */
	bool Initialize(const sint32& width, const sint32& height);

	/**
	 * \brief 设置多步优化的数据
	 * \param cost				// 代价数据
	 * \param cross_arms		// 十字交叉臂数据
	 * \param disp_left			// 左视图视差数据
	 * \param disp_right		// 右视图视差数据
	 */
	void SetData(const float32* cost, const vector<CrossArm>* cross_arms, float32* disp_left, float32* disp_right);


	/**
	 * \brief 设置多步优化的参数
	 * \param min_disparity	// 最小视差
	 * \param max_disparity	// 最大视差
	 * \param irv_ts		// Iterative Region Voting参数ts
	 * \param irv_th		// Iterative Region Voting参数th
	 * \param lrcheck_thres	// 一致性检查阈值
	 */
	void SetParam(const sint32& min_disparity, const sint32& max_disparity, const sint32& irv_ts, const float32& irv_th, const float32& lrcheck_thres);

	void Refine();

private:
	//------4小步视差优化------//
	/** \brief 离群点检测	 */
	void OutlierDetection();
	/** \brief 迭代局部投票 */
	void IterativeRegionVoting();
	/** \brief 内插填充 */
	void ProperInterpolation();
	/** \brief 深度非连续区视差调整 */
	void DepthDiscontinuityAdjustment();

private:
	/** \brief 图像尺寸 */
	sint32	width_;
	sint32	height_;

	/** \brief 代价数据 */
	const float32* cost_;
	/** \brief 交叉臂数据 */
	const vector<CrossArm>* vec_cross_arms_;

	/** \brief 左视图视差数据 */
	float* disp_left_;
	/** \brief 右视图视差数据 */
	float* disp_right_;

	/** \brief 最小视差值 */
	sint32 min_disparity_;
	/** \brief 最大视差值 */
	sint32 max_disparity_;

	/** \brief Iterative Region Voting参数ts */
	sint32	irv_ts_;
	/** \brief Iterative Region Voting参数th */
	float32 irv_th_;

	/** \brief 一致性检查阈值 */
	float32 lrcheck_thres_;

	/** \brief 遮挡区像素集	*/
	std::vector<std::pair<int, int>> occlusions_;
	/** \brief 误匹配区像素集	*/
	std::vector<std::pair<int, int>> mismatches_;
};
#endif
