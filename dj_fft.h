#ifndef DJ_FFT_H
#define DJ_FFT_H


#include <vector>
#include <complex>
#include <cmath>

namespace dj {
    inline void fft_recursiva(std::vector<std::complex<float>>& a) {
        size_t n = a.size();
        if (n <= 1) return;

        std::vector<std::complex<float>> a_par(n / 2), a_impar(n / 2);
        for (size_t i = 0; i < n / 2; i++) {
            a_par[i] = a[2 * i];
            a_impar[i] = a[2 * i + 1];
        }

        fft_recursiva(a_par);
        fft_recursiva(a_impar);

        float angulo = 2 * M_PI / n * -1;
        std::complex<float> w(1), wn(std::cos(angulo), std::sin(angulo));
        for (size_t i = 0; i < n / 2; i++) {
            a[i] = a_par[i] + w * a_impar[i];
            a[i + n / 2] = a_par[i] - w * a_impar[i];
            w *= wn;
        }
    }
    // Función principal para usar con tus floats
    inline std::vector<float> calcular_fft(const std::vector<float>& muestrasMono) {
        size_t n = muestrasMono.size();
        std::vector<std::complex<float>> bufferComplejo(n);
        for (size_t i = 0; i < n; i++) {
            bufferComplejo[i] = std::complex<float>(muestrasMono[i], 0.0f);
        }

        fft_recursiva(bufferComplejo);

        // Nos quedamos solo con la primera mitad (512 frecuencias útiles)
        std::vector<float> magnitudes(n / 2);
        for (size_t i = 0; i < n / 2; i++) {
            // Calculamos la magnitud física (el volumen real) del número complejo
            magnitudes[i] = std::abs(bufferComplejo[i]) / n;
        }
        return magnitudes;
    }
}

#endif // DJ_FFT_H
