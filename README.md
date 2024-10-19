# MPFun - Audio Processing Utilities

## Overview

**MPFun** is a library designed for audio processing, specifically targeting operations involving **PCM (Pulse Code Modulation)** data, **MDCT (Modified Discrete Cosine Transform)**, and **quantization** of audio samples. This repository includes utilities and tools for handling transformations, scaling, and quantization, with the aim of preserving low-amplitude details during the compression process.

## Includes

- **Reading MP4 Files**: Functions for opening and parsing audio data from MP4 files (video coming soon!).
- **MDCT Transformations**: Utilities for converting PCM samples to the frequency domain using MDCT.
- **PCM Quantization with Scaling**: A mechanism to scale PCM data before quantization, improving the precision of low-amplitude samples.
- **Inverse MDCT (IMDCT)**: Functions for converting frequency-domain data back to the time domain.
- **Audio Effects**: Basic audio effects such as **low-pass filtering** **chorus** and **reverb** to apply effects to audio streams. More effects are under development.
- **Utility Functions**: Includes helper functions for audio manipulation, scaling, and analysis.

## Installation

1. Clone the repository:

   git clone https://github.com/ArnavMehrotra/mpfun.git
   cd mpfun

2. Build (requires FFMpeg on a Unix based system):
    make

3. Enjoy!

**Feel free to use any source code or huffman tables to write your own codec.**