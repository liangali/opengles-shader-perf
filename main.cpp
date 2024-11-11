#include <ANGLE/GLES3/gl3.h>
#include <EGL/egl.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <windows.h>
#include "shaders.h"
#include "texture_utils.h"

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
bool initEGL(HWND window) {
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        std::cerr << "Failed to get EGL display" << std::endl;
        return false;
    }

    EGLint majorVersion, minorVersion;
    if (!eglInitialize(display, &majorVersion, &minorVersion)) {
        std::cerr << "Failed to initialize EGL" << std::endl;
        return false;
    }

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

int main() {
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
    if (!initEGL(hwnd)) {
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