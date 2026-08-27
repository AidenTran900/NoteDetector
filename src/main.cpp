#include "gui.hpp"
#include "audio/AudioAnalyzer.hpp"
#include "audio/RealtimeAudioRecorder.hpp"
#include "filedialog/tinyfiledialogs.h"
#include "util/imageLoader.hpp"
#include "imgui.h"
#include "implot.h"
#include "piano/imgui_piano.h"
#include "RtAudio.h"
#include <iostream>
#include <map>

const int DOWNSAMPLE_RATE = 10;
const unsigned int RECORD_SAMPLE_RATE = 41000 / DOWNSAMPLE_RATE;
const float REALTIME_SECONDS = 2.0f;

static bool KeyPressed[128] = {};
static ImGuiExt::Piano::keyCode_t prevNoteActive = ImGuiExt::Piano::keyCodeNone;
static ImGuiExt::Piano::keyCode_t detectedNoteIndex = ImGuiExt::Piano::keyCodeNone;

AudioAnalyzer audioAnalyzer;
RealtimeAudioRecorder* realtimeRecorder = nullptr;
bool useRealtimeAudio = false;

const char* filters[] = { "*.wav", "*.aiff", "*.jpg", "*.*" };

void ListAudioDevices() {
    RtAudio audio;
    unsigned int deviceCount = audio.getDeviceCount();
    std::cout << "Devices: " << std::endl;

    for (unsigned int i = 0; i < deviceCount; ++i) {
        RtAudio::DeviceInfo info = audio.getDeviceInfo(i);
        std::cout << "Device " << i << ": " << info.name << std::endl;
        std::cout << "Input Channels: " << info.inputChannels << std::endl;
        std::cout << "Output Channels: " << info.outputChannels << std::endl;
        std::cout << "Sample Rates: ";
        for (auto rate : info.sampleRates) {
            std::cout << rate << " ";
        }
        std::cout << std::endl;
    }
}

int NoteNameToKeyIndex(const std::string& note) {
    static std::map<std::string, int> noteMap = {
        {"C", 0}, {"C#", 1}, {"D", 2}, {"D#", 3},
        {"E", 4}, {"F", 5}, {"F#", 6}, {"G", 7},
        {"G#", 8}, {"A", 9}, {"A#", 10}, {"B", 11}
    };

    if (note.length() < 2 || note.length() > 3)
        return -1;

    std::string name = note.substr(0, note.length() - 1);
    int octave = note.back() - '0';

    if (noteMap.find(name) == noteMap.end() || octave < 0 || octave > 9)
        return -1;

    return (octave + 1) * 12 + noteMap[name];
}

bool SimplePianoCallback(void* userData, ImGuiExt::Piano::KeyboardMsgType msg, ImGuiExt::Piano::keyCode_t keyCode, float velocity) {
    switch (msg) {
        case ImGuiExt::Piano::KeyboardMsgType::NoteGetStatus:
            return keyCode == detectedNoteIndex;
        case ImGuiExt::Piano::KeyboardMsgType::NoteOn:
        case ImGuiExt::Piano::KeyboardMsgType::NoteOff:
            return true;
    }
    return false;
}

void LoadRigby(int len, const std::string& detectedNote) {
    int imgWidth = 0, imgHeight = 0;
    std::string selectedImage = "assets/images/rigby.jpg";
    std::string message = "";
    if (len > 0) {
        message = "Me Rigby cat says \n this note is: " + detectedNote;
        selectedImage = "assets/images/rigbyResult.jpg";
    }
    GLuint myImageTex = LoadTextureFromFile(selectedImage.c_str(), &imgWidth, &imgHeight);
    ImGui::Begin("The Almighty Rigby", nullptr);
    ImVec2 imagePos = ImGui::GetCursorScreenPos();
    ImVec2 imageSize = ImVec2(imgWidth, imgHeight);
    if (myImageTex) {
        ImGui::Image((void*)(intptr_t)myImageTex, imageSize);
    } else {
        ImGui::Text("Failed to load image.");
    }
    ImGui::SetNextWindowSize(imageSize);
    ImVec2 textPos = ImVec2(imagePos.x + 20, imagePos.y + 30);
    float fontSize = 32.0f;
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddText(ImGui::GetFont(), fontSize, ImVec2(textPos.x + 1, textPos.y + 1), IM_COL32(0, 0, 0, 255), message.c_str());
    drawList->AddText(ImGui::GetFont(), fontSize, textPos, IM_COL32(255, 255, 0, 255), message.c_str());
    ImGui::End();
}

