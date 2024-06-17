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

#include "LAppModel.hpp"

#include "FloatingPlatform.hpp"
#include <cstdio>
#include <string>
// #include <windows.h>
#include <fstream>
#include <sstream>

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
        const string &audioFilePath,
        const string &existingVideoPath,
        const string &outputPath)
    {
        // Construct the FFmpeg command to overlay the RGBA frames onto the existing video
        string ResourcesPathStr(ResourcesPath);
        string widthStr = std::to_string(width);
        string heightStr = std::to_string(height);

        string portraitImagePath = ResourcesPathStr + "static-portrait.png";
        string landscapeImagePath = ResourcesPathStr + "static-landscape.png";
        string staticImagePath = width > height ? landscapeImagePath : portraitImagePath;

        csmFloat32 correctedVideoADuration = min(VideoADuration, VideoVDuration);
        csmFloat32 remainingDuration = AudioDuration - correctedVideoADuration + postDuration;
        csmFloat32 crossfadeDuration = 2.0f;

        string VideoFPSStr = to_string(VideoFPS);
        string timebase = "1/" + VideoFPSStr;
        string gopSizeStr = to_string(VideoFPS/2);
        string postDurationStr = to_string(postDuration);

        string remainingDurationStr = std::to_string(remainingDuration);
        string correctedVideoADurationStr = std::to_string(correctedVideoADuration);

        string crossfadeDurationStr = std::to_string(crossfadeDuration);
        string crossfadeAndRemainingDuration = std::to_string(crossfadeDuration + remainingDuration);

        string bgmPath = ResourcesPathStr + "bgm.mp3";

        string scaleFilter =
            "[0:v]scale='if(gt(a," + widthStr + "/" + heightStr + ")," + heightStr + "*a," + widthStr + ")'"
            ":'if(gt(a," + widthStr + "/" + heightStr + ")," + heightStr + "," + widthStr + "/a)':"
            "flags=lanczos,crop=" + widthStr + ":" + heightStr + ","
            "trim=duration=" + crossfadeAndRemainingDuration + ","
            "fps=" + VideoFPSStr + ","
            "settb=expr=" + timebase + ",format=yuv420p,setsar=1[scaled_img];";

        string crossfadeFilter =
            "[video_trimmed][scaled_img]xfade=transition=fade:duration=" + std::to_string(crossfadeDuration) +
            ":offset=" + std::to_string(correctedVideoADuration - crossfadeDuration) + "[extended_video];";

        string equalizer = "";
            "equalizer=f=120:width_type=q:width=1.2:g=3.0,"
            "equalizer=f=650:width_type=q:width=1:g=-3.5,"
            "equalizer=f=5000:width_type=q:width=1.2:g=3.0,"
            "equalizer=f=8000:width_type=q:width=15:g=-12,"
            "equalizer=f=16000:width_type=q:width=1:g=3,";

        string deesser =
            "deesser=i=0.5:f=0.5:m=0.5:s=o,"
            "deesser=i=0.4:f=0.2:m=0.2:s=o,"
            "deesser=i=0.36:f=0:m=0:s=o,"
            "deesser=i=0.37:f=0:m=0:s=o,"
            "deesser=i=0.37:f=0:m=0:s=o,"
            "deesser=i=0.37:f=0:m=0:s=o,";

        string compressor = 
            "";

        string voice =
            "[3:a]afftdn=nf=-80:tn=1:tr=1:om=o,"
            "agate=threshold=-35dB:attack=20:release=100:ratio=2,"
            "highpass=f=50:width_type=o:width=2," + deesser + equalizer + 
            "atrim=start=0.025,pan=stereo|c0<c0|c1<c0[voice];";

        string command =
            string("ffmpeg -y") +
            " -loop 1 -t " + crossfadeAndRemainingDuration + " -i " + staticImagePath + // Loop the static image
            " -f rawvideo -vcodec rawvideo -video_size " + widthStr + "x" + heightStr +
            " -r " + VideoFPSStr + " -pix_fmt rgba -thread_queue_size 1024 -i -" + // Raw video input
            " -hwaccel cuda -i \"" + existingVideoPath + "\"" +                                  // Existing video file
            " -i \"" + audioFilePath + "\"" +                                      // Audio file
            " -i \"" + bgmPath + "\"" +                                            // Background music
            " -filter_complex \"" +
            scaleFilter +
            "[2:v]trim=duration=" + correctedVideoADurationStr + ",fps=fps=" + VideoFPSStr + ",settb=expr=" + timebase + "[video_trimmed];" + // Trim the existing video
            crossfadeFilter +                                                                         // Crossfade video and static image
            "[extended_video][1:v]overlay=format=yuv420[outv];" +                                     // Overlay raw video onto the extended base video
            "[2:a]atrim=0:" + correctedVideoADurationStr + "[audio1];" +                              // Trim audio file to corrected duration
            "[4:a]atrim=0:" + crossfadeAndRemainingDuration + ",volume=0.15" + "[audio2];" +           // Trim BGM to remaining duration
            "[audio1][audio2]acrossfade=d=" + crossfadeDurationStr + "[extended_audio];" +            // Concat video audio and BGM audio
            voice +                                                                                   // Noise Reduction
            "[voice]asplit=2[mix_voice][duck_voice];"
            "[duck_voice]apad=pad_dur=" + postDurationStr + "[duck_voice_pad];"                                                 // ffmpeg can't reuse the same audio stream for multiple filters
            "[extended_audio][duck_voice_pad]sidechaincompress=threshold=0.05:ratio=2:attack=4:knee=3:release=1000[ducked_audio];"
            "[mix_voice][ducked_audio]amix=inputs=2:duration=longest:dropout_transition=0.5[outa]\"" + // Mix the audio streams
            " -map \"[outv]\"" +                                                                      // Map the video output
            " -map \"[outa]\"" +                                                                      // Map the audio output
            " -c:v h264_nvenc -cq 20 -pix_fmt yuv420p -bf 2 -preset slow -profile:v high -g " + gopSizeStr + " -r " + VideoFPSStr + // Video codec settings
            " -c:a aac -b:a 384k" +                                                                   // Audio codec settings
            // " -shortest" +
            " -f mp4 \"" + outputPath + ".mp4" + "\"";

        // For the transparency model only
        // std::string command = "ffmpeg -y -f rawvideo -vcodec rawvideo -s " + std::to_string(width) + "x" + std::to_string(height) + " -r 30 -pix_fmt rgba -i - -i \"" + audioFilePath + "\" -c:v prores_ks -profile:v 4444 " + "-pix_fmt yuva444p10le -c:a copy \"" + outputPath + ".mov\"";

        // Video for Thumbs
        // std::string command = "ffmpeg -y -f rawvideo -vcodec rawvideo -s " + std::to_string(width) + "x" + std::to_string(height) + 
        //                   " -r 30 -pix_fmt rgba -i - -c:v prores_ks -profile:v 4444 " +
        //                   "-pix_fmt yuva444p10le \"" + outputPath + ".mov\"";
        
        // Thumbs
        // std::string outputFolder = "output_images";
        // std::string command = "ffmpeg -y -f rawvideo -vcodec rawvideo -s " + std::to_string(width) + "x" + std::to_string(height) + 
        //               " -pix_fmt rgba -i - \"" + outputFolder + "/" + outputPath + "_frame%04d.png\"";

        // Open a pipe to the FFmpeg process
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

    void getVideoInfo(const std::string &videoPath)
    {
        std::string command = "ffprobe -v error -show_entries stream=codec_type,width,height,r_frame_rate,duration "
                              "-of ini \"" +
                              videoPath + "\"";

        FILE *pipe = _popen(command.c_str(), "r");
        if (!pipe)
        {
            std::cerr << "Failed to start ffprobe process.\n";
            return;
        }

        char buffer[128];
        std::string output;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            output += buffer;
        }
        _pclose(pipe);

        std::cout << "FFprobe output: " << output << std::endl;

        // Parse the ini output
        std::istringstream stream(output);
        std::string line;
        std::string currentCodecType;

        while (std::getline(stream, line))
        {
            if (line.empty() || line[0] == '#')
            {
                continue; // Skip empty lines and comments
            }
            else if (line[0] == '[' && line[line.size() - 1] == ']')
            {
                // Section header
                currentCodecType.clear();
            }
            else
            {
                auto delimiterPos = line.find('=');
                if (delimiterPos != std::string::npos)
                {
                    std::string key = line.substr(0, delimiterPos);
                    std::string value = line.substr(delimiterPos + 1);

                    if (key == "codec_type")
                    {
                        currentCodecType = value;
                    }
                    else if (currentCodecType == "video")
                    {
                        if (key == "width")
                        {
                            RenderTargetWidth = std::stoi(value);
                        }
                        else if (key == "height")
                        {
                            RenderTargetHeight = std::stoi(value);
                        }
                        else if (key == "r_frame_rate")
                        {
                            int num = std::stoi(value.substr(0, value.find('/')));
                            int den = std::stoi(value.substr(value.find('/') + 1));
                            VideoFPS = static_cast<float>(num) / den;
                        }
                        else if (key == "duration")
                        {
                            VideoVDuration = std::stof(value);
                        }
                    }
                    else if (currentCodecType == "audio")
                    {
                        if (key == "duration")
                        {
                            VideoADuration = std::stof(value);
                        }
                    }
                }
            }
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

bool LAppDelegate::Initialize()
{
    if (DebugLogEnable)
    {
        LAppPal::PrintLogLn("START");
    }

    FloatingPlatform *platform = FloatingPlatform::getInstance();
    _audioFilePath = platform->getAudioFilePath();
    _videoFilePath = platform->getVideoFilePath();
    _outputPath = platform->getOutputPath();

    platform->setElapsedSeconds(&elapsedSeconds);

    FFmpegUtils::getVideoInfo(_videoFilePath);
    // RenderTargetHeight = 1920;
    // RenderTargetWidth = 1080;
    // cout << "Video resolution: " << RenderTargetWidth << "x" << RenderTargetHeight << endl;
    // cout << "Video duration: " << VideoVDuration << " seconds" << endl;
    // cout << "Audio duration: " << VideoADuration << " seconds" << endl;

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
    _window = glfwCreateWindow(1, 1, "Live2D ROX", NULL, NULL);
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
    int width, height;
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

// void printWorkingDirectory()
// {
//     const DWORD buffSize = MAX_PATH;
//     char buffer[buffSize];
//     if (GetCurrentDirectoryA(buffSize, buffer))
//     {
//         std::cout << "Current working directory is: " << buffer << std::endl;
//     }
//     else
//     {
//         std::cout << "Error getting current working directory" << std::endl;
//     }
// }

void LAppDelegate::Run()
{
    // initial parameters
    double targetFPS = VideoFPS;
    float deltaTimeInterval = 1.0 / targetFPS;

    _wavFileHandler->Start(_audioFilePath.c_str());
    const LAppWavFileHandler::WavFileInfo &fileInfo = _wavFileHandler->GetWavFileInfo();
    _wavFileHandler->ComputeMaxRMS(deltaTimeInterval);

    AudioDuration = static_cast<float>(fileInfo._samplesPerChannel) / fileInfo._samplingRate;

    initializePBOs(RenderTargetWidth, RenderTargetHeight); // Assume this function initializes PBOs

    // Open FFmpeg's stdin in binary write mode
    FILE *ffmpeg = FFmpegUtils::startFFmpegProcess(RenderTargetWidth, RenderTargetHeight, _audioFilePath, _videoFilePath, _outputPath);

    // Main loop
    _finalFbo->BeginDraw();
    // while (glfwWindowShouldClose(_window) == GL_FALSE && !_isEnd)
    // {
    while (glfwWindowShouldClose(_window) == GL_FALSE && !_isEnd && elapsedSeconds < (AudioDuration + postDuration))
    {
        LAppPal::UpdateTimeFPS(targetFPS);
        deltaTime = LAppPal::GetDeltaTime();
        elapsedSeconds += deltaTime;
        // cout << "elapsed: " << elapsedSeconds << "rms value: " << FloatingPlatform::getInstance()->getWavFileHandler()->GetRms(true) << endl;

        glViewport(0, 0, RenderTargetWidth, RenderTargetHeight);
        // Draw to FBO
        // _finalFbo->BeginDraw();
        glClearDepth(1.0);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        _view->Render();
        // _finalFbo->EndDraw();

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
    _finalFbo->EndDraw();
    FFmpegUtils::closeFFmpegProcess(ffmpeg);
    // Cleanup
    Release();
    FloatingPlatform::releaseInstance();
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
    LAppLive2DManager *live2DManager = LAppLive2DManager::GetInstance();


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