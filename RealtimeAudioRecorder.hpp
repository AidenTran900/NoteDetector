#ifndef REALTIME_AUDIO_RECORDER_HPP
#define REALTIME_AUDIO_RECORDER_HPP

#include "RtAudio.h"
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <complex>

class RealtimeAudioRecorder {
public:
    RealtimeAudioRecorder(unsigned int sampleRate, float recordSeconds);
    ~RealtimeAudioRecorder();

    bool Start();
    void Stop();
    bool IsRecording() const { return isRecording; }

    void UpdateLiveBuffer();
    const std::vector<double>& GetLiveBuffer() const { return liveBuffer; }
    const std::vector<double>& GetFrequencyIntensity() const { return fIntensity; }
    const std::vector<double>& GetFrequencyAxis() const { return fxAxis; }
    double GetPeakFrequency() const { return peakFrequency; }
    std::string GetDetectedNote() const { return detectedNote; }

    unsigned int GetCircularBufferSize() const { return circularBufferSize; }
    unsigned int GetSampleRate() const { return sampleRate; }

private:
    static int AudioCallback(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames,
                             double streamTime, RtAudioStreamStatus status, void* userData);

    void PeakDetection();

    const unsigned int channels = 1;
    const unsigned int sampleRate;
    const unsigned int bufferFrames = 256;
    const unsigned int circularBufferSize;

    std::vector<double> circularBuffer;
    std::atomic<unsigned int> writeIndex;
    std::mutex bufferMutex;
    std::unique_ptr<RtAudio> rtAudio;
    RtAudio::StreamParameters inputParams;
    bool isRecording = false;

    std::vector<double> liveBuffer;
    std::vector<std::complex<double>> compIntensity;
    std::vector<double> fIntensity;
    std::vector<double> fxAxis;
    double peakFrequency = 0.0;
    std::string detectedNote = "";
};

#endif
