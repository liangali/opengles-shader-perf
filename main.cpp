#include <ANGLE/GLES3/gl3.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglext_angle.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <string>
#include <windows.h>
#include "shaders.h"
#include "texture_utils.h"
#include <dxgi1_2.h>
#pragma comment(lib, "dxgi.lib")

// 添加一些可能缺少的 EGL 常量定义
#ifndef EGL_DEVICE_EXT
#define EGL_DEVICE_EXT                     0x322C
#endif

#ifndef EGL_PLATFORM_DEVICE_EXT
#define EGL_PLATFORM_DEVICE_EXT            0x313F
#endif

#ifndef EGL_DEVICE_NAME
#define EGL_DEVICE_NAME                    0x3200
#endif

// 在文件开头添加这些常量定义
#ifndef EGL_PLATFORM_ANGLE_ANGLE
#define EGL_PLATFORM_ANGLE_ANGLE 0x3202
#endif

#ifndef EGL_PLATFORM_ANGLE_TYPE_ANGLE
#define EGL_PLATFORM_ANGLE_TYPE_ANGLE 0x3203
#endif

#ifndef EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE
#define EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE 0x3208
#endif

#ifndef EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE
#define EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE 0x3201
#endif

#ifndef EGL_PLATFORM_ANGLE_DEVICE_ID_HIGH_ANGLE
#define EGL_PLATFORM_ANGLE_DEVICE_ID_HIGH_ANGLE 0x3209
#endif

#ifndef EGL_PLATFORM_ANGLE_DEVICE_ID_LOW_ANGLE
#define EGL_PLATFORM_ANGLE_DEVICE_ID_LOW_ANGLE 0x320A
#endif

// 在其他常量定义后添加
#ifndef EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE
#define EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE 0x320C
#endif

#ifndef EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE
#define EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE 0x320D
#endif

// 在文件开头添加新的常量定义
#ifndef EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE
#define EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE 0x3204
#endif

#ifndef EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE
#define EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE 0x3205
#endif

// GPU related structures and variables
struct GPUInfo {
    EGLDeviceEXT device;
    std::string name;
    std::string vendor;
};

std::vector<GPUInfo> gpuList;

// Function declarations
void queryGPUAdapters();

// Performance test parameters
const int TEST_ITERATIONS = 100;
const int WIDTH = 3840;  // 4K
const int HEIGHT = 2160;

// OpenGL related variables
GLuint shaderProgram;
GLuint VBO, VAO, EBO;
GLuint yTexture, uvTexture;
EGLDisplay display;
EGLSurface surface;
EGLContext context;
GLuint fbo;
GLuint outputTexture;

// Performance test results
struct PerfResult {
    double avgTime;
    double minTime;
    double maxTime;
};

