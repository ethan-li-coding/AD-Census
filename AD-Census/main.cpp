/* -*-c++-*- AD-Census - Copyright (C) 2020.
* Author	: Ethan Li <ethan.li.whu@gmail.com>
* https://github.com/ethan-li-coding/AD-Census
* Describe	: main
*/
#include <iostream>
#include "ADCensusStereo.h"
#include <chrono>
using namespace std::chrono;

// opencv library
#include <opencv2/opencv.hpp>
#ifdef _DEBUG
#pragma comment(lib,"opencv_world310d.lib")
#else
#pragma comment(lib,"opencv_world310.lib")
#endif

/*显示视差图*/
void ShowDisparityMap(const float32* disp_map, const sint32& width, const sint32& height, const std::string& name);
/*保存视差图*/
void SaveDisparityMap(const float32* disp_map, const sint32& width, const sint32& height, const std::string& path);
/*保存视差点云*/
void SaveDisparityCloud(const uint8* img_bytes, const float32* disp_map, const sint32& width, const sint32& height, const std::string& path);

/*显示深度图*/
void disp2DepthMap(const float32* disp_map, cv::Mat depth_mat, const float32& fx, const float32& baseline, const sint32& width, const sint32& height, const std::string& name);
/*保存深度图*/
void SaveDepthMap(const float32* disp_map, const float32& fx, const float32& baseline, const sint32& width, const sint32& height, const std::string& path);
/*保存深度点云*/
void SaveDepthCloud(const uint8* img_bytes, const float32* disp_map, const float32& fx, const float32& baseline, const sint32& width, const sint32& height, const std::string& path);


/**
* \brief
* \param argv 3
* \param argc argc[1]:左影像路径 argc[2]: 右影像路径 argc[3]: 最小视差[可选，默认0] argc[4]: 最大视差[可选，默认64]
* \param eg. ..\Data\cone\im2.png ..\Data\cone\im6.png 0 64
* \param eg. ..\Data\Cloth3\view1.png ..\Data\Cloth3\view5.png 0 128
* \return
*/
int main(int argv, char** argc)
{
	if (argv < 3) {
		std::cout << "参数过少，请至少指定左右影像路径！" << std::endl;
		return -1;
	}

	printf("Image Loading...");
	//···············································································//
	// 读取影像
	std::string path_left = argc[1];
	std::string path_right = argc[2];

	cv::Mat img_left = cv::imread(path_left, cv::IMREAD_COLOR);
	cv::Mat img_right = cv::imread(path_right, cv::IMREAD_COLOR);

	if (img_left.data == nullptr || img_right.data == nullptr) {
		std::cout << "读取影像失败！" << std::endl;
		return -1;
	}
	if (img_left.rows != img_right.rows || img_left.cols != img_right.cols) {
		std::cout << "左右影像尺寸不一致！" << std::endl;
		return -1;
	}


	//···············································································//
	const sint32 width = static_cast<uint32>(img_left.cols);
	const sint32 height = static_cast<uint32>(img_right.rows);

	// 左右影像的彩色数据
	auto bytes_left = new uint8[width * height * 3];
	auto bytes_right = new uint8[width * height * 3];
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			bytes_left[i * 3 * width + 3 * j] = img_left.at<cv::Vec3b>(i, j)[0];
			bytes_left[i * 3 * width + 3 * j + 1] = img_left.at<cv::Vec3b>(i, j)[1];
			bytes_left[i * 3 * width + 3 * j + 2] = img_left.at<cv::Vec3b>(i, j)[2];
			bytes_right[i * 3 * width + 3 * j] = img_right.at<cv::Vec3b>(i, j)[0];
			bytes_right[i * 3 * width + 3 * j + 1] = img_right.at<cv::Vec3b>(i, j)[1];
			bytes_right[i * 3 * width + 3 * j + 2] = img_right.at<cv::Vec3b>(i, j)[2];
		}
	}
	printf("Done!\n");

	// AD-Census匹配参数设计
	ADCensusOption ad_option;
	// 候选视差范围
	ad_option.min_disparity = argv < 4 ? 0 : atoi(argc[3]);
	ad_option.max_disparity = argv < 5 ? 64 : atoi(argc[4]);
	// 一致性检查阈值
	ad_option.lrcheck_thres = 1.0f;

	// 是否执行一致性检查
	ad_option.do_lr_check = true;

	// 是否执行视差填充
	// 视差图填充的结果并不可靠，若工程，不建议填充，若科研，则可填充
	ad_option.do_filling = true;
	
	printf("w = %d, h = %d, d = [%d,%d]\n\n", width, height, ad_option.min_disparity, ad_option.max_disparity);

	// 定义AD-Census匹配类实例
	ADCensusStereo ad_census;

	printf("AD-Census Initializing...\n");
	auto start = steady_clock::now();
	//···············································································//
	// 初始化
	if (!ad_census.Initialize(width, height, ad_option)) {
		std::cout << "AD-Census初始化失败！" << std::endl;
		return -2;
	}
	auto end = steady_clock::now();
	auto tt = duration_cast<milliseconds>(end - start);
	printf("AD-Census Initializing Done! Timing :	%lf s\n\n", tt.count() / 1000.0);

	printf("AD-Census Matching...\n");
	// disparity数组保存子像素的视差结果
	auto disparity = new float32[uint32(width * height)]();

	start = steady_clock::now();
	//···············································································//
	// 匹配
	if (!ad_census.Match(bytes_left, bytes_right, disparity)) {
		std::cout << "AD-Census匹配失败！" << std::endl;
		return -2;
	}
	end = steady_clock::now();
	tt = duration_cast<milliseconds>(end - start);
	printf("\nAD-Census Matching...Done! Timing :	%lf s\n", tt.count() / 1000.0);

	//···············································································//
	// 显示视差图
	ShowDisparityMap(disparity, width, height, "disp-left");
	// 保存视差图
	SaveDisparityMap(disparity, width, height, path_left);
	// 保存视差点云(x, y, disp, r, g, b)
	SaveDisparityCloud(bytes_left, disparity, width, height, path_left);



	//···············································································//
	// 视差图转换为深度图
	const cv::Mat depth_mat = cv::Mat(height, width, CV_8UC1);
	float fx = 4.9250945790423793e+02;//取fx为f，fx由标定内参得到
	float baseline = 40; //基线距离b，根据标定的相机外参计算。如果只需要相对深度取1即可
	disp2DepthMap(disparity, depth_mat, fx, baseline, width, height, "disp-left");
	// 保存深度图
	SaveDepthMap(disparity, fx, baseline, width, height, path_left);
	// 保存深度点云(x, y, z,disp, r, g, b)
	SaveDepthCloud(bytes_left, disparity, fx, baseline, width, height, path_left);

	cv::waitKey(0);

	//···············································································//
	// 释放内存
	delete[] disparity;
	disparity = nullptr;
	delete[] bytes_left;
	bytes_left = nullptr;
	delete[] bytes_right;
	bytes_right = nullptr;

	system("pause");


	return 0;
}

