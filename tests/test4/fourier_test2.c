// fourier.c
#include "fourier.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ... your existing 1D helpers and fourier_1d(...) stay as-is ... */

// ------------- 2D DFT helpers -------------
static void dft_1d_complex(const double *xr, const double *xi, size_t N,
                           double *Xr, double *Xi, int inverse)
{
    // inverse==0: forward DFT, inverse!=0: inverse DFT (unused here)
    const double s = (inverse ? 2.0 : -2.0) * M_PI / (double)N;
    for (size_t k = 0; k < N; ++k) {
        double sum_r = 0.0, sum_i = 0.0;
        for (size_t n = 0; n < N; ++n) {
            double ang = s * (double)(k * n);
            double c = cos(ang), sn = sin(ang);
            // (xr[n] + i*xi[n]) * (c + i*sn)
            sum_r += xr[n] * c - xi[n] * sn;
            sum_i += xr[n] * sn + xi[n] * c;
        }
        Xr[k] = sum_r;
        Xi[k] = sum_i;
    }
}

// FFT-shift (swap quadrants) so DC is centered
static void fftshift_2d(uint8_t *img, size_t W, size_t H)
{
    size_t halfW = W / 2, halfH = H / 2;
    // swap Q0<->Q3 and Q1<->Q2
    for (size_t y = 0; y < halfH; ++y) {
        for (size_t x = 0; x < halfW; ++x) {
            size_t a = y * W + x;                   // Q0
            size_t b = (y + halfH) * W + (x + halfW); // Q3
            uint8_t t = img[a]; img[a] = img[b]; img[b] = t;
        }
    }
    for (size_t y = 0; y < halfH; ++y) {
        for (size_t x = halfW; x < W; ++x) {
            size_t a = y * W + x;                   // Q1
            size_t b = (y + halfH) * W + (x - halfW); // Q2
            uint8_t t = img[a]; img[a] = img[b]; img[b] = t;
        }
    }
}

int fourier_2d_spectrum(uint8_t *canvas, size_t W, size_t H)
{
    if (!canvas || W == 0 || H == 0) return 0;

    // 1) Copy/normalize input to double real part in [0,1], imag=0
    double *row_in_r = (double*)calloc(W, sizeof(double));
    double *row_in_i = (double*)calloc(W, sizeof(double));
    double *row_out_r = (double*)calloc(W, sizeof(double));
    double *row_out_i = (double*)calloc(W, sizeof(double));
    if (!row_in_r || !row_in_i || !row_out_r || !row_out_i) {
        free(row_in_r); free(row_in_i); free(row_out_r); free(row_out_i);
        return 0;
    }

    // Intermediate after row-DFT: H rows × W complex
    double *tmp_r = (double*)calloc(W * H, sizeof(double));
    double *tmp_i = (double*)calloc(W * H, sizeof(double));
    if (!tmp_r || !tmp_i) {
        free(row_in_r); free(row_in_i); free(row_out_r); free(row_out_i);
        free(tmp_r); free(tmp_i);
        return 0;
    }

    // 2) Row-wise DFT for each y
    for (size_t y = 0; y < H; ++y) {
        // load row
        const uint8_t *src = canvas + y * W;
        for (size_t x = 0; x < W; ++x) {
            row_in_r[x] = (double)src[x] / 255.0; // normalize
            row_in_i[x] = 0.0;
        }
        dft_1d_complex(row_in_r, row_in_i, W, row_out_r, row_out_i, 0);
        // store to tmp
        for (size_t kx = 0; kx < W; ++kx) {
            tmp_r[y * W + kx] = row_out_r[kx];
            tmp_i[y * W + kx] = row_out_i[kx];
        }
    }

    // 3) Column-wise DFT for each x (operate down tmp_*)
    double *col_in_r = (double*)calloc(H, sizeof(double));
    double *col_in_i = (double*)calloc(H, sizeof(double));
    double *col_out_r = (double*)calloc(H, sizeof(double));
    double *col_out_i = (double*)calloc(H, sizeof(double));
    if (!col_in_r || !col_in_i || !col_out_r || !col_out_i) {
        free(row_in_r); free(row_in_i); free(row_out_r); free(row_out_i);
        free(tmp_r); free(tmp_i);
        free(col_in_r); free(col_in_i); free(col_out_r); free(col_out_i);
        return 0;
    }

    // final complex spectrum (overwrite tmp_* to save memory)
    for (size_t x = 0; x < W; ++x) {
        // load column
        for (size_t y = 0; y < H; ++y) {
            size_t idx = y * W + x;
            col_in_r[y] = tmp_r[idx];
            col_in_i[y] = tmp_i[idx];
        }
        dft_1d_complex(col_in_r, col_in_i, H, col_out_r, col_out_i, 0);
        // store back
        for (size_t ky = 0; ky < H; ++ky) {
            size_t idx = ky * W + x;
            tmp_r[idx] = col_out_r[ky];
            tmp_i[idx] = col_out_i[ky];
        }
    }

    // 4) Magnitude -> log scale
    double mag_min = INFINITY, mag_max = -INFINITY;
    double *mag = (double*)malloc(W * H * sizeof(double));
    if (!mag) {
        free(row_in_r); free(row_in_i); free(row_out_r); free(row_out_i);
        free(tmp_r); free(tmp_i);
        free(col_in_r); free(col_in_i); free(col_out_r); free(col_out_i);
        return 0;
    }
    for (size_t i = 0; i < W * H; ++i) {
        double m = sqrt(tmp_r[i]*tmp_r[i] + tmp_i[i]*tmp_i[i]);
        // log(1+m) for dynamic range compression
        double lm = log(1.0 + m);
        mag[i] = lm;
        if (lm < mag_min) mag_min = lm;
        if (lm > mag_max) mag_max = lm;
    }

    // 5) Normalize to 0..255 and write to canvas
    double denom = (mag_max > mag_min) ? (mag_max - mag_min) : 1.0;
    for (size_t i = 0; i < W * H; ++i) {
        double v = (mag[i] - mag_min) / denom; // 0..1
        int px = (int)lround(v * 255.0);
        if (px < 0) px = 0; if (px > 255) px = 255;
        canvas[i] = (uint8_t)px;
    }

    // 6) FFT-shift (center the DC)
    fftshift_2d(canvas, W, H);

    free(row_in_r); free(row_in_i); free(row_out_r); free(row_out_i);
    free(col_in_r); free(col_in_i); free(col_out_r); free(col_out_i);
    free(tmp_r); free(tmp_i);
    free(mag);

    return 1;
}
