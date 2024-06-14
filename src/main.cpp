/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppDelegate.hpp"
#include "FloatingPlatform.hpp"
#include "LAppDefine.hpp"
#include <iostream>
#include <string>
#include <algorithm>
#include <windows.h>
#include <vector>
#include <sstream>

std::string GetExecutableDirectory() {
    char path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);
    std::string exePath(path);
    return exePath.substr(0, exePath.find_last_of("\\/"));
}

std::string GetCurrentWorkingDirectory() {
    char cwd[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, cwd);
    return std::string(cwd);
}

std::vector<std::string> SplitPath(const std::string& path) {
    std::vector<std::string> parts;
    std::stringstream pathStream(path);
    std::string part;
    while (getline(pathStream, part, '\\')) {
        if (!part.empty())
            parts.push_back(part);
    }
    return parts;
}

std::string ComputeRelativePath(const std::string& from, const std::string& to) {
    std::vector<std::string> fromParts = SplitPath(from);
    std::vector<std::string> toParts = SplitPath(to);
    auto fromIt = fromParts.begin();
    auto toIt = toParts.begin();

    // Skip common parts
    while (fromIt != fromParts.end() && toIt != toParts.end() && *fromIt == *toIt) {
        ++fromIt;
        ++toIt;
    }

    // Create the relative path
    std::string relativePath;
    for (; fromIt != fromParts.end(); ++fromIt) {
        relativePath += "..\\";
    }
    for (; toIt != toParts.end(); ++toIt) {
        relativePath += *toIt + '\\';
    }

    // Remove the last backslash
    if (!relativePath.empty() && relativePath.back() == '\\') {
        relativePath.pop_back();
    }

    return relativePath.empty() ? "." : relativePath;
}

// Function to find a command line argument and return its value
std::string getCmdOption(char **begin, char **end, const std::string &option)
{
    char **itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return std::string(*itr);
    }
    return "";
}

// Function to check if a command line option exists
bool cmdOptionExists(char **begin, char **end, const std::string &option)
{
    return std::find(begin, end, option) != end;
}

int main(int argc, char *argv[])
{
    // Check if the minimum required arguments are provided
    if (argc < 5)
    {
        std::cerr << "Usage: " << argv[0] << " -a <audiofile> -v <videofile> [-o <outputpath>]" << std::endl;
        return 1; // Exit if the required arguments are not provided
    }

    // Parse the command line arguments
    std::string exeDir = GetExecutableDirectory();
    std::string cwd = GetCurrentWorkingDirectory();
    std::cout << "cwd: " << cwd << std::endl;
    std::cout << "exeDir: " << exeDir << std::endl;
    std::string resourcesDir = GetExecutableDirectory() + "\\Resources/";
    std::string relativePath = ComputeRelativePath(cwd, resourcesDir);

    LAppDefine::ResourcesPath = new char[relativePath.size() + 1];
    strcpy(LAppDefine::ResourcesPath, relativePath.c_str());
    
    std::cout << "Resources path: " << LAppDefine::ResourcesPath << std::endl;

    std::string audioFilePath = getCmdOption(argv, argv + argc, "-a");
    std::string videoFilePath = getCmdOption(argv, argv + argc, "-v");
    std::string outputPath = getCmdOption(argv, argv + argc, "-o");

    std::cout << "Audio file path: " << audioFilePath << std::endl;
    std::cout << "Video file path: " << videoFilePath << std::endl;
    std::cout << "Output path: " << outputPath << std::endl;

    if (audioFilePath.empty() || videoFilePath.empty())
    {
        std::cerr << "Error: Audio file and video file paths are required." << std::endl;
        return 1;
    }

    if (outputPath.empty())
    {
        outputPath = "output"; // Set default output path if not provided
    }

    // Get the singleton instance of FloatingPlatform
    FloatingPlatform *platform = FloatingPlatform::getInstance();
    platform->setAudioFilePath(audioFilePath);
    platform->setVideoFilePath(videoFilePath);
    platform->setOutputPath(outputPath);

    if (LAppDelegate::GetInstance()->Initialize() == GL_FALSE)
    {
        FloatingPlatform::releaseInstance();
        LAppDelegate::ReleaseInstance();
        return 1;
    }

    // Run the application
    LAppDelegate::GetInstance()->Run();

    return 0;
}