void ShowDisparityMap(const float32* disp_map, const sint32& width, const sint32& height, const std::string& name)
{
	// 显示视差图
	const cv::Mat disp_mat = cv::Mat(height, width, CV_8UC1);
	float32 min_disp = float32(width), max_disp = -float32(width);
	for (sint32 i = 0; i < height; i++) {
		for (sint32 j = 0; j < width; j++) {
			const float32 disp = abs(disp_map[i * width + j]);
			if (disp != Invalid_Float) {
				min_disp = std::min(min_disp, disp);
				max_disp = std::max(max_disp, disp);
			}
		}
	}
	for (sint32 i = 0; i < height; i++) {
		for (sint32 j = 0; j < width; j++) {
			const float32 disp = abs(disp_map[i * width + j]);
			if (disp == Invalid_Float) {
				disp_mat.data[i * width + j] = 0;
			}
			else {
				disp_mat.data[i * width + j] = static_cast<uchar>((disp - min_disp) / (max_disp - min_disp) * 255);
			}
		}
	}

	cv::imshow(name, disp_mat);
	cv::Mat disp_color;
	applyColorMap(disp_mat, disp_color, cv::COLORMAP_JET);
	cv::imshow(name + "-color", disp_color);

}

void SaveDisparityMap(const float32* disp_map, const sint32& width, const sint32& height, const std::string& path)
{
	// 保存视差图
	const cv::Mat disp_mat = cv::Mat(height, width, CV_8UC1);
	float32 min_disp = float32(width), max_disp = -float32(width);
	for (sint32 i = 0; i < height; i++) {
		for (sint32 j = 0; j < width; j++) {
			const float32 disp = abs(disp_map[i * width + j]);
			if (disp != Invalid_Float) {
				min_disp = std::min(min_disp, disp);
				max_disp = std::max(max_disp, disp);
			}
		}
	}
	for (sint32 i = 0; i < height; i++) {
		for (sint32 j = 0; j < width; j++) {
			const float32 disp = abs(disp_map[i * width + j]);
			if (disp == Invalid_Float) {
				disp_mat.data[i * width + j] = 0;
			}
			else {
				disp_mat.data[i * width + j] = static_cast<uchar>((disp - min_disp) / (max_disp - min_disp) * 255);
			}
		}
	}

	cv::imwrite(path + "-disparity.png", disp_mat);
	cv::Mat disp_color;
	applyColorMap(disp_mat, disp_color, cv::COLORMAP_JET);
	cv::imwrite(path + "-disp_color.png", disp_color);
}

