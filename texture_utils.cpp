#include "texture_utils.h"

void FillNV12TestPattern(uint8_t* y_plane, uint8_t* uv_plane, int width, int height) {
    // 填充Y平面，值从0开始递增
    uint8_t y_value = 0;
    for (int i = 0; i < width * height; i++) {
        y_plane[i] = y_value++;
    }

    // 填充UV平面，交替填充U和V值
    uint8_t uv_value = 0;
    for (int i = 0; i < (width * height / 2); i++) {
        uv_plane[i] = uv_value++;
    }
}

bool SaveRGBToBMP(const char* filename, uint8_t* rgb_data, int width, int height) {
    FILE* file = fopen(filename, "wb");
    if (!file) return false;

    // BMP文件头
    BITMAPFILEHEADER bmp_header = {0};
    BITMAPINFOHEADER bmp_info = {0};
    
    bmp_header.bfType = 0x4D42; // "BM"
    bmp_header.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + width * height * 3;
    bmp_header.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    bmp_info.biSize = sizeof(BITMAPINFOHEADER);
    bmp_info.biWidth = width;
    bmp_info.biHeight = -height; // 负值表示从上到下存储
    bmp_info.biPlanes = 1;
    bmp_info.biBitCount = 24;
    bmp_info.biCompression = BI_RGB;

    fwrite(&bmp_header, sizeof(BITMAPFILEHEADER), 1, file);
    fwrite(&bmp_info, sizeof(BITMAPINFOHEADER), 1, file);

    // 写入RGB数据（BMP需要BGR格式）
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 3;
            uint8_t temp[3] = {
                rgb_data[idx + 2], // B
                rgb_data[idx + 1], // G
                rgb_data[idx]      // R
            };
            fwrite(temp, 3, 1, file);
        }
        
        // 每行需要4字节对齐
        uint8_t padding[3] = {0};
        fwrite(padding, (4 - (width * 3) % 4) % 4, 1, file);
    }

    fclose(file);
    return true;
} 