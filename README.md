# MPFun - Audio Processing Utilities

## Overview

**MPFun** is a reference demonstrating various techniques required to work with digital audio. These techniques include: reading files, compression, and hopefully other things, soon. 

The **MDCT** (found in dsp.cpp) and **Huffman Coding** (found in huffman.c) algorithms included are very slow, and should not be used by anyone. Ever. The purpose of this is to keep the code readable enough to understand what is going on from a math and DSP perspective, which is not the case in most hyperoptimized modern codecs.

## Currently Includes

- **Reading MP4 Files**: Functions for opening and parsing audio data from MP4 files (video coming soon!).
- **MDCT Transformations**: Utilities for converting PCM samples to the frequency domain using MDCT.
- **(Broken) Huffman Coding**: Example of huffman coding for MP3 Compression.
- **MP3 Compression**: Convert raw PCM audio to an encoded MP3 file.
- **PCM Quantization with Scaling**: A mechanism to scale PCM data before quantization, improving the precision of low-amplitude samples.
- **Audio Effects**: Basic audio effects such as **low-pass filtering** **chorus** and **reverb** to apply effects to audio streams. More effects are under development.
- **Utility Functions**: Includes helper functions for audio manipulation, scaling, and analysis.

## This repository is for educational purposes!