 
# Smoke Simulation for Real-time Music Visualisation

## Overview

This project features an interactive 2D smoke simulation that runs in real-time and
integrates with a live audio analyser to produce a music visualisation. The simulator
is implemented on the GPU to maximise performance. Audio input devices can be analysed
to visualise live music. An FFT function is used to generate the frequency distribution
of the audio streams. The resulting visual effect produced can be used by visual DJs to
visualise music in a live setting such as a concert or musical performance. The effect
can also be used by artists to aid in the creation of music videos for prerecorded audio.

Hosted on Github here: https://github.com/EmperorJack/smokey-bbq

## Technical Report

Link goes here.

## Demos

Progress demonstation video - https://www.youtube.com/watch?v=FYY-ArCmbB8

## Libraries

- OpenGL 3.3 / GLEW
- GLFW - Window management
- GLM - Vector and matrix math
- Portaudio - Low-level audio
- KissFFT - FFT function
- Imgui - User interfaces

## Usage

#### Building

CMake files provided. The implementation has been tested on:
- Windows (MinGW / Visual Studio)
- Linux
- Mac (LLVM - Clang)

#### User Interfaces

Three user interface windows are provided. They can be shown or hidden with the following
key bindings:
- Composition Manager GUI - "C" key
- Smoke Simulation GUI - "S" key
- Audio Analyser GUI - "A" key

#### Compositions

Compositions define an integration between the smoke simulation and audio analyser to
produce a visualisation. The current composition can be switched in the composition
manager GUI. The following compositions are provided:
- Horizontal Spectrum
- Circular Spectrum
- RGB Spectrum

#### Video Settings

At the moment the video settings can only be adjusted in `main.hpp` by configuring the 
`FULL_SCREEN`, `BORDERLESS`, `VSYNC`, `SCREEN_WIDTH` and `SCREEN_HEIGHT` declarations.

#### Smoke Simulation Settings

The grid resolution can be adjusted in `smoke_simulation.hpp` by configuring the
`GRID_SIZE` constant. The GPU implementation is enabled by default, if you get
stuck you can switch to the CPU implementation by using the "G" key.

#### Audio Analyser Settings

The sample rate, sample size and number of frequency bands can be adjusted in
`audio_analyser.hpp` by configuring the `SAMPLE_RATE`, `SAMPLE_SIZE` and `NUM_BANDS`
constants.

## Live Audio

The audio analyser works by analysing a specified input device such as a microphone.
This means you can plug in any audio feed into your microphone jack. The input device can
be selected in the audio analyser GUI. If you want to visualise the output sound of your 
PC then you can use one of the following methods based on your OS to route that audio to
an input device:
- Windows - Stereo mix
- Linux - PulseAudio
- Mac - Soundflower
