#ifndef PEAK_DETECTOR_HPP
#define PEAK_DETECTOR_HPP

#include <vector>
#include <array>

using sample_type = double;
constexpr int Global_Filter_N = 41;
constexpr int min_peak_spacing = 500;

template <typename T=sample_type, int N=Global_Filter_N>
class Filter_MA {
public:
    T clk(T in) {
        sum += in - buf[index];
        buf[index] = in;
        index = (index + 1) % N;
        if constexpr (std::is_floating_point_v<T>)
            return sum / N;
        else
            return (sum + (N / 2)) / N;
    }

    bool update_vectors(const std::vector<T>& vin, std::vector<T>* pvout, std::vector<T>* prawout = nullptr) {
        if (vin.size() <= N || pvout == nullptr)
            return false;
        pvout->reserve(vin.size() - N);
        if (prawout != nullptr)
            pvout->reserve(vin.size() - N);
        for (size_t i = 0; i < N; i++)
            clk(vin[i]);
        for (size_t i = N; i < vin.size(); i++) {
            pvout->push_back(clk(vin[i]));
            if (prawout != nullptr)
                prawout->push_back(vin[i - N / 2]);
        }
        return true;
    }

private:
    std::array<T, N> buf{};
    T sum{};
    size_t index{};
};

template <typename T=sample_type>
std::pair<T, T> peak_detect(T y1, T y2, T y3) {
    T pk = 100 * (y1 - y3) / (2 * (y1 - 2 * y2 + y3));
    T mag = 2 * y2 - y1 - y3;
    return std::pair{ pk, mag };
}

struct WaveInfo {
    sample_type w_mean{};
    sample_type w_max{};
    sample_type w_min{};
    std::vector<sample_type> peaks;
    std::vector<sample_type> mags;
};

WaveInfo get_wave_info(std::vector<sample_type> v);

#endif
