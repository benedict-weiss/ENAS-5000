#include "fourier.h"

#include "raster.h" // maybe not great

// returns 0 if multiplication causes size_t overflow
// otherwise returns 1 if you can safely multiply
// result in output parameter *out
int safe_multiply(size_t a, size_t b, size_t *out) {
    if (!out) return 0;
    if (a == 0 || b == 0) {
        *out = 0;
        return 1;
    }
    if (a > SIZE_MAX / b) {
        return 0;
    } else {
        *out = a * b;
        return 1;
    }
}

// extracts a 1D float average of nonzero pixels in a column
// populate using column-wise y average
// forward fill and backwards fill based on last occupied value
// if empty canvas, populate with mid value
// result in output buffer of length width
void extract_signal(const uint8_t *canvas, size_t width, size_t height, float *s_out) {
    const float UNFILLED = -1.0f;

    // fills output array with sentinel
    for (size_t i = 0; i < width; ++i){
        s_out[i] = UNFILLED;
    }

    // finds average signal for each column
    for (size_t x = 0; x < width; ++x) {
        int count = 0;
        double y_sum = 0.0;
        const uint8_t *col = canvas + x;
        for (size_t y = 0; y < height; ++y) {
            if (*col) {
                y_sum += (double)y;
                count ++;
            }
            col += width;
        }
        if (count > 0) {
            s_out[x] = (float)y_sum / (double)count;
        }
    }

    // back fill
    float last_filled = UNFILLED;
    for (size_t x = 0; x < width; ++x) {
        if (s_out[x] != UNFILLED) {
            last_filled = s_out[x];
        } else {
            s_out[x] = last_filled;
        }
    }

    // forward fill
    float first_filled = UNFILLED;
    for (size_t x = width; x-- > 0; ){
        if (s_out[x] == UNFILLED) {
            s_out[x] = first_filled;
        } else {
            first_filled = s_out[x];
        }
    }

    // in case everything is still unfilled (should never happen)
    if (width) {
        float mid = (float)(0.5 * height);
        for (size_t x = 0; x < width; ++x) {
            if (s_out[x] == UNFILLED) {
                s_out[x] = mid;
            }
        }
    }

};

// WRITE OUT 2 FUNCTIONS BELOW IN LATEX

// compute real Fourier coefficients of a 1D signal
// f is input signal of length N (from extract_signal above)
// N is number of samples
// K is number of harmonics (change to be able to vary this later)
// a0_out is mean term output
// a and b are output arrays of length K with cosine and sine coeffs respectively (have to allocate in driver function)
void dft_real_coeffs(const float *f, size_t N, int K, double *a0_out, double *a, double *b){  // careful with types

    double a0 = 0.0;
    for (size_t n = 0; n < N; ++n) {
        a0 += (double)f[n];
    }
    a0 /= (double)N;
    *a0_out = a0;

    const double scale = 2.0 / (double)N;

    for (int k = 1; k <= K; ++k) {
        double ak = 0.0;
        double bk = 0.0;
        const double twopikoverN = scale * M_PI * (double)k;
        for (size_t n = 0; n < N; ++n) {
            double arg = twopikoverN * (double)n;
            double fn = (double)f[n];
            ak += fn * cos(arg);
            bk += fn * sin(arg);
        }
        a[k-1] = scale * ak;
        b[k-1] = scale * bk;
    }

};

// reconstruct the signal from truncated Fourier series (ie find approximation)
// N, K, a0, a, b as above
// out is output parameter with form float[N]
void reconstruct_series(size_t N, int K, double a0, double *a, double *b, float *out) {
    for (size_t n = 0; n < N; ++n) {
        double y = a0;
        for (int k = 1; k <= K; ++k) {
            double arg = 2.0 * M_PI * (double)k * (double)n / (double)N;
            y += a[k-1] * cos(arg) + b[k-1] * sin(arg);
        }
        if(!isfinite(y)) {
            y = 0.0;
        }
        out[n] = (float)y;
    }
}


int fourier_1d(uint8_t *canvas, size_t width, size_t height, int num_terms){
    if (!canvas || width == 0 || height == 0) return 0;

    // should be RASTER_SIZE ^2
    size_t total_pixels = 0;
    if (!safe_multiply(width, height, &total_pixels)) return 0;

    size_t function_buffer = 0;
    if(!safe_multiply(width, sizeof(float), &function_buffer)) return 0;

    float *input = (float *)malloc(function_buffer);
    float *output = (float *)malloc(function_buffer);

    if (!input || !output) {
        free(input);
        free(output);
        return 0;
    }

    extract_signal(canvas, width, height, input);

    int K = num_terms;
    if ((size_t)K > width / 2) { // probably overkill - limited by io
        K = (int)width / 2; 
    }
    if (K < 1) K = 1;

    double *a = (double*)calloc((size_t)K, sizeof(double));
    double *b = (double*)calloc((size_t)K, sizeof(double));

    if (!a || !b) {
        free(a);
        free(b);
        free(input);
        free(output);
        return 0;
    }

    double a0 = 0.0;
    dft_real_coeffs(input, width, K, &a0, a, b);

    reconstruct_series(width, K, a0, a, b, output);

    memset(canvas, 0, total_pixels);

    // this is now a smooth line using bresenham's line algorithm previously implemented

    // add original line
    for (int x = 1; x < width; ++x) {
        raster_line(canvas,
            x - 1, (int)lroundf(input[x-1]),
            x, (int)lroundf(input[x]),
            2);
    }

    // populate canvas
    for (int x = 1; x < width; ++x) {
        raster_line(canvas,
            x - 1, (int)lroundf(output[x-1]),
            x, (int)lroundf(output[x]),
            1);
    }

    free(a);
    free(b);
    free(input);
    free(output);

    return 1;
}

//better to do it from polyline in this case I think
int fourier_2d_from_pl(uint8_t *canvas, size_t width, size_t height, int num_terms, const Polyline *pl) {
    return 0;
}
