#include "RealtimeAudioRecorder.hpp"
#include "AudioAnalyzer.hpp"
#include "fourier.hpp"
#include <iostream>
#include <cmath>

const double PI = acos(-1);

RealtimeAudioRecorder::RealtimeAudioRecorder(unsigned int sampleRate, float recordSeconds)
    : sampleRate(sampleRate)
    , circularBufferSize(static_cast<unsigned int>(sampleRate * recordSeconds * channels))
    , circularBuffer(circularBufferSize, 0.0)
    , writeIndex(0) {
    fxAxis.resize(circularBufferSize / 2);
    for (unsigned int i = 0; i < circularBufferSize / 2; ++i) {
        fxAxis[i] = sampleRate * (static_cast<double>(i) / circularBufferSize);
    }
}

RealtimeAudioRecorder::~RealtimeAudioRecorder() {
    Stop();
}

int RealtimeAudioRecorder::AudioCallback(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames,
                                         double /*streamTime*/, RtAudioStreamStatus status, void* userData) {
    if (status) std::cerr << "Stream underflow/overflow detected.\n";
    if (!inputBuffer) return 0;

    RealtimeAudioRecorder* recorder = static_cast<RealtimeAudioRecorder*>(userData);
    double* in = static_cast<double*>(inputBuffer);

    for (unsigned int i = 0; i < nBufferFrames; ++i) {
        unsigned int index = (recorder->writeIndex + i) % recorder->circularBufferSize;
        recorder->circularBuffer[index] = in[i];
    }
    recorder->writeIndex = (recorder->writeIndex + nBufferFrames) % recorder->circularBufferSize;

    return 0;
}

bool RealtimeAudioRecorder::Start() {
    rtAudio = std::make_unique<RtAudio>();

    if (rtAudio->getDeviceCount() < 1) {
        std::cerr << "No audio devices found!" << std::endl;
        return false;
    }

    inputParams.deviceId = rtAudio->getDefaultInputDevice();
    inputParams.nChannels = channels;
    inputParams.firstChannel = 0;

    unsigned int bufferFramesCopy = bufferFrames;
    RtAudioErrorType err;

    err = rtAudio->openStream(nullptr, &inputParams, RTAUDIO_FLOAT64,
                              sampleRate, &bufferFramesCopy,
                              &AudioCallback, this);
    if (err != RTAUDIO_NO_ERROR) {
        std::cerr << "Error opening stream: " << rtAudio->getErrorText() << std::endl;
        return false;
    }

    err = rtAudio->startStream();
    if (err != RTAUDIO_NO_ERROR) {
        std::cerr << "Error starting stream: " << rtAudio->getErrorText() << std::endl;
        return false;
    }

    isRecording = true;
    std::cout << "Started real-time recording" << std::endl;
    return true;
}

void RealtimeAudioRecorder::Stop() {
    if (!rtAudio || !isRecording) return;

    if (rtAudio->isStreamRunning()) {
        rtAudio->stopStream();
    }
    if (rtAudio->isStreamOpen()) {
        rtAudio->closeStream();
    }

    isRecording = false;
    liveBuffer.clear();
    std::cout << "Stopped recording" << std::endl;
}

void RealtimeAudioRecorder::UpdateLiveBuffer() {
    liveBuffer.resize(circularBufferSize);
    for (unsigned int i = 0; i < circularBufferSize; ++i) {
        unsigned int index = (writeIndex + i) % circularBufferSize;
        liveBuffer[i] = circularBuffer[index];
    }

    compIntensity.resize(circularBufferSize);
    for (unsigned int i = 0; i < circularBufferSize; ++i) {
        double hanning = 0.5 * (1 - std::cos(2 * PI * i / (circularBufferSize - 1)));
        compIntensity[i] = std::complex<double>(liveBuffer[i] * hanning, 0.0);
    }
    FFT(compIntensity);

    fIntensity.resize(circularBufferSize / 2);
    for (unsigned int i = 0; i < circularBufferSize / 2; ++i) {
        fIntensity[i] = std::norm(compIntensity[i]);
    }

    PeakDetection();
}

void RealtimeAudioRecorder::PeakDetection() {
    double maxIntensity = 0.0;
    peakFrequency = 0.0;
    for (size_t i = 1; i < fIntensity.size(); ++i) {
        if (fIntensity[i] > maxIntensity) {
            maxIntensity = fIntensity[i];
            peakFrequency = fxAxis[i];
        }
    }
    detectedNote = AudioAnalyzer::FrequencyToNoteName(peakFrequency);
}
