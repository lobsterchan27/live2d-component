/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppDelegate.hpp"
#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "LAppView.hpp"
#include "LAppPal.hpp"
#include "LAppDefine.hpp"
#include "LAppLive2DManager.hpp"
#include "LAppTextureManager.hpp"
#include <Rendering/OpenGL/CubismOffscreenSurface_OpenGLES2.hpp>

#include "FloatingPlatform.hpp"
#include <cstdio>
#include <iostream>
#include <string>
#include <windows.h>
#include <fstream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace Csm;
using namespace std;
using namespace LAppDefine;

namespace FFmpegUtils
{
    FILE *startFFmpegProcess(
        csmUint32 width,
        csmUint32 height,
        const std::string &audioFilePath,
        const std::string &outputPath)
    {
        // std::string command = "ffmpeg -y -f rawvideo -vcodec rawvideo -s " + std::to_string(width) + "x" + std::to_string(height)
        //                     + " -r 30 -pix_fmt rgba -i - -i \"" + audioFilePath + "\" -c:v libx264 -preset medium -qp 0 "
        //                     + "-c:a copy output.mp4";

        std::string command = "ffmpeg -y -f rawvideo -vcodec rawvideo -s " + std::to_string(width) + "x" + std::to_string(height)
                + " -r 30 -pix_fmt rgba -i - -i \"" + audioFilePath + "\" -c:v prores_ks -profile:v 4444 "
                + "-pix_fmt yuva444p10le -c:a copy \"" + outputPath + ".mov\"";


        FILE *ffmpeg = _popen(command.c_str(), "wb");

        if (!ffmpeg)
        {
            std::cerr << "Failed to start FFmpeg process.\n";
            return nullptr;
        }
        return ffmpeg;
    }

    void sendFrameToFFmpeg(FILE *ffmpeg, unsigned char *data, int size)
    {
        if (ffmpeg)
        {
            fwrite(data, 1, size, ffmpeg);
            fflush(ffmpeg);
        }
    }

    void closeFFmpegProcess(FILE *ffmpeg)
    {
        if (ffmpeg)
        {
            _pclose(ffmpeg);
        }
    }
}

namespace
{
    int imageCount = 0;
    LAppDelegate *s_instance = NULL;
    GLuint pbos[2] = {0, 0};
    int currentPBO = 0;
    int nextPBO = 1; // Use two PBOs to switch between them

    void initializePBOs(csmUint32 width, csmUint32 height)
    {
        glGenBuffers(2, pbos);
        for (int i = 0; i < 2; ++i)
        {
            glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos[i]);
            glBufferData(GL_PIXEL_PACK_BUFFER, width * height * 4, nullptr, GL_STREAM_READ);
            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        }
    }

    void cleanupPBOs()
    {
        glDeleteBuffers(2, pbos);
        pbos[0] = 0;
        pbos[1] = 0;
    }

    void saveImageFromPBO(const char *filename, GLubyte *ptr, int width, int height)
    {
        stbi_write_png(filename, width, height, 4, ptr, width * 4);
    }
}

LAppDelegate *LAppDelegate::GetInstance()
{
    if (s_instance == NULL)
    {
        s_instance = new LAppDelegate();
    }

    return s_instance;
}

void LAppDelegate::ReleaseInstance()
{
    if (s_instance != NULL)
    {
        delete s_instance;
    }

    s_instance = NULL;
}