void SaveDisparityCloud(const uint8* img_bytes, const float32* disp_map, const sint32& width, const sint32& height, const std::string& path)
{
	// 保存视差点云(x,y,disp,r,g,b)
	FILE* fp_disp_cloud = nullptr;
	fopen_s(&fp_disp_cloud, (path + "-cloud.txt").c_str(), "w");
	if (fp_disp_cloud) {
		for (sint32 i = 0; i < height; i++) {
			for (sint32 j = 0; j < width; j++) {
				const float32 disp = abs(disp_map[i * width + j]);
				if (disp == Invalid_Float) {
					continue;
				}
				fprintf_s(fp_disp_cloud, "%f %f %f %d %d %d\n", float32(j), float32(i),
					disp, img_bytes[i * width * 3 + 3 * j + 2], img_bytes[i * width * 3 + 3 * j + 1], img_bytes[i * width * 3 + 3 * j]);
			}
		}
		fclose(fp_disp_cloud);
	}
}
/*
void disp2Depth(cv::Mat dispMap, cv::Mat &depthMap)
{
	float fx = 4.9250945790423793e+02;//取fx为f，fx由标定内参得到
	float baseline = 600; //基线距离b，根据标定的相机外参计算。如果只需要相对深度取1即可

	int height = dispMap.rows;
	int width = dispMap.cols;
	depthMap.create(height, width, CV_16U);

	//这里深度图的数据类型可能会不一样，大家根据自己的情况进行修改
	short* dispData = (short*)dispMap.data;
	ushort* depthData = (ushort*)depthMap.data;
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			int id = i * width + j;
			if (!dispData[id])  continue;  //防止0除
			depthData[id] = ushort(fx*baseline / dispData[id]);
		}
	}
}
*/

void disp2DepthMap(const float32* disp_map, cv::Mat depth_mat, const float32& fx, const float32& baseline, const sint32& width, const sint32& height, const std::string& name)
{
	// 显示深度图
	//const cv::Mat depth_mat = cv::Mat(height, width, CV_8UC1);
	float32 min_disp = float32(width), max_disp = -float32(width);
	for (sint32 i = 0; i < height; i++) {
		for (sint32 j = 0; j < width; j++) {
			const float32 disp = abs(disp_map[i * width + j]);
			if (disp != Invalid_Float) {
				min_disp = std::min(min_disp, disp);
				max_disp = std::max(max_disp, disp);
			}
		}
	}
	for (sint32 i = 0; i < height; i++) {
		for (sint32 j = 0; j < width; j++) {
			const float32 disp = abs(disp_map[i * width + j]);
			if (disp == Invalid_Float) {
				depth_mat.data[i * width + j] = 0;
			}
			else {
				depth_mat.data[i * width + j] = static_cast<uchar>(fx*baseline / disp);
			}
		}
	}

	cv::imshow(name, depth_mat);
	cv::Mat depth_color;
	applyColorMap(depth_mat, depth_color, cv::COLORMAP_JET);
	cv::imshow(name + "-depth_color", depth_color);

}

void SaveDepthMap(const float32* disp_map, const float32& fx, const float32& baseline, const sint32& width, const sint32& height, const std::string& path)
{
	// 保存深度图
	const cv::Mat depth_mat = cv::Mat(height, width, CV_8UC1);
	float32 min_disp = float32(width), max_disp = -float32(width);
	for (sint32 i = 0; i < height; i++) {
		for (sint32 j = 0; j < width; j++) {
			const float32 disp = abs(disp_map[i * width + j]);
			if (disp != Invalid_Float) {
				min_disp = std::min(min_disp, disp);
				max_disp = std::max(max_disp, disp);
			}
		}
	}
	for (sint32 i = 0; i < height; i++) {
		for (sint32 j = 0; j < width; j++) {
			const float32 disp = abs(disp_map[i * width + j]);
			if (disp == Invalid_Float) {
				depth_mat.data[i * width + j] = 0;
			}
			else {
				//disp_mat.data[i * width + j] = static_cast<uchar>((disp - min_disp) / (max_disp - min_disp) * 255);
				depth_mat.data[i * width + j] = static_cast<uchar>(fx*baseline / disp);
			}
		}
	}

	cv::imwrite(path + "-depth.png", depth_mat);
	cv::Mat depth_color;
	applyColorMap(depth_mat, depth_color, cv::COLORMAP_JET);
	cv::imwrite(path + "-depth_color.png", depth_color);
}

void SaveDepthCloud(const uint8* img_bytes, const float32* disp_map, const float32& fx, const float32& baseline, const sint32& width, const sint32& height, const std::string& path)
{
	// 保存深度点云(x,y,z,disp,r,g,b)

	const cv::Mat depth_mat = cv::Mat(height, width, CV_8UC1);
	FILE* fp_disp_cloud = nullptr;
	fopen_s(&fp_disp_cloud, (path + "-cloud.txt").c_str(), "w");
	if (fp_disp_cloud) {
		for (sint32 i = 0; i < height; i++) {
			for (sint32 j = 0; j < width; j++) {
				const float32 disp = abs(disp_map[i * width + j]);
				if (disp == Invalid_Float) {
					continue;
				}
				else {
					//disp_mat.data[i * width + j] = static_cast<uchar>((disp - min_disp) / (max_disp - min_disp) * 255);
					depth_mat.data[i * width + j] = static_cast<uchar>(fx*baseline / disp);
				}
				fprintf_s(fp_disp_cloud, "%f %f %f %f %d %d %d\n", float32(j), float32(i), depth_mat.data[i * width + j],
					disp, img_bytes[i * width * 3 + 3 * j + 2], img_bytes[i * width * 3 + 3 * j + 1], img_bytes[i * width * 3 + 3 * j]);
			}
		}
		fclose(fp_disp_cloud);
	}
}
