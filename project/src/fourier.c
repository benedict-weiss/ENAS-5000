#include "fourier.h"

#include "raster.h" // maybe not great from file structure perspective

#define MIN_SAMPLE_DENSITY 128
#define MAX_SAMPLE_DENSITY 4096
#define CURVE_DENSITY 4

// returns 0 if multiplication causes size_t overflow
// otherwise returns 1 if you can safely multiply
// result in output parameter *out
// probably overkill but initially had crashing issues and thought this may have been an issue
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

    raster_clear(canvas);

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

// resamples evenly along the polyline
// wasn't necessary for 1D as you could use pixel coordinate
int uniform_pts_polyline(const Polyline *pl, Pt *output, size_t num_output){
    if(!pl || !output || num_output < 2) return 0;

    Vec2 *pts = pl->pts;
    int num_pts = pl->len;
    if (num_pts < 2) return 0;

    double *length_arr = malloc(sizeof(double) * (num_pts + 1));
    if (!length_arr) return 0;

    // length_arr[i] gives cumulative arclength from first point to ith point
    length_arr[0] = 0.0;
    for (size_t i = 1; i < num_pts; ++i){
        double dx = pts[i].x - pts[i-1].x;
        double dy = pts[i].y - pts[i-1].y;
        length_arr[i] = length_arr[i-1] + sqrt(dx*dx + dy*dy);
    }

    // accounts for loop closing
    double dx = pts[0].x - pts[num_pts-1].x;
    double dy = pts[0].y - pts[num_pts-1].y;
    length_arr[num_pts] = length_arr[num_pts-1] + sqrt(dx*dx + dy*dy);

    double total_length = length_arr[num_pts];

    // check for overflow here?

    size_t segment = 0; // current segment index
    size_t final_idx = num_pts;

    // NB num_output is points in output pts arr
    for (size_t i = 0; i < num_output; ++i){
        // target arclength position
        double target_length = (total_length * i) / num_output;

        // advance until you reach original point before length becomes greater
        while (segment + 1 <= final_idx && length_arr[segment + 1] <= target_length){
            segment ++;
        }

        // wrap endpoints (just in case)
        size_t i0 = segment % num_pts;
        size_t i1 = (segment + 1) % num_pts;
        Vec2 A = pts[i0];
        Vec2 B = pts[i1];

        double d_arclength = length_arr[segment + 1] - length_arr[segment];
        if (d_arclength < DBL_EPSILON) d_arclength = DBL_EPSILON; // prevents divide by zero

        double scale = (target_length - length_arr[segment]) / d_arclength;

        output[i].x = A.x + scale * (B.x - A.x);
        output[i].y = A.y + scale * (B.y - A.y);
    }


    free(length_arr);
    return 1;
}

typedef struct { double re, im; } complex_t;

// WRITE OUT IN LATEX FOR CLARITY

// compute 2d fourier descriptors (mostly based of second link found online)
// https://users.cs.utah.edu/~tch/CS6640/lectures/Weeks5-6/Zahn-Roskies.pdf
// https://link.springer.com/chapter/10.1007/978-1-84882-919-0_6 (specifically chapter 6)
void compute_fourier_descriptors(const Pt *input, size_t num_pts, int K, complex_t *output){
    // first, compute centroid
    double mean_x = 0.0;
    double mean_y = 0.0;
    for (size_t i = 0; i < num_pts; ++i){
        mean_x += input[i].x;
        mean_y += input[i].y;
    }
    mean_x /= num_pts;
    mean_y /= num_pts;

    // intialise output array
    for (int i = 0; i < 2 * K + 1; ++i){
        output[i].re = 0.0;
        output[i].im = 0.0;
    }

    // store centroid as coefficient of zero frequency component (ie in middle of array)
    output[K].re = mean_x;
    output[K].im = mean_y;

    double twopioverm = 2 * M_PI / num_pts;

    for (int k = -K; k <= K; ++k){
        if (k == 0) continue; // skips term defined above
        
        double sum_re = 0.0;
        double sum_im = 0.0;

        for (size_t m = 0; m < num_pts; ++m){
        
            // centred
            double x_re = input[m].x - mean_x;
            double y_im = input[m].y - mean_y;

            double theta = -twopioverm * k * m;

            sum_re += x_re * cos(theta) - y_im * sin(theta);
            sum_im += x_re * sin(theta) + y_im * cos(theta);

        }

        // normalise by sample num
        output[k+K].re = sum_re / num_pts;
        output[k+K].im = sum_im / num_pts;

    }
}

// hard to transcribe equations into code readably haha
void reconstruct_series_2d(const complex_t *input, int K, size_t num_samples, Pt *output){
    for (size_t r = 0; r < num_samples; ++r){
        // normalised around loop
        double t = (double)r / (double)num_samples; // nb have to cast here

        // start at centroid
        double x = input[K].re;
        double y = input[K].im;

        double twopit = 2 * M_PI * t;

        for (int k = 1; k <= K; ++k){

            double theta = twopit * k;
            complex_t c_pos = input[K+k];
            complex_t c_neg = input[K-k];

            // not the most readable, but adds contributions from pos / neg k
            x += c_pos.re * cos(theta) - c_pos.im * sin(theta);
            y += c_pos.re * sin(theta) + c_pos.im * cos(theta);
            x += c_neg.re * cos(theta) + c_neg.im * sin(theta);
            y += -c_neg.re * sin(theta) + c_neg.im * cos(theta);

        }

        output[r].x = x;
        output[r].y = y;

    }
}

//better to do it from polyline in this case I think
int fourier_2d_from_pl(uint8_t *canvas, size_t width, size_t height, int num_terms, const Polyline *pl) {
    // general safety checks
    if (!canvas || !pl || !pl->pts || pl->len < 2 || width == 0 || height == 0) return 0;

    size_t num_pts = pl->len;
    if (num_pts < MIN_SAMPLE_DENSITY) num_pts = MIN_SAMPLE_DENSITY;
    if (num_pts > MAX_SAMPLE_DENSITY) num_pts = MAX_SAMPLE_DENSITY;

    Pt *spaced_pts = malloc(sizeof(Pt)*num_pts);
    if(!spaced_pts) return 0;

    // resamples uniformly (stored in spaced_pts)
    if (!uniform_pts_polyline(pl, spaced_pts, num_pts)){
        free(spaced_pts);
        return 0;
    }

    // snap last sample to first to avoid gaps
    spaced_pts[num_pts-1].x = spaced_pts[0].x;
    spaced_pts[num_pts-1].y = spaced_pts[0].y;

    int K = num_terms;
    if (K > (num_pts / 2 - 1)) K = num_pts / 2 - 1;

    // initialise complex array for descriptors (as output)
    complex_t *descriptors = calloc(2*K+1, sizeof(complex_t));
    if (!descriptors) {
        free(spaced_pts);
        return 0;
    }

    compute_fourier_descriptors(spaced_pts, num_pts, K, descriptors);

    size_t num_samples = num_pts * CURVE_DENSITY;

    Pt *reconstructed = malloc(sizeof(Pt) * num_samples);
    if(!reconstructed){
        free(spaced_pts);
        free(descriptors);
        return 0;
    }

    reconstruct_series_2d(descriptors, K, num_samples, reconstructed);

    raster_clear(canvas);

    raster_closed_line_from_pts(canvas, spaced_pts, num_pts, 2);
    raster_closed_line_from_pts(canvas, reconstructed, num_samples, 1);

    free(spaced_pts);
    free(descriptors);
    free(reconstructed);
    return 1;

}