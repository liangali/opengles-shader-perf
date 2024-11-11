#pragma once

#include <cstdint>  // for uint8_t
#include <windows.h> // for BITMAPFILEHEADER, BITMAPINFOHEADER
#include <cstdio>   // for FILE operations

// 用于生成测试pattern的函数
void FillNV12TestPattern(uint8_t* y_plane, uint8_t* uv_plane, int width, int height);

// 保存RGB数据为BMP文件
bool SaveRGBToBMP(const char* filename, uint8_t* rgb_data, int width, int height); 