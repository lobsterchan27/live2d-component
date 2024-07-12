Here's a draft README for the Live2D Component project:

# Live2D Component for Banana Client

A Live2D integration component that renders Live2D models and animations for use in video overlays.

## Features

- Renders Live2D models using the Cubism SDK
- Supports model loading, animations, and expressions
- Integrates with audio for lip sync
- Generates video output with FFmpeg
- Cross-platform support (Windows)

## Installation

1. Clone the repository:
   ```
   git clone https://github.com/your-repo/live2d-component.git
   ```

2. Install dependencies:
   - CMake (3.16 or higher)
   - Visual Studio 2019 or 2022 with C++ development tools
   - FFmpeg

3. Build the project:
   ```
   cd live2d-component
   mkdir build && cd build
   cmake ..
   cmake --build . --config Release
   ```

## Usage

Run the compiled executable with the following command-line arguments:

```
live2d.exe -a <audio_file> -v <video_file> [-o <output_path>]
```

- `-a`: Path to the input audio file
- `-v`: Path to the input video file 
- `-o`: (Optional) Path for the output video file. Defaults to "output" if not specified.

Example:
```
live2d.exe -a input_audio.wav -v input_video.mp4 -o output_video.mp4
```

This will render the Live2D model animation synchronized with the audio, overlay it on the input video, and save the result as `output_video.mp4`.

## Configuration

Model settings can be adjusted in the `LAppDefine.hpp` file. This includes render target dimensions, motion groups, and other parameters.

## Contributing

1. Fork the repository
2. Create a new branch for your feature
3. Commit your changes
4. Push to your branch
5. Create a new Pull Request

Please ensure your code follows the existing style and includes appropriate tests.

## License

This project uses the Live2D Cubism SDK, which is licensed under the Live2D Open Software License.

By using this software, you agree to the terms of the [Live2D Open Software License](https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html).

The Live2D Cubism Components are subject to the license terms and conditions set forth by Live2D Inc. For more details, please refer to the official Live2D website and the license agreement included with the SDK.

All rights to the Live2D Cubism SDK belong to Live2D Inc. This project is not endorsed or certified by Live2D Inc.

## Acknowledgements

- [Live2D Cubism SDK](https://www.live2d.com/en/download/cubism-sdk/)
- [FFmpeg](https://ffmpeg.org/)
- [GLFW](https://www.glfw.org/)
- [GLEW](http://glew.sourceforge.net/)
- [stb](https://github.com/nothings/stb)

## Notes

This component is designed for integration with the Banana client. It provides a framework for rendering Live2D models and generating video output, but may require additional configuration or customization for specific use cases.
