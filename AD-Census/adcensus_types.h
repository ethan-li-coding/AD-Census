/* -*-c++-*- AD-Census - Copyright (C) 2020.
* Author	: Yingsong Li(Ethan Li) <ethan.li.whu@gmail.com>
*			  https://github.com/ethan-li-coding/AD-Census
* Describe	: header of adcensus_types
*/

#ifndef ADCENSUS_STEREO_TYPES_H_
#define ADCENSUS_STEREO_TYPES_H_

#include <cstdint>
#include <limits>
#include <vector>
using std::vector;
using std::pair;

#ifndef SAFE_DELETE
#define SAFE_DELETE(P) {if(P) delete[](P);(P)=nullptr;}
#endif

/** \brief 基础类型别名 */
typedef int8_t			sint8;		// 有符号8位整数
typedef uint8_t			uint8;		// 无符号8位整数
typedef int16_t			sint16;		// 有符号16位整数
typedef uint16_t		uint16;		// 无符号16位整数
typedef int32_t			sint32;		// 有符号32位整数
typedef uint32_t		uint32;		// 无符号32位整数
typedef int64_t			sint64;		// 有符号64位整数
typedef uint64_t		uint64;		// 无符号64位整数
typedef float			float32;	// 单精度浮点
typedef double			float64;	// 双精度浮点

/** \brief float32无效值 */
constexpr auto Invalid_Float = std::numeric_limits<float32>::infinity();

constexpr auto Large_Float = 99999.0f;
constexpr auto Small_Float = -99999.0f;

/** \brief Census窗口尺寸类型 */
enum CensusSize {
	Census5x5 = 0,
	Census9x7
};

/** \brief ADCensus参数结构体 */
struct ADCensusOption {
	sint32  min_disparity;		// 最小视差
	sint32	max_disparity;		// 最大视差

	sint32	lambda_ad;			// 控制AD代价值的参数
	sint32	lambda_census;		// 控制Census代价值的参数
	sint32	cross_L1;			// 十字交叉窗口的空间域参数：L1
	sint32  cross_L2;			// 十字交叉窗口的空间域参数：L2
	sint32	cross_t1;			// 十字交叉窗口的颜色域参数：t1
	sint32  cross_t2;			// 十字交叉窗口的颜色域参数：t2
	float32	so_p1;				// 扫描线优化参数p1
	float32	so_p2;				// 扫描线优化参数p2
	sint32	so_tso;				// 扫描线优化参数tso
	sint32	irv_ts;				// Iterative Region Voting法参数ts
	float32 irv_th;				// Iterative Region Voting法参数th
	
	float32	lrcheck_thres;		// 左右一致性约束阈值

	bool	do_lr_check;					// 是否检查左右一致性
	bool	do_filling;						// 是否做视差填充
	bool	do_discontinuity_adjustment;	// 是否做非连续区调整
	
	ADCensusOption(): min_disparity(0), max_disparity(64),
	                  lambda_ad(10), lambda_census(30),
	                  cross_L1(34), cross_L2(17),
	                  cross_t1(20), cross_t2(6),
	                  so_p1(1.0f), so_p2(3.0f),
	                  so_tso(15), irv_ts(20), irv_th(0.4f),
	                  lrcheck_thres(1.0f),
					  do_lr_check(true), do_filling(true), do_discontinuity_adjustment(false) {} ;
};

/**
* \brief 颜色结构体
*/
struct ADColor {
	uint8 r, g, b;
	ADColor() : r(0), g(0), b(0) {}
	ADColor(uint8 _b, uint8 _g, uint8 _r) {
		r = _r; g = _g; b = _b;
	}
};

#endif
