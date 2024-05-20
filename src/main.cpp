/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppDelegate.hpp"
#include <iostream>
#include <string>
#include <algorithm>

// Function to find a command line argument
std::string getCmdOption(char** begin, char** end, const std::string& option) {
    char** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end) {
        return std::string(*itr);
    }
    return "";
}

// Function to check if a command line option exists
bool cmdOptionExists(char** begin, char** end, const std::string& option) {
    return std::find(begin, end, option) != end;
}

int main(int argc, char* argv[]) {
    // Check if the audio file argument is provided
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <audiofile> [outputpath]" << std::endl;
        return 1; // Exit if the audio file is not provided
    }

    std::string audioFilePath = argv[1]; // The first argument is the audio file path
    std::string outputPath = argc > 2 ? argv[2] : "default_output_path/"; // Optional output path

    // Initialize the application with the audio file path
    if (LAppDelegate::GetInstance()->Initialize(audioFilePath, outputPath) == GL_FALSE) {
        std::cerr << "Failed to initialize application with audio file: " << audioFilePath << std::endl;
        return 1;
    }

    // Run the application
    LAppDelegate::GetInstance()->Run();

    return 0;
}