#include "peakDetector.hpp"
#include <numeric>
#include <algorithm>
#include <limits>

using std::vector, std::size_t;

WaveInfo get_wave_info(std::vector<sample_type> v)
{
    constexpr int N = Global_Filter_N;
    static_assert(Global_Filter_N & 1, "filter must be odd number");
    WaveInfo w;
    w.w_max = *std::max_element(v.begin(), v.end());
    w.w_min = *std::min_element(v.begin(), v.end());
    // "0ll + sample_type{}" Produces either a double or long long int depending on sample_type to stop overflow if > 2M samples
    w.w_mean = static_cast<sample_type>(std::accumulate(v.begin(), v.end(), 0ll + sample_type{}) / std::size(v));
    sample_type pos_thresh = w.w_mean + (w.w_max - w.w_mean) / 5;  // 10% above ave.
    sample_type neg_thresh = 1000000000000000000;
    sample_type lastVal = 0;
    // double nextCoef = 10000;
    int search_polarity = 0;    // if 0 prior peak polarity not determined
    int lastPeakIndex = -min_peak_spacing;

    for (int i = 0; i < int(v.size()) - N; i++)
    {
        if (i-lastPeakIndex < min_peak_spacing) { continue; }
        const int center = N/2;
        // /(v[i] - lastVal) < (w.w_max * nextCoef) && 
        if (v[i] > pos_thresh && v[i] > v[i + N - 1] && v[i] < v[i + center] && search_polarity >= 0)
        {
            search_polarity = -1;
            auto results = peak_detect(v[i], v[i + center], v[i + N - 1]);
            w.peaks.push_back(results.first * center / 100 + i + center);
            w.mags.push_back(results.second);
            lastPeakIndex = i;
        }
        if (v[i] < neg_thresh && v[i] < v[i + N - 1] && v[i] > v[i + center] && search_polarity <= 0)
        {
            search_polarity = 1;
            auto results = peak_detect(v[i], v[i + N / 2], v[i + N - 1]);
            w.peaks.push_back(results.first * center / 100 + i + center);
            w.mags.push_back(-results.second);
            lastPeakIndex = i;
        }
        lastVal = v[i];
    }
    return w;
}