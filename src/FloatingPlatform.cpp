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

FloatingPlatform::FloatingPlatform()
    : _wavFileHandler(nullptr) {
}

FloatingPlatform::~FloatingPlatform() {
    if (_wavFileHandler != nullptr) {
        delete _wavFileHandler;
        _wavFileHandler = nullptr;
    }
}