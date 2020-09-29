/* -*-c++-*- AD-Census - Copyright (C) 2020.
* Author	: Yingsong Li(Ethan Li) <ethan.li.whu@gmail.com>
*			  https://github.com/ethan-li-coding/AD-Census
* Describe	: header of ad-census stereo class
*/
#pragma once

#include "adcensus_types.h"
#include "cost_computor.h"
#include "cross_aggregator.h"
#include "scanline_optimizer.h"
#include "multistep_refiner.h"

class ADCensusStereo {	
public:
	ADCensusStereo();
	~ADCensusStereo();

	/**
	* \brief 类的初始化，完成一些内存的预分配、参数的预设置等
	* \param width		输入，核线像对影像宽
	* \param height		输入，核线像对影像高
	* \param option		输入，算法参数
	*/
	bool Initialize(const sint32& width, const sint32& height, const ADCensusOption& option);

	/**
	* \brief 执行匹配
	* \param img_left	输入，左影像数据指针，3通道彩色数据
	* \param img_right	输入，右影像数据指针，3通道彩色数据
	* \param disp_left	输出，左影像视差图指针，预先分配和影像等尺寸的内存空间
	*/
	bool Match(const uint8* img_left, const uint8* img_right, float32* disp_left);

	/**
	* \brief 重设
	* \param width		输入，核线像对影像宽
	* \param height		输入，核线像对影像高
	* \param option		输入，算法参数
	*/
	bool Reset(const uint32& width, const uint32& height, const ADCensusOption& option);

private:
	/** \brief 代价计算 */
	void ComputeCost();

	/** \brief 代价聚合 */
	void CostAggregation();

	/** \brief 扫描线优化	 */
	void ScanlineOptimize();

	/** \brief 多步骤视差优化	*/
	void MultiStepRefine();

	/** \brief 视差计算（左视图）*/
	void ComputeDisparity();

	/** \brief 视差计算（右视图）*/
	void ComputeDisparityRight();

	/** \brief 内存释放 */
	void Release();

private:
	/** \brief 算法参数 */
	ADCensusOption option_;

	/** \brief 影像宽 */
	sint32 width_;
	/** \brief 影像高 */
	sint32 height_;

	/** \brief 左影像数据，3通道彩色数据 */
	const uint8* img_left_;
	/** \brief 右影像数据	，3通道彩色数据 */
	const uint8* img_right_;

	/** \brief 代价计算器 */
	CostComputor cost_computer_;
	/** \brief 代价聚合器 */
	CrossAggregator aggregator_;
	/** \brief 扫描线优化器 */
	ScanlineOptimizer scan_line_;
	/** \brief 多步优化器 */
	MultiStepRefiner refiner_;

	/** \brief 左影像视差图 */
	float32* disp_left_;
	/** \brief 右影像视差图 */
	float32* disp_right_;

	/** \brief 是否初始化标志	*/
	bool is_initialized_;
};
