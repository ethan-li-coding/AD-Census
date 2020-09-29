/* -*-c++-*- AD-Census - Copyright (C) 2020.
* Author	: Yingsong Li(Ethan Li) <ethan.li.whu@gmail.com>
* https://github.com/ethan-li-coding/AD-Census
* Describe	: header of class CostComputor
*/

#ifndef AD_CENSUS_COST_COMPUTOR_H_
#define AD_CENSUS_COST_COMPUTOR_H_

#include "adcensus_types.h"

/**
 * \brief 代价计算器类
 */
class CostComputor {
public:
	CostComputor();
	~CostComputor();

	/**
	 * \brief 初始化
	 * \param width			影像宽
	 * \param height		影像高
	 * \param min_disparity	最小视差
	 * \param max_disparity	最大视差
	 * \return true: 初始化成功
	 */
	bool Initialize(const sint32& width, const sint32& height, const sint32& min_disparity, const sint32& max_disparity);

	/**
	 * \brief 设置代价计算器的数据
	 * \param img_left		// 左影像数据，三通道
	 * \param img_right		// 右影像数据，三通道
	 */
	void SetData(const uint8* img_left, const uint8* img_right);

	/**
	 * \brief 设置代价计算器的参数
	 * \param lambda_ad		// lambda_ad
	 * \param lambda_census // lambda_census
	 */
	void SetParams(const sint32& lambda_ad, const sint32& lambda_census);

	/** \brief 计算初始代价 */
	void Compute();

	/** \brief 获取初始代价数组指针 */
	float32* get_cost_ptr();

private:
	/** \brief 计算灰度数据 */
	void ComputeGray();

	/** \brief Census变换 */
	void CensusTransform();

	/** \brief 计算代价 */
	void ComputeCost();
private:
	/** \brief 图像尺寸 */
	sint32	width_;
	sint32	height_;

	/** \brief 影像数据 */
	const uint8* img_left_;
	const uint8* img_right_;

	/** \brief 左影像灰度数据	 */
	vector<uint8> gray_left_;
	/** \brief 右影像灰度数据	 */
	vector<uint8> gray_right_;

	/** \brief 左影像census数组	*/
	vector<uint64> census_left_;
	/** \brief 右影像census数组	*/
	vector<uint64> census_right_;

	/** \brief 初始匹配代价	*/
	vector<float32> cost_init_;

	/** \brief lambda_ad*/
	sint32 lambda_ad_;
	/** \brief lambda_census*/
	sint32 lambda_census_;

	/** \brief 最小视差值 */
	sint32 min_disparity_;
	/** \brief 最大视差值 */
	sint32 max_disparity_;

	/** \brief 是否成功初始化标志	*/
	bool is_initialized_;
};
#endif