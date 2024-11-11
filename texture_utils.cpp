#include "texture_utils.h"
#include <cmath>  // for sin and cos functions

void FillNV12TestPattern(uint8_t* y_plane, uint8_t* uv_plane, int width, int height) {
    // Y plane: create vertical gradient
    for (int y = 0; y < height; y++) {
        uint8_t value = (uint8_t)((y * 255) / height);
        for (int x = 0; x < width; x++) {
            y_plane[y * width + x] = value;
        }
    }

    // UV plane: create color pattern
    for (int y = 0; y < height/2; y++) {
        for (int x = 0; x < width/2; x++) {
            int index = y * (width/2) + x;
            uv_plane[index * 2] = (uint8_t)(128 + 127 * sin(x * 6.28 / (width/2)));     // U
            uv_plane[index * 2 + 1] = (uint8_t)(128 + 127 * cos(y * 6.28 / (height/2))); // V
        }
    }
}

bool SaveRGBToBMP(const char* filename, uint8_t* rgb_data, int width, int height) {
    FILE* file = fopen(filename, "wb");
    if (!file) return false;

    // BMP file header
    BITMAPFILEHEADER bmp_header = {0};
    BITMAPINFOHEADER bmp_info = {0};
    
    bmp_header.bfType = 0x4D42; // "BM"
    bmp_header.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + width * height * 3;
    bmp_header.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    bmp_info.biSize = sizeof(BITMAPINFOHEADER);
    bmp_info.biWidth = width;
    bmp_info.biHeight = -height; // Negative value means top-down storage
    bmp_info.biPlanes = 1;
    bmp_info.biBitCount = 24;
    bmp_info.biCompression = BI_RGB;

    fwrite(&bmp_header, sizeof(BITMAPFILEHEADER), 1, file);
    fwrite(&bmp_info, sizeof(BITMAPINFOHEADER), 1, file);

    // Write RGB data (BMP needs BGR format)
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
        
        // Each row needs 4-byte alignment
        uint8_t padding[3] = {0};
        fwrite(padding, (4 - (width * 3) % 4) % 4, 1, file);
    }

    fclose(file);
    return true;
} 