bool LAppDelegate::Initialize(
    const std::string audioFilePath,
    const std::string outputPath)
{
    if (DebugLogEnable)
    {
        LAppPal::PrintLogLn("START");
    }

    _audioFilePath = audioFilePath;
    _outputPath = outputPath;

    // GLFWの初期化
    if (glfwInit() == GL_FALSE)
    {
        if (DebugLogEnable)
        {
            LAppPal::PrintLogLn("Can't initilize GLFW");
        }
        return GL_FALSE;
    }

    // Windowの生成_
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    _window = glfwCreateWindow(RenderTargetWidth, RenderTargetHeight, "Live2D ROX", NULL, NULL);
    if (_window == NULL)
    {
        if (DebugLogEnable)
        {
            LAppPal::PrintLogLn("Can't create GLFW window.");
        }
        glfwTerminate();
        return GL_FALSE;
    }

    // Windowのコンテキストをカレントに設定
    glfwMakeContextCurrent(_window);

    // Disable this for the completed implementation. from 1 to 0 disables
    glfwSwapInterval(0);

    if (glewInit() != GLEW_OK)
    {
        if (DebugLogEnable)
        {
            LAppPal::PrintLogLn("Can't initilize glew.");
        }
        glfwTerminate();
        return GL_FALSE;
    }

    // テクスチャサンプリング設定
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // 透過設定
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // コールバック関数の登録
    //  glfwSetMouseButtonCallback(_window, EventHandler::OnMouseCallBack);
    //  glfwSetCursorPosCallback(_window, EventHandler::OnMouseCallBack);

    // ウィンドウサイズ記憶
    // int width, height;
    // glfwGetWindowSize(LAppDelegate::GetInstance()->GetWindow(), &width, &height);
    _windowWidth = RenderTargetWidth;
    _windowHeight = RenderTargetHeight;

    _finalFbo = new Csm::Rendering::CubismOffscreenSurface_OpenGLES2();
    _finalFbo->CreateOffscreenSurface(RenderTargetWidth, RenderTargetHeight);

    initializePBOs(RenderTargetWidth, RenderTargetHeight);

    // AppViewの初期化
    _view->Initialize();

    // Cubism SDK の初期化
    InitializeCubism();

    return GL_TRUE;
}

void LAppDelegate::Release()
{
    // Windowの削除

    cleanupPBOs();

    delete _finalFbo;

    glfwDestroyWindow(_window);

    glfwTerminate();

    delete _textureManager;
    delete _view;
    delete _wavFileHandler;

    // リソースを解放
    LAppLive2DManager::ReleaseInstance();

    // Cubism SDK の解放
    CubismFramework::Dispose();
}

void printWorkingDirectory()
{
    const DWORD buffSize = MAX_PATH;
    char buffer[buffSize];
    if (GetCurrentDirectory(buffSize, buffer))
    {
        std::cout << "Current working directory is: " << buffer << std::endl;
    }
    else
    {
        std::cout << "Error getting current working directory" << std::endl;
    }
}

void LAppDelegate::Run()
{
    // initial parameters
    double targetFPS = 30.0;
    double deltaTime = 1.0 / targetFPS;

    _wavFileHandler->Start(_audioFilePath.c_str());
    const LAppWavFileHandler::WavFileInfo &fileInfo = _wavFileHandler->GetWavFileInfo();
    _wavFileHandler->ComputeMaxRMS(deltaTime);

    float audioDurationSeconds = static_cast<float>(fileInfo._samplesPerChannel) / fileInfo._samplingRate;


    initializePBOs(RenderTargetWidth, RenderTargetHeight); // Assume this function initializes PBOs

    // Open FFmpeg's stdin in binary write mode
    FILE *ffmpeg = FFmpegUtils::startFFmpegProcess(RenderTargetWidth, RenderTargetHeight, _audioFilePath, _outputPath);

    // Main loop
    while (glfwWindowShouldClose(_window) == GL_FALSE && !_isEnd && elapsedSeconds < audioDurationSeconds)
    {


        LAppPal::UpdateTimeFPS(targetFPS);
        csmFloat32 deltaTime = LAppPal::GetDeltaTime();
        elapsedSeconds += deltaTime;

        // Draw to FBO
        _finalFbo->BeginDraw();
        glClearDepth(1.0);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        _view->Render();
        _finalFbo->EndDraw();

        // Bind PBO and read pixels from FBO
        glBindFramebuffer(GL_READ_FRAMEBUFFER, _finalFbo->GetRenderTexture());
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos[currentPBO]); // Use the correct PBO id

        // Adding alignment checks and stride calculation
        glPixelStorei(GL_PACK_ALIGNMENT, 1); // Ensure there is no padding at the end of each pixel row
        glReadPixels(0, 0, RenderTargetWidth, RenderTargetHeight, GL_RGBA, GL_UNSIGNED_BYTE, 0);

        GLubyte *ptr = (GLubyte *)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);

        if (ptr)
        {
            size_t numBytes = RenderTargetWidth * 4; // RGBA
            for (int y = RenderTargetHeight - 1; y >= 0; --y)
            {
                GLubyte *rowPtr = ptr + y * numBytes;
                fwrite(rowPtr, 1, numBytes, ffmpeg); // Write directly to FFmpeg's stdin
            }
            fflush(ffmpeg); // Ensure data is immediately sent out

            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        }

        std::swap(currentPBO, nextPBO); // Swap the PBOs

    }

    FFmpegUtils::closeFFmpegProcess(ffmpeg);
    // Cleanup
    Release();
    LAppDelegate::ReleaseInstance();
}