// Initialize EGL
bool initEGL(HWND window, int gpuIndex = 0) {
    display = EGL_NO_DISPLAY;
    
    std::cout << "\n=== EGL Initialization Start ===" << std::endl;
    std::cout << "Requested GPU Index: " << gpuIndex << std::endl;
    
    // 使用 EGLint 而不是 EGLAttrib
    std::vector<EGLint> displayAttributes = {
        // 指定使用 D3D11 后端
        EGL_PLATFORM_ANGLE_TYPE_ANGLE,
        static_cast<EGLint>(EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE),
        
        // 使用 D3D11 Feature Level 11_0
        EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE, 11,
        EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE, 0,
        
        // 请求硬件渲染
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE,
        static_cast<EGLint>(EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE)
    };

    // 如果有选择特定的 GPU
    if (gpuIndex >= 0 && gpuIndex < gpuList.size()) {
        std::cout << "Attempting to select GPU: " << gpuList[gpuIndex].name << std::endl;
        
        IDXGIFactory1* factory = nullptr;
        HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory);
        std::cout << "CreateDXGIFactory1 result: 0x" << std::hex << hr << std::dec << std::endl;
        
        if (SUCCEEDED(hr)) {
            IDXGIAdapter1* adapter = nullptr;
            UINT adapterIndex = 0;
            for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
                if (adapterIndex == gpuIndex) {
                    DXGI_ADAPTER_DESC1 desc;
                    adapter->GetDesc1(&desc);
                    
                    // 添加 LUID 到属性列表
                    displayAttributes.push_back(EGL_PLATFORM_ANGLE_DEVICE_ID_LOW_ANGLE);
                    displayAttributes.push_back(static_cast<EGLint>(desc.AdapterLuid.LowPart));
                    displayAttributes.push_back(EGL_PLATFORM_ANGLE_DEVICE_ID_HIGH_ANGLE);
                    displayAttributes.push_back(static_cast<EGLint>(desc.AdapterLuid.HighPart));
                    
                    std::cout << "Found matching adapter:" << std::endl;
                    std::cout << "  Name: " << gpuList[gpuIndex].name << std::endl;
                    std::cout << "  LUID: High=0x" << std::hex << desc.AdapterLuid.HighPart 
                             << ", Low=0x" << desc.AdapterLuid.LowPart << std::dec << std::endl;
                    
                    // 添加额外的 D3D11 特定属性
                    displayAttributes.push_back(EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE);
                    displayAttributes.push_back(static_cast<EGLint>(EGL_TRUE));
                    break;
                }
                adapter->Release();
                adapterIndex++;
            }
            factory->Release();
        }
    }

    // 添加终止标记
    displayAttributes.push_back(EGL_NONE);

    // 打印所有属性值
    std::cout << "Display attributes:" << std::endl;
    for (size_t i = 0; i < displayAttributes.size() - 1; i += 2) {
        std::cout << "  0x" << std::hex << displayAttributes[i] 
                 << " = 0x" << displayAttributes[i + 1] << std::dec << std::endl;
    }

    // 使用 eglGetPlatformDisplayEXT 创建显示
    PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT =
        (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
    
    std::cout << "eglGetPlatformDisplayEXT function pointer: " 
              << (eglGetPlatformDisplayEXT ? "Valid" : "NULL") << std::endl;
    
    if (eglGetPlatformDisplayEXT) {
        display = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE,
                                         EGL_DEFAULT_DISPLAY,
                                         displayAttributes.data());
        
        if (display == EGL_NO_DISPLAY) {
            EGLint error = eglGetError();
            std::cout << "eglGetPlatformDisplayEXT failed with error: 0x" 
                      << std::hex << error << std::dec << std::endl;
            
            // 如果失败，尝试移除 LUID 相关属性
            if (error == EGL_BAD_ATTRIBUTE) {
                std::cout << "Trying without LUID..." << std::endl;
                // 移除 LUID 相关属性
                displayAttributes = {
                    EGL_PLATFORM_ANGLE_TYPE_ANGLE,
                    static_cast<EGLint>(EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE),
                    EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE, 11,
                    EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE, 0,
                    EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE,
                    static_cast<EGLint>(EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE),
                    EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE,
                    static_cast<EGLint>(EGL_TRUE),
                    EGL_NONE
                };

                // 打印重新构建后的属性值
                std::cout << "Display attributes (without LUID):" << std::endl;
                for (size_t i = 0; i < displayAttributes.size() - 1; i += 2) {
                    std::cout << "  0x" << std::hex << displayAttributes[i]
                              << " = 0x" << displayAttributes[i + 1] << std::dec << std::endl;
                }

                display = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE,
                                                 EGL_DEFAULT_DISPLAY,
                                                 displayAttributes.data());
            }
        } else {
            std::cout << "eglGetPlatformDisplayEXT succeeded" << std::endl;
        }
    }

    // 如果失败，回退到默认方式
    if (display == EGL_NO_DISPLAY) {
        std::cout << "Falling back to eglGetDisplay..." << std::endl;
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }
    
    if (display == EGL_NO_DISPLAY) {
        std::cerr << "Failed to get EGL display" << std::endl;
        return false;
    }

    EGLint majorVersion, minorVersion;
    if (!eglInitialize(display, &majorVersion, &minorVersion)) {
        std::cerr << "Failed to initialize EGL" << std::endl;
        return false;
    }
    std::cout << "EGL Initialized: Version " << majorVersion << "." << minorVersion << std::endl;

    // 获取并打印 EGL 客户端 APIs
    const char* apis = eglQueryString(display, EGL_CLIENT_APIS);
    std::cout << "Supported client APIs: " << (apis ? apis : "Unknown") << std::endl;

    // 获取并打印 EGL 扩展
    const char* extensions = eglQueryString(display, EGL_EXTENSIONS);
    std::cout << "EGL Extensions: " << (extensions ? extensions : "None") << std::endl;

    const EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_NONE
    };

    EGLConfig config;
    EGLint numConfigs;
    if (!eglChooseConfig(display, configAttribs, &config, 1, &numConfigs)) {
        std::cerr << "Failed to choose EGL config" << std::endl;
        return false;
    }

    const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };

    context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    if (context == EGL_NO_CONTEXT) {
        std::cerr << "Failed to create EGL context" << std::endl;
        return false;
    }

    surface = eglCreateWindowSurface(display, config, window, nullptr);
    if (surface == EGL_NO_SURFACE) {
        std::cerr << "Failed to create EGL surface" << std::endl;
        return false;
    }

    if (!eglMakeCurrent(display, surface, surface, context)) {
        std::cerr << "Failed to make EGL context current" << std::endl;
        return false;
    }

    std::cout << "=== EGL Initialization Complete ===\n" << std::endl;
    return true;
}

