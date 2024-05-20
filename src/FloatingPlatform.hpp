#pragma once

#include "LAppWavFileHandler.hpp"

class FloatingPlatform {
public:
    static FloatingPlatform* getInstance();

    static void releaseInstance();

    void setWavFileHandler(LAppWavFileHandler* handler);

    LAppWavFileHandler* getWavFileHandler();

private:
    FloatingPlatform();

    ~FloatingPlatform();

    LAppWavFileHandler* _wavFileHandler;
    
};