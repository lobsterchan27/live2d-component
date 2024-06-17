#include "FloatingPlatform.hpp"
#include "LAppWavFileHandler.hpp"

namespace {
    FloatingPlatform* s_instance = nullptr;
}

FloatingPlatform* FloatingPlatform::getInstance() {
    if (s_instance == nullptr) {
        s_instance = new FloatingPlatform();
    }

    return s_instance;
}

void FloatingPlatform::releaseInstance() {
    if (s_instance != nullptr) {
        delete s_instance;
    }

    s_instance = nullptr;
}

void FloatingPlatform::setWavFileHandler(LAppWavFileHandler* handler) {
    _wavFileHandler = handler;
}

LAppWavFileHandler* FloatingPlatform::getWavFileHandler() {
    return _wavFileHandler;
}

void FloatingPlatform::setAudioFilePath(const std::string& path) {
    _audioFilePath = path;
}

std::string FloatingPlatform::getAudioFilePath() const {
    return _audioFilePath;
}

void FloatingPlatform::setVideoFilePath(const std::string& path) {
    _videoFilePath = path;
}

std::string FloatingPlatform::getVideoFilePath() const {
    return _videoFilePath;
}

void FloatingPlatform::setOutputPath(const std::string& path) {
    _outputPath = path;
}

std::string FloatingPlatform::getOutputPath() const {
    return _outputPath;
}

void FloatingPlatform::setElapsedSeconds(float* elapsedSeconds) {
    _elapsedSeconds = elapsedSeconds;
}

float* FloatingPlatform::getElapsedSeconds() const {
    return _elapsedSeconds;
}

FloatingPlatform::FloatingPlatform()
    : _wavFileHandler(nullptr) {
}

FloatingPlatform::~FloatingPlatform() {
    
}