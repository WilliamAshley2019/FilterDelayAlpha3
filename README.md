Delay Filter Alpha 3 Plugin

I still need to make a readme for this it is GPL 3 Juce and VST3 license terms. I will update these when I am done with this concept.

Note that to hear this plugin well put mix at position 1.0 which is fully wet, vs the default 0.5 (mixed wet/dry).
Delay Filter Alpha 3 Plugin
![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)
![JUCE](https://img.shields.io/badge/JUCE-8.0.9-green)
![VST3](https://img.shields.io/badge/VST3-3.7.0-orange)

A versatile multi-mode audio filter plugin built with JUCE 8.0.9, emulating classic filter behaviors using delay-based interference principles. Inspired by signal processing concepts like comb filtering, FIR/IIR designs, and phase modulation, it offers adjustable frequency targeting across modes.This plugin is designed for VST3 hosts and supports stereo I/O. It's perfect for sound design, mixing, and experimental audio processing.FeaturesMulti-Mode Filtering:Comb: Delay-based notches and peaks for metallic/resonant tones.
FIR: Multi-tap feedforward with Hann windowing for smooth, linear-phase filtering.
IIR: Biquad-based (Low-pass, High-pass, Band-pass) with Q/resonance control.
Phaser: All-pass stages with frequency-dependent modulation for sweeping notches.
Flanger: Modulated delay comb for dynamic sweeps.

Frequency Tuning: Unified "Filter Freq" knob (20 Hz–20 kHz) targets the core response in each mode (e.g., cutoff for IIR, notch spacing for Comb/FIR).
Modulation: LFO rate/depth for Phaser/Flanger sweeps.
Mix & Feedback: Blend dry/wet and add resonance/echo.
Smoothing: Parameter changes are smoothed to prevent zipper noise.
Linear Interpolation: Smooth delay reads for artifact-free processing.

UsageFilter Type: Select mode from dropdown.
Filter Freq: Tune the target frequency (Hz) for the filter's response.
IIR-Specific (in IIR mode): Choose type (Low/High/Band-pass) and adjust Q for sharpness.
Delay-Based Modes (Comb/FIR/Flanger): Use Delay (ms) for base timing; Filter Freq auto-adjusts spacing.
Modulation: Enable LFO Rate/Depth for Phaser/Flanger.
Mix: 0% = dry (bypass), 100% = full effect.

Pro Tip: For precise targeting, use a frequency analyzer in your DAW to sweep and visualize notches/peaks.

BuildingThis project uses JUCE Projucer (v8.0.9). Requirements:JUCE 8.0.9   pluginbasics + dsp module.

LicenseThis project is licensed under the GNU General Public License v3.0 (GPLv3). See LICENSE for full text.Third-Party LicensesJUCE Framework (GPLv3 Compatible)This software uses JUCE (version 8.0.9), a free, open-source C++ framework for cross-platform audio applications.JUCE License Notice (excerpt):Copyright (C) 2017 - Raw Material Software Limited.JUCE is provided under the terms of the ISC license:
Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies.THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
Full JUCE license: JUCE/modules/juce_core/native/juce_licence.h.VST3 SDK (Steinberg License)This plugin uses the VST3 SDK (version 3.7.0) from Steinberg Media Technologies GmbH.VST3 SDK License Notice (excerpt):Copyright (C) 2023 Steinberg Media Technologies GmbH. All Rights Reserved.The VST 3 Software Development Kit (VST 3 SDK) is solely intended for use by developers for the purpose of creating plug-ins and host applications according to the Steinberg VST 3 specification.The VST 3 SDK is provided by Steinberg Media Technologies GmbH on an "AS IS" basis without warranty of any kind, either expressed or implied.
Redistribution of the VST3 SDK is not permitted. For full terms, see VST3_SDK/License.txt (included in the repo).

Built on October 06, 2025.


Future Intentions:
Get better CPU function.
Consider modernizing the GUI.
