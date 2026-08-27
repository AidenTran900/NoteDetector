#ifndef AUDIO_ANALYZER_HPP
#define AUDIO_ANALYZER_HPP

#include <vector>
#include <string>
#include <complex>

struct AudioData {
    std::vector<double> intensity;
    std::vector<double> intensityInterval;
    std::vector<std::complex<double>> compIntensity;
    std::vector<double> fIntensity;
    std::vector<double> fxAxis;
    std::vector<double> xAxis;
    std::vector<double> xAxisInterval;
    std::vector<double> peaks;
    std::vector<double> plotPeaks;

    int len = 0;
    int intervalLen = 0;
    int sampleRate = 0;
    double duration = 0;
    int numSamples = 0;
    int timeIndex = 0;
    double peakFrequency = 0;
    std::string detectedNote = "";
    bool needsAutoFit = false;
    bool analyzeFullAudio = false;
};

class AudioAnalyzer {
public:
    static std::string FrequencyToNoteName(double frequency);
    static std::vector<double> AnalyzeAudioFile(const std::string& path, int downsampleRate, int* len, int* sampleRate, double* duration, int* numSamples);

    void LoadAudioFile(const std::string& path, int downsampleRate);
    void UpdateData();
    void PeakDetection();
    void ChangeInterval(bool forward);
    void ToggleFullAudio(bool toggle);

    AudioData data;

private:
    int TimeToIndex(double time) const;
};

#endif
