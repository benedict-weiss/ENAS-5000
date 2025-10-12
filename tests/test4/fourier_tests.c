#include <math.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define N_TERMS 5

// Safe multiply: returns 0 on overflow, 1 on success (result in *out)
static int safe_mul_size(size_t a, size_t b, size_t *out){
    if (a == 0 || b == 0) { *out = 0; return 1; }
    if (a > SIZE_MAX / b) return 0;
    *out = a * b;
    return 1;
}

static void extract_signal_from_canvas(const uint8_t *canvas, size_t width, size_t height, float *f_out){
    // Use a sentinel rather than NaN to avoid UB under fast-math
    const float SENT = -1.0f;

    for (size_t x = 0; x < width; ++x) f_out[x] = SENT;

    for (size_t x = 0; x < width; ++x){
        int count = 0;
        double sumy = 0.0;
        for (size_t y = 0; y < height; ++y){
            if (canvas[y*width + x]) { sumy += (double)y; count++; }
        }
        if (count > 0) f_out[x] = (float)(sumy / (double)count);
    }

    // Fill gaps (if a column had no lit pixels)
    float last = SENT;
    for (size_t x = 0; x < width; ++x){            // forward fill
        if (f_out[x] == SENT) f_out[x] = last; else last = f_out[x];
    }
    float next = SENT;
    for (size_t xi = 0; xi < width; ++xi){         // backward fill
        size_t x = width-1 - xi;
        if (f_out[x] == SENT) f_out[x] = next; else next = f_out[x];
    }
    for (size_t x = 0; x < width; ++x){            // fallback (empty canvas)
        if (f_out[x] == SENT) f_out[x] = (float)(height/2.0);
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
        // clamp to a safe float (defensive)
        if (!isfinite(y)) y = 0.0;
        out[n] = (float)y;
    }
}

int fourier_spectrum(uint8_t *canvas, size_t width, size_t height){
    if (!canvas || width == 0 || height == 0) return 0;

    // Check for overflow in buffer size computations
    size_t nbytes = 0;
    if (!safe_mul_size(width, height, &nbytes)) return 0; // overflow
    if (!safe_mul_size(nbytes, sizeof(uint8_t), &nbytes)) return 0; // theoretical

    // Allocate working buffers with overflow checks
    size_t wbytes = 0;
    if (!safe_mul_size(width, sizeof(float), &wbytes)) return 0;
    float *f    = (float*)malloc(wbytes);
    float *fhat = (float*)malloc(wbytes);
    if (!f || !fhat){ free(f); free(fhat); return 0; }

    // 1) Extract signal
    extract_signal_from_canvas(canvas, width, height, f);

    // 2) Fourier up to N_TERMS (clamp to <= width/2 and >= 1)
    int K = N_TERMS;
    if ((size_t)K > width/2) K = (int)(width/2);
    if (K < 1) K = 1;

    double a0 = 0.0;
    double a[N_TERMS] = {0}, b[N_TERMS] = {0};
    compute_fourier_coeffs(f, width, K, &a0, a, b);

    // 3) Reconstruct
    reconstruct_signal(width, K, a0, a, b, fhat);

    // 4) Draw the approximation back into canvas
    memset(canvas, 0, nbytes);
    for (size_t x = 0; x < width; ++x){
        // round safely and bound-check in size_t domain
        long y_l = lroundf(fhat[x]);
        if (y_l < 0) continue;
        size_t y = (size_t)y_l;
        if (y >= height) continue;

        size_t idx = 0;
        // y*width + x can't overflow if y<height and x<width and we already checked width*height
        idx = y * width + x;
        canvas[idx] = 255;
    }

    free(f);
    free(fhat);
    return 1;
}