// Initialize shaders
bool initShaders() {
    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    // Check compilation errors
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Vertex shader compilation failed: " << infoLog << std::endl;
        return false;
    }

    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
        return false;
    }

    // Link shader program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return true;
}


void initFramebuffer() {
    // Create and setup FBO
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Create color attachment texture
    glGenTextures(1, &outputTexture);
    glBindTexture(GL_TEXTURE_2D, outputTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);  // Use RGBA format
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);  // Use NEAREST filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // Attach texture to FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexture, 0);

    // Check if FBO is complete
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer is not complete! Status: " << status << std::endl;
    } else {
        std::cout << "Framebuffer is complete!" << std::endl;
    }

    // Reset binding
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Initialize vertex data and textures
void initGeometryAndTextures() {
    float vertices[] = {
        // Position          // Texture coordinates
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f
    };

    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 删除这部分，因为我们后面会重新创建纹理
    /*
    std::vector<unsigned char> yData(WIDTH * HEIGHT, 128);
    std::vector<unsigned char> uvData(WIDTH * HEIGHT / 2, 128);

    glGenTextures(1, &yTexture);
    glBindTexture(GL_TEXTURE_2D, yTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, WIDTH, HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, yData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenTextures(1, &uvTexture);
    glBindTexture(GL_TEXTURE_2D, uvTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, WIDTH/2, HEIGHT/2, 0, GL_RG, GL_UNSIGNED_BYTE, uvData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    */

    initFramebuffer();
}

// Run performance test
PerfResult runPerfTest() {
    std::vector<double> times;
    times.reserve(TEST_ITERATIONS);

    glUseProgram(shaderProgram);
    
    // Set texture units
    glUniform1i(glGetUniformLocation(shaderProgram, "yTexture"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "uvTexture"), 1);

    // Bind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, WIDTH, HEIGHT);

    // 添加这行
    glClear(GL_COLOR_BUFFER_BIT);

    for (int i = 0; i < TEST_ITERATIONS; i++) {
        auto start = std::chrono::high_resolution_clock::now();

        // Bind textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, yTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, uvTexture);

        // Render
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        // Sync GPU
        glFinish();

        auto end = std::chrono::high_resolution_clock::now();
        double time = std::chrono::duration<double, std::milli>(end - start).count();
        times.push_back(time);
    }

    // Reset FBO binding
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Calculate statistics
    double avgTime = 0;
    for (double time : times) {
        avgTime += time;
    }
    avgTime /= TEST_ITERATIONS;

    double minTime = *std::min_element(times.begin(), times.end());
    double maxTime = *std::max_element(times.begin(), times.end());

    return {avgTime, minTime, maxTime};
}

