#include <math.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Hard-code number of Fourier series terms for now
#define N_TERMS 5

// Helper: find a 1D signal f[x] from a thin rasterized curve by taking the mean y of lit pixels per column.
static void extract_signal_from_canvas(const uint8_t *canvas, size_t width, size_t height, float *f_out){
    for (size_t x = 0; x < width; ++x) f_out[x] = NAN;

    for (size_t x = 0; x < width; ++x){
        int count = 0;
        double sumy = 0.0;
        for (size_t y = 0; y < height; ++y){
            if (canvas[y*width + x]) { sumy += (double)y; count++; }
        }
        if (count > 0) f_out[x] = (float)(sumy / (double)count);
    }

    // Fill gaps (if a column had no lit pixels)
    float last = NAN;
    for (size_t x = 0; x < width; ++x){            // forward fill
        if (isnan(f_out[x])) f_out[x] = last; else last = f_out[x];
    }
    float next = NAN;
    for (size_t xi = 0; xi < width; ++xi){         // backward fill
        size_t x = width-1 - xi;
        if (isnan(f_out[x])) f_out[x] = next; else next = f_out[x];
    }
    for (size_t x = 0; x < width; ++x){            // fallback (empty canvas)
        if (isnan(f_out[x])) f_out[x] = (float)(height/2.0);
    }
}

static void compute_fourier_coeffs(const float *f, size_t N, int K,
                                   double *a0_out, double *a_out, double *b_out){
    const double two_over_N = 2.0 / (double)N;
    double a0 = 0.0;
    for (size_t n = 0; n < N; ++n) a0 += (double)f[n];
    a0 /= (double)N;
    *a0_out = a0;

    for (int k = 1; k <= K; ++k){
        double ak = 0.0, bk = 0.0;
        for (size_t n = 0; n < N; ++n){
            double t = (2.0*M_PI*(double)k*(double)n)/(double)N;
            double fn = (double)f[n];
            ak += fn * cos(t);
            bk += fn * sin(t);
        }
        a_out[k-1] = two_over_N * ak;
        b_out[k-1] = two_over_N * bk;
    }
}

static void reconstruct_signal(size_t N, int K, double a0,
                               const double *a, const double *b, float *out){
    for (size_t n = 0; n < N; ++n){
        double y = a0; // mean
        for (int k = 1; k <= K; ++k){
            double t = (2.0*M_PI*(double)k*(double)n)/(double)N;
            y += a[k-1]*cos(t) + b[k-1]*sin(t);
        }
        out[n] = (float)y;
    }
}

int fourier_spectrum(uint8_t *canvas, size_t width, size_t height){
    if (!canvas || !width || !height) return 0;

    float *f = (float*)malloc(width*sizeof(float));
    float *fhat = (float*)malloc(width*sizeof(float));
    if (!f || !fhat){ free(f); free(fhat); return 0; }

    // 1) Extract signal
    extract_signal_from_canvas(canvas, width, height, f);

    // 2) Fourier up to N_TERMS
    double a0;
    double a[N_TERMS], b[N_TERMS];
    compute_fourier_coeffs(f, width, N_TERMS, &a0, a, b);

    // 3) Reconstruct with exactly N_TERMS
    reconstruct_signal(width, N_TERMS, a0, a, b, fhat);

    // 4) Draw the approximation back into canvas
    memset(canvas, 0, width*height*sizeof(uint8_t));
    for (size_t x = 0; x < width; ++x){
        int y = (int)llround((long double)fhat[x]);
        if (y >= 0 && (size_t)y < height) canvas[y*width + x] = 255;
    }

    free(f);
    free(fhat);
    return 1;
}

