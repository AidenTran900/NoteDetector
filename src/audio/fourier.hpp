#ifndef FOURIER_HPP
#define FOURIER_HPP

#include <vector>
#include <complex>

using Complex = std::complex<double>;
using CArray = std::vector<Complex>;

std::vector<double> FFTfreq(int N, double d = 1.0);
void FFT(CArray &x);

#endif