// Cleanup resources
void cleanup() {
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &outputTexture);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteTextures(1, &yTexture);
    glDeleteTextures(1, &uvTexture);
    glDeleteProgram(shaderProgram);

    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
    eglTerminate(display);
}

// Window procedure function
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void queryGPUAdapters() {
    // 尝试用 DXGI 来枚举 GPU
    IDXGIFactory1* factory = nullptr;
    std::vector<IDXGIAdapter1*> adapters;
    
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory);
    if (SUCCEEDED(hr)) {
        IDXGIAdapter1* adapter = nullptr;
        for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);
            
            GPUInfo info;
            info.device = EGL_NO_DEVICE_EXT;  // 我们不使用 EGL device
            
            // 将 WCHAR 转换为 string
            char name[128];
            wcstombs(name, desc.Description, sizeof(name));
            info.name = name;
            
            // 使用 VendorID 来确定厂商
            switch(desc.VendorId) {
                case 0x10DE: info.vendor = "NVIDIA"; break;
                case 0x1002: info.vendor = "AMD"; break;
                case 0x8086: info.vendor = "Intel"; break;
                default: info.vendor = "Unknown"; break;
            }
            
            gpuList.push_back(info);
            std::cout << "GPU " << gpuList.size()-1 << ": " << info.name 
                     << " (" << info.vendor << ")" << std::endl;
            
            adapter->Release();
        }
        factory->Release();
    }

    // 如果没有找到任何设备，添加一个默认设备
    if (gpuList.empty()) {
        GPUInfo defaultGPU;
        defaultGPU.device = EGL_NO_DEVICE_EXT;
        defaultGPU.name = "Default GPU";
        defaultGPU.vendor = "Default Vendor";
        gpuList.push_back(defaultGPU);
        std::cout << "Using default GPU" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    // 在 main 函数开头添加
    SetEnvironmentVariable("ANGLE_DEFAULT_PLATFORM", "d3d11");
    SetEnvironmentVariable("ANGLE_D3D11_FORCE_DISCRETE_GPU", "1");
    SetEnvironmentVariable("ANGLE_PLATFORM_ANGLE_DEVICE_TYPE", "hardware");
    SetEnvironmentVariable("ANGLE_DEBUG_DISPLAY", "1");
    SetEnvironmentVariable("ANGLE_DEBUG_LAYERS", "1");
    
    int selectedGPU = 0;
    
    // 处理命令行参数
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--gpu" && i + 1 < argc) {
            selectedGPU = std::atoi(argv[i + 1]);
            i++;
        } else if (arg == "--list-gpus") {
            queryGPUAdapters();
            return 0;
        }
    }

    // 查询可用GPU
    queryGPUAdapters();
    
    if (gpuList.empty()) {
        std::cerr << "No GPU adapters found!" << std::endl;
        return -1;
    }
    
    if (selectedGPU >= gpuList.size()) {
        std::cerr << "Selected GPU index " << selectedGPU << " is out of range. "
                 << "Available GPUs: 0-" << (gpuList.size()-1) << std::endl;
        return -1;
    }
    
    std::cout << "Using GPU " << selectedGPU << ": " 
              << gpuList[selectedGPU].name << std::endl;

    // Register window class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = "ShaderPerfTest";
    RegisterClassEx(&wc);

    // Create window
    HWND hwnd = CreateWindowEx(
        0,
        "ShaderPerfTest",
        "Shader Performance Test",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );

    if (!hwnd) {
        std::cerr << "Failed to create window" << std::endl;
        return -1;
    }

    ShowWindow(hwnd, SW_SHOW);

    // Initialize EGL and OpenGL ES
    if (!initEGL(hwnd, selectedGPU)) {
        return -1;
    }

    if (!initShaders()) {
        return -1;
    }

    initGeometryAndTextures();

    // 创建并初始化NV12纹理时使用测试pattern
    uint8_t* nv12_data = new uint8_t[WIDTH * HEIGHT * 3 / 2];
    uint8_t* y_plane = nv12_data;
    uint8_t* uv_plane = nv12_data + (WIDTH * HEIGHT);
    
    FillNV12TestPattern(y_plane, uv_plane, WIDTH, HEIGHT);

    // 创建NV12纹理并上传数据
    glGenTextures(1, &yTexture);
    glBindTexture(GL_TEXTURE_2D, yTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, WIDTH, HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, y_plane);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenTextures(1, &uvTexture);
    glBindTexture(GL_TEXTURE_2D, uvTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, WIDTH/2, HEIGHT/2, 0, GL_RG, GL_UNSIGNED_BYTE, uv_plane);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Initialize FBO
    initFramebuffer();

    // Run performance test
    PerfResult result = runPerfTest();

    // Output results
    std::cout << "Performance Test Results:" << std::endl;
    std::cout << "Average Time: " << result.avgTime << " ms" << std::endl;
    std::cout << "Minimum Time: " << result.minTime << " ms" << std::endl;
    std::cout << "Maximum Time: " << result.maxTime << " ms" << std::endl;

    // Read RGB result
    uint8_t* rgb_data = new uint8_t[WIDTH * HEIGHT * 3];
    
    // Bind FBO and perform rendering
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, WIDTH, HEIGHT);
    
    // Clear buffer to red (for debugging)
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Execute rendering
    glUseProgram(shaderProgram);
    
    // Set texture uniform
    GLint yLoc = glGetUniformLocation(shaderProgram, "yTexture");
    GLint uvLoc = glGetUniformLocation(shaderProgram, "uvTexture");
    glUniform1i(yLoc, 0);  // texture unit 0
    glUniform1i(uvLoc, 1); // texture unit 1
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, yTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, uvTexture);
    
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    // Ensure rendering is complete
    glFinish();

    // Add error checking
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "OpenGL error before reading pixels: 0x" << std::hex << err << std::endl;
    }
    
    // Read pixel data
    uint8_t* rgba_data = new uint8_t[WIDTH * HEIGHT * 4];  // Using RGBA format
    glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, rgba_data);
    
    // Check for errors again
    err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "OpenGL error after reading pixels: 0x" << std::hex << err << std::endl;
    }

    // Convert RGBA to RGB
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        rgb_data[i * 3] = rgba_data[i * 4];      // R
        rgb_data[i * 3 + 1] = rgba_data[i * 4 + 1];  // G
        rgb_data[i * 3 + 2] = rgba_data[i * 4 + 2];  // B
    }

    delete[] rgba_data;  // Clean up RGBA data

    // Check if all pixel values are zero
    bool allZero = true;
    for (int i = 0; i < WIDTH * HEIGHT * 3; i++) {
        if (rgb_data[i] != 0) {
            allZero = false;
            break;
        }
    }
    if (allZero) {
        std::cerr << "Warning: All pixel values are zero!" << std::endl;
    } else {
        std::cout << "Successfully read non-zero pixel values." << std::endl;
    }

    // Save as BMP file
    SaveRGBToBMP("output_test.bmp", rgb_data, WIDTH, HEIGHT);

    // Reset FBO binding
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Clean up
    delete[] nv12_data;
    delete[] rgb_data;

    // Cleanup resources
    cleanup();

    // Close the window and exit
    DestroyWindow(hwnd);
    return 0;
} 