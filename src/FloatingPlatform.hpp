#pragma once

#include "LAppWavFileHandler.hpp"
#include <string>

class FloatingPlatform
{
public:
    static FloatingPlatform *getInstance();

    static void releaseInstance();

    void setWavFileHandler(LAppWavFileHandler *handler);

    LAppWavFileHandler *getWavFileHandler();

    void setAudioFilePath(const std::string &path);
    std::string getAudioFilePath() const;

    void setVideoFilePath(const std::string &path);
    std::string getVideoFilePath() const;

    void setOutputPath(const std::string &path);
    std::string getOutputPath() const;

    void setElapsedSeconds(float* elapsedSeconds);

    float* getElapsedSeconds() const;

private:
    FloatingPlatform();

    ~FloatingPlatform();

    LAppWavFileHandler *_wavFileHandler;

    float* _elapsedSeconds;

    // Member variables to store CLI arguments
    std::string _audioFilePath;
    std::string _videoFilePath;
    std::string _outputPath;
};