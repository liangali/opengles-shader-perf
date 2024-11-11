#pragma once

#include <cstdint>  // for uint8_t
#include <windows.h> // for BITMAPFILEHEADER, BITMAPINFOHEADER
#include <cstdio>   // for FILE operations

// Function to generate test pattern
void FillNV12TestPattern(uint8_t* y_plane, uint8_t* uv_plane, int width, int height);

// Save RGB data to BMP file
bool SaveRGBToBMP(const char* filename, uint8_t* rgb_data, int width, int height);