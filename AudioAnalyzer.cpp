#include "AudioAnalyzer.hpp"
#include "AudioFile.h"
#include "fourierTrans/fourier.hpp"
#include "peakDetector.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>

const double PI = acos(-1);

std::string AudioAnalyzer::FrequencyToNoteName(double frequency) {
    if (frequency <= 0) return "N/A";
    static const std::string noteNames[] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    double A4 = 440.0;
    int midiNumber = int(std::round(12 * std::log2(frequency / A4))) + 69;
    int octave = midiNumber / 12 - 1;
    int noteIndex = midiNumber % 12;
    if (midiNumber < 0 || midiNumber > 127) return "Out of Range";
    return noteNames[noteIndex] + std::to_string(octave);
}

std::vector<double> AudioAnalyzer::AnalyzeAudioFile(const std::string& path, int downsampleRate, int* len, int* sampleRate, double* duration, int* numSamples) {
    std::cout << "**********************" << std::endl;
    std::cout << "Running Example: Load Audio File and Print Summary" << std::endl;
    std::cout << "**********************" << std::endl;
    AudioFile<double> audioFile;
    if (!audioFile.load(path)) {
        std::cerr << "Failed to load file: " << path << std::endl;
        *len = 0;
        return {};
    }
    std::cout << "Bit Depth: " << audioFile.getBitDepth() << std::endl;
    std::cout << "Sample Rate: " << audioFile.getSampleRate() << std::endl;
    std::cout << "Num Channels: " << audioFile.getNumChannels() << std::endl;
    std::cout << "Length in Seconds: " << audioFile.getLengthInSeconds() << std::endl << std::endl;
    int channel = 0;
    const auto& samples = audioFile.samples[channel];
    *numSamples = audioFile.getNumSamplesPerChannel();
    *len = *numSamples / downsampleRate;
    *sampleRate = audioFile.getSampleRate();
    *duration = audioFile.getLengthInSeconds();
    std::vector<double> intensity(*len);
    for (int i = 0; i < *len; ++i) {
        intensity[i] = samples[i * downsampleRate];
    }
    return intensity;
}

void AudioAnalyzer::LoadAudioFile(const std::string& path, int downsampleRate) {
    data.intensity = AnalyzeAudioFile(path, downsampleRate, &data.len, &data.sampleRate, &data.duration, &data.numSamples);
    data.timeIndex = 0;

    PeakDetection();

    std::vector<double> detectedPeaks = get_wave_info(data.intensity).peaks;
    data.peaks.resize(detectedPeaks.size());
    data.plotPeaks.resize(detectedPeaks.size());
    for (size_t i = 0; i < detectedPeaks.size(); ++i) {
        data.peaks[i] = detectedPeaks[i] / static_cast<double>(data.sampleRate);
        data.plotPeaks[i] = (detectedPeaks[i] * downsampleRate) / static_cast<double>(data.sampleRate);
    }

    if (data.peaks.size() >= 2) {
        data.intervalLen = TimeToIndex(data.peaks[1]) - TimeToIndex(data.peaks[0]);
    } else {
        data.intervalLen = data.len;
    }

    data.intensityInterval.resize(data.intervalLen);
    int startIdx = TimeToIndex(data.peaks[data.timeIndex]);
    for (int i = 0; i < data.intervalLen; ++i) {
        data.intensityInterval[i] = data.intensity[startIdx + i];
    }

    UpdateData();
}

int AudioAnalyzer::TimeToIndex(double time) const {
    return static_cast<int>(time * data.sampleRate);
}

void AudioAnalyzer::UpdateData() {
    if (data.peaks.empty()) return;

    int startIdx = TimeToIndex(data.peaks[data.timeIndex]);
    if (data.analyzeFullAudio) {
        startIdx = 0;
        data.intervalLen = data.len;
    }

    data.compIntensity.resize(data.intervalLen);
    for (int i = 0; i < data.intervalLen; ++i) {
        double hanning = 0.5 * (1 - std::cos(2 * PI * i / (data.intervalLen - 1)));
        data.compIntensity[i] = std::complex<double>(data.intensity[startIdx + i] * hanning, 0.0);
    }
    FFT(data.compIntensity);

    int effectiveSampleRate = data.sampleRate / 10;

    data.xAxis.resize(data.len);
    for (int i = 0; i < data.len; ++i) {
        data.xAxis[i] = i / static_cast<double>(effectiveSampleRate);
    }

    data.xAxisInterval.resize(data.intervalLen);
    for (int i = 0; i < data.intervalLen; ++i) {
        data.xAxisInterval[i] = (startIdx + i) / static_cast<double>(effectiveSampleRate);
    }

    data.fxAxis.resize(data.intervalLen / 2);
    for (int i = 0; i < data.intervalLen / 2; ++i) {
        data.fxAxis[i] = effectiveSampleRate * (static_cast<double>(i) / data.intervalLen);
    }

    data.fIntensity.resize(data.intervalLen / 2);
    for (int i = 0; i < data.intervalLen / 2; ++i) {
        data.fIntensity[i] = std::norm(data.compIntensity[i]);
    }
}

void AudioAnalyzer::PeakDetection() {
    double maxIntensity = 0.0;
    data.peakFrequency = 0.0;
    for (int i = 1; i < static_cast<int>(data.fIntensity.size()); ++i) {
        if (data.fIntensity[i] > maxIntensity) {
            maxIntensity = data.fIntensity[i];
            data.peakFrequency = data.fxAxis[i];
        }
    }
    data.detectedNote = FrequencyToNoteName(data.peakFrequency);
}

void AudioAnalyzer::ChangeInterval(bool forward) {
    int increment = forward ? 1 : -1;
    data.timeIndex = std::clamp(data.timeIndex + increment, 0, static_cast<int>(data.peaks.size()) - 1);

    if (data.timeIndex + 1 < static_cast<int>(data.peaks.size())) {
        data.intervalLen = TimeToIndex(data.peaks[data.timeIndex + 1]) - TimeToIndex(data.peaks[data.timeIndex]);
    } else {
        data.intervalLen = data.len - TimeToIndex(data.peaks[data.timeIndex]);
    }

    data.intensityInterval.resize(data.intervalLen);
    int startIdx = TimeToIndex(data.peaks[data.timeIndex]);
    for (int i = 0; i < data.intervalLen; ++i) {
        data.intensityInterval[i] = data.intensity[startIdx + i];
    }

    UpdateData();
    PeakDetection();

    data.needsAutoFit = true;
}

void AudioAnalyzer::ToggleFullAudio(bool toggle) {
    if (toggle) {
        data.intervalLen = data.len;
        data.timeIndex = 0;
        UpdateData();
        PeakDetection();
    } else {
        ChangeInterval(true);
    }
}