LAppDelegate::LAppDelegate() : _cubismOption(),
                               _window(NULL),
                               _captured(false),
                               _mouseX(0.0f),
                               _mouseY(0.0f),
                               _isEnd(false),
                               _windowWidth(0),
                               _windowHeight(0)
{
    _view = new LAppView();
    _textureManager = new LAppTextureManager();
    _wavFileHandler = new LAppWavFileHandler();
    FloatingPlatform::getInstance()->setWavFileHandler(_wavFileHandler);
}

LAppDelegate::~LAppDelegate()
{
}

void LAppDelegate::InitializeCubism()
{
    // setup cubism
    _cubismOption.LogFunction = LAppPal::PrintMessage;
    _cubismOption.LoggingLevel = LAppDefine::CubismLoggingLevel;
    Csm::CubismFramework::StartUp(&_cubismAllocator, &_cubismOption);

    // Initialize cubism
    CubismFramework::Initialize();

    // load model
    LAppLive2DManager::GetInstance();

    // default proj #lobby
    // this is NOT being used for whatever reason
    CubismMatrix44 projection;

    LAppPal::UpdateTime();

    _view->InitializeSprite();
}

void LAppDelegate::OnMouseCallBack(GLFWwindow *window, int button, int action, int modify)
{
    if (_view == NULL)
    {
        return;
    }
    if (GLFW_MOUSE_BUTTON_LEFT != button)
    {
        return;
    }

    if (GLFW_PRESS == action)
    {
        _captured = true;
        _view->OnTouchesBegan(_mouseX, _mouseY);
    }
    else if (GLFW_RELEASE == action)
    {
        if (_captured)
        {
            _captured = false;
            _view->OnTouchesEnded(_mouseX, _mouseY);
        }
    }
}

void LAppDelegate::OnMouseCallBack(GLFWwindow *window, double x, double y)
{
    _mouseX = static_cast<float>(x);
    _mouseY = static_cast<float>(y);

    if (!_captured)
    {
        return;
    }
    if (_view == NULL)
    {
        return;
    }

    _view->OnTouchesMoved(_mouseX, _mouseY);
}

GLuint LAppDelegate::CreateShader()
{
    // バーテックスシェーダのコンパイル
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    const char *vertexShader =
        "#version 120\n"
        "attribute vec3 position;"
        "attribute vec2 uv;"
        "varying vec2 vuv;"
        "void main(void){"
        "    gl_Position = vec4(position, 1.0);"
        "    vuv = uv;"
        "}";
    glShaderSource(vertexShaderId, 1, &vertexShader, NULL);
    glCompileShader(vertexShaderId);
    if (!CheckShader(vertexShaderId))
    {
        return 0;
    }

    // フラグメントシェーダのコンパイル
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
    const char *fragmentShader =
        "#version 120\n"
        "varying vec2 vuv;"
        "uniform sampler2D texture;"
        "uniform vec4 baseColor;"
        "void main(void){"
        "    gl_FragColor = texture2D(texture, vuv) * baseColor;"
        "}";
    glShaderSource(fragmentShaderId, 1, &fragmentShader, NULL);
    glCompileShader(fragmentShaderId);
    if (!CheckShader(fragmentShaderId))
    {
        return 0;
    }

    // プログラムオブジェクトの作成
    GLuint programId = glCreateProgram();
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    // リンク
    glLinkProgram(programId);

    glUseProgram(programId);

    return programId;
}

bool LAppDelegate::CheckShader(GLuint shaderId)
{
    GLint status;
    GLint logLength;
    glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar *log = reinterpret_cast<GLchar *>(CSM_MALLOC(logLength));
        glGetShaderInfoLog(shaderId, logLength, &logLength, log);
        CubismLogError("Shader compile log: %s", log);
        CSM_FREE(log);
    }

    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        glDeleteShader(shaderId);
        return false;
    }

    return true;
}
