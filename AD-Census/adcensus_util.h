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
}