void OnImport(const char* file) {
    std::cout << "Imported file: " << file << std::endl;
    audioAnalyzer.LoadAudioFile(file, DOWNSAMPLE_RATE);
}

int main() {
    ListAudioDevices();
    ImPlot::CreateContext();
    GLFWwindow* window = CreateWindowIMGUI(1200, 800, 0, 1);
    if (!window) {
        std::cerr << "Failed to create window!" << std::endl;
        return 0;
    }

    while (!glfwWindowShouldClose(window)) {
        UpdateWindowSize(window);

        ImGui::Begin("Main Window", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::Text("Select An Audio File!");
        if (ImGui::Button("Import File")) {
            const char* file = tinyfd_openFileDialog("Select a file", "", 4, filters, "All files", 0);
            if (file) {
                OnImport(file);
            }
        }

        ImGui::Checkbox("Use Real-Time Audio", &useRealtimeAudio);

        if (useRealtimeAudio && !realtimeRecorder) {
            realtimeRecorder = new RealtimeAudioRecorder(RECORD_SAMPLE_RATE, REALTIME_SECONDS);
            realtimeRecorder->Start();
        } else if (!useRealtimeAudio && realtimeRecorder) {
            delete realtimeRecorder;
            realtimeRecorder = nullptr;
        }

        if (audioAnalyzer.data.len > 0) {
            if (ImGui::Checkbox("Analyze Full Audio", &audioAnalyzer.data.analyzeFullAudio)) {
                std::cout << "CLICKED" << std::endl;
            }

            if (ImGui::Button("Backward")) {
                audioAnalyzer.ChangeInterval(false);
            }
            if (ImGui::Button("Forward")) {
                audioAnalyzer.ChangeInterval(true);
            }

            int noteIndex = NoteNameToKeyIndex(audioAnalyzer.data.detectedNote);
            if (noteIndex >= 0 && noteIndex <= 127) {
                detectedNoteIndex = static_cast<ImGuiExt::Piano::keyCode_t>(noteIndex);
                ImGuiExt::Piano::Keyboard("MyPiano", ImVec2(900, 100), &prevNoteActive, 0, 127, SimplePianoCallback);
            }
            ImGui::Text("Peak Frequency: %.2f Hz", audioAnalyzer.data.peakFrequency);

            ImGui::Separator();

            std::string sampleStr = std::to_string(audioAnalyzer.data.sampleRate);
            std::string waveformTitle = "Waveform Sampled At " + sampleStr + "Hz";
            std::string waveformTitle2 = "Waveform Interval Sampled At " + sampleStr + "Hz";
            std::string spectrumTitle = "Periodogram";

            if (realtimeRecorder && realtimeRecorder->IsRecording()) {
                realtimeRecorder->UpdateLiveBuffer();
                const auto& liveBuffer = realtimeRecorder->GetLiveBuffer();

                std::vector<double> timeAxis(liveBuffer.size());
                for (size_t i = 0; i < liveBuffer.size(); ++i) {
                    timeAxis[i] = i / static_cast<double>(RECORD_SAMPLE_RATE);
                }

                if (ImPlot::BeginPlot("Real-Time Audio")) {
                    ImPlot::SetupAxes("Time (s)", "Amplitude");
                    ImPlot::PlotLine("Live", timeAxis.data(), liveBuffer.data(), liveBuffer.size());
                    ImPlot::EndPlot();
                }
                if (ImPlot::BeginPlot(spectrumTitle.c_str())) {
                    ImPlot::SetupAxes("Frequency (Hz)", "Intensity");
                    const auto& fIntensity = realtimeRecorder->GetFrequencyIntensity();
                    const auto& fxAxis = realtimeRecorder->GetFrequencyAxis();
                    ImPlot::PlotLine("FFT", fxAxis.data(), fIntensity.data(), realtimeRecorder->GetCircularBufferSize() / 2);
                    ImPlot::EndPlot();
                }

                int noteIndex = NoteNameToKeyIndex(realtimeRecorder->GetDetectedNote());
                if (noteIndex >= 0 && noteIndex <= 127) {
                    detectedNoteIndex = static_cast<ImGuiExt::Piano::keyCode_t>(noteIndex);
                }
            } else {
                audioAnalyzer.UpdateData();
                audioAnalyzer.PeakDetection();

                if (ImPlot::BeginPlot(waveformTitle.c_str())) {
                    ImPlot::SetupAxes("Time (Seconds)", "Amplitude");
                    if (audioAnalyzer.data.analyzeFullAudio) {
                        ImPlot::PlotLine("Signal", audioAnalyzer.data.xAxis.data(), audioAnalyzer.data.intensity.data(), audioAnalyzer.data.len);
                    } else {
                        ImPlot::PlotLine("Signal", audioAnalyzer.data.xAxis.data(), audioAnalyzer.data.intensity.data(), audioAnalyzer.data.len);
                        ImPlot::PlotInfLines("Peaks", audioAnalyzer.data.plotPeaks.data(), static_cast<int>(audioAnalyzer.data.peaks.size()));
                        if (!audioAnalyzer.data.peaks.empty()) {
                            double startTime = audioAnalyzer.data.plotPeaks[audioAnalyzer.data.timeIndex];
                            double endTime = (audioAnalyzer.data.timeIndex + 1 < static_cast<int>(audioAnalyzer.data.plotPeaks.size()))
                                ? audioAnalyzer.data.plotPeaks[audioAnalyzer.data.timeIndex + 1]
                                : audioAnalyzer.data.duration;

                            std::vector<double> intervalFillX = { startTime, endTime };
                            std::vector<double> intervalFillLow = { -1.0, -1.0 };
                            std::vector<double> intervalFillHigh = { 1.0, 1.0 };

                            ImPlot::PushStyleColor(ImPlotCol_Fill, IM_COL32(255, 255, 0, 50));
                            ImPlot::PlotShaded("Current Interval", intervalFillX.data(), intervalFillLow.data(), intervalFillHigh.data(), 2);
                            ImPlot::PopStyleColor();
                        }
                    }

                    ImPlot::EndPlot();
                }
                if (audioAnalyzer.data.needsAutoFit) {
                    ImPlot::SetNextAxesToFit();
                    audioAnalyzer.data.needsAutoFit = false;
                }
                if (!audioAnalyzer.data.analyzeFullAudio && ImPlot::BeginPlot(waveformTitle2.c_str())) {
                    ImPlot::SetupAxes("Time (Seconds)", "Amplitude");
                    ImPlot::PlotLine("Signal", audioAnalyzer.data.xAxisInterval.data(), audioAnalyzer.data.intensityInterval.data(), audioAnalyzer.data.intervalLen);
                    ImPlot::EndPlot();
                }
                if (ImPlot::BeginPlot(spectrumTitle.c_str())) {
                    ImPlot::SetupAxes("Frequency (Hz)", "Intensity");
                    if (audioAnalyzer.data.analyzeFullAudio) {
                        ImPlot::PlotLine("FFT", audioAnalyzer.data.fxAxis.data(), audioAnalyzer.data.fIntensity.data(), audioAnalyzer.data.len / 2);
                    } else {
                        ImPlot::PlotLine("FFT", audioAnalyzer.data.fxAxis.data(), audioAnalyzer.data.fIntensity.data(), audioAnalyzer.data.intervalLen / 2);
                    }
                    ImPlot::EndPlot();
                }
            }
        }

        std::string currentNote = realtimeRecorder && realtimeRecorder->IsRecording()
            ? realtimeRecorder->GetDetectedNote()
            : audioAnalyzer.data.detectedNote;
        LoadRigby(audioAnalyzer.data.len, currentNote);
        ImGui::End();
        RenderWindow(window);
    }

    if (realtimeRecorder) {
        delete realtimeRecorder;
    }

    ImPlot::DestroyContext();
    TerminateIMGUI(window);
    return 0;
}
