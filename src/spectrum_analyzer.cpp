#include "spectrum_analyzer.h"
#include "fftsg.h"

#include <cmath>
#include <algorithm>
#include <format>

#include "const.h"

static const double OVERLAP_RATIO = 0.5;
static const double FREQUENCY_MIN = 20.0;

static const int DISPLAY_MAX_WIDTH = 140;
static const int DISPLAY_MAX_HEIGHT = 20;
static const double DISPLAY_DB_MIN = -60.0;

void *thread_function(void* arg) {
    ((SpectrumAnalyzer *)arg)->thread_loop();
    return NULL;
}

// private methods

void SpectrumAnalyzer::calculate_fft(int buffer_index) {
    // calculate max value
    this->input_max_[buffer_index] = 1e-20;
    for (int i = 0; i < this->process_size_; i++) {
        double abs_val = fabs(this->input_buffers_[buffer_index][i]);
        if (this->input_max_[buffer_index] < abs_val) {
            this->input_max_[buffer_index] = abs_val;
        }
    }
    // apply Hanning window
    for (int i = 0; i < this->process_size_; i++) {
        double window = 0.5 * (1.0 - cos(2.0 * M_PI * (double)i / (double)(this->process_size_ - 1)));
        this->input_buffers_[buffer_index][i] *= window;
    }
    /*
        process_size_:
                        data length (int)
                        n >= 2, n = power of 2
        1:              forward transform
        input_buffers_[0...n-1]:
                        input/output data (double *)
                            output data
                                a[2*k] = R[k], 0<=k<n/2
                                a[2*k+1] = I[k], 0<k<n/2
                                a[1] = R[n/2]
        work_area_[0...*]:
                        work area for bit reversal (int *)
                        length of ip >= 2+sqrt(n/2)
                        strictly, 
                        length of ip >= 
                            2+(1<<(int)(log(n/2+0.5)/log(2))/2).
                        ip[0],ip[1] are pointers of the cos/sin table.
        cos_sin_table_[0...n/2-1]:
                        cos/sin table (double *)
                        w[],ip[] are initialized if ip[0] == 0.
    */
    rdft(
        this->process_size_,
        1,
        this->input_buffers_[buffer_index],
        this->work_area_,
        this->cos_sin_table_
    );
    this->current_result_index_ = buffer_index;
    this->result_callback_(this);
}

void SpectrumAnalyzer::copy_with_mix(double *dest, const float *src, unsigned int frames, unsigned int channels) {
    const float *end = src + (frames * channels);
    while (src < end) {
        double sum = 0.0;
        for (unsigned int c = 0; c < channels; c++) {
            sum += (double)*src++;
        }
        *dest++ = sum / (double)channels;
    }
}


// public methods

SpectrumAnalyzer::SpectrumAnalyzer(void (*result_callback)(SpectrumAnalyzer *sa)) {
    result_callback_ = result_callback;
    display_.reserve(DISPLAY_MAX_HEIGHT * (DISPLAY_MAX_WIDTH + 5) + 10);

    pthread_mutex_init(&this->mutex_, NULL);
    pthread_cond_init(&this->cond_, NULL);
    pthread_create(&this->thread_, NULL, thread_function, this);
};

SpectrumAnalyzer::~SpectrumAnalyzer() {
    pthread_detach(this->thread_);
    pthread_mutex_destroy(&this->mutex_);
    pthread_cond_destroy(&this->cond_);

    for (int i = 0; i < 2; i++) {
        if (this->input_buffers_[i]) {
            delete[] this->input_buffers_[i];
        }
    }
    if (this->work_area_) {
        delete[] this->work_area_;
    }
    if (this->cos_sin_table_) {
        delete[] this->cos_sin_table_;
    }
};

void SpectrumAnalyzer::initialize(unsigned int sample_rate, unsigned int updates_per_second) {
    this->sample_rate_ = sample_rate;
    this->update_size_ = sample_rate / updates_per_second;
    double samples_min = std::max(
        (double)sample_rate / FREQUENCY_MIN,
        (1.0 + OVERLAP_RATIO) * (double)this->update_size_
    );
    this->process_size_ = pow(2, ceil(log2(samples_min)));

    for (int i = 0; i < 2; i++) {
        this->input_buffers_[i] = new double[this->process_size_];
    }
    this->work_area_ = new int[2 + ceil(sqrt((double)this->process_size_ / 2.0))];
    this->work_area_[0] = 0; // initialize for fftsg
    this->cos_sin_table_ = new double[this->process_size_ / 2];
}

void SpectrumAnalyzer::process(const float *input_buffer, unsigned int frames, unsigned int channels) {
    int copy_size = std::min((int)frames, this->process_size_ - this->buffer_cur_);
    this->copy_with_mix(
        &this->input_buffers_[this->current_buffer_index_][this->buffer_cur_],
        input_buffer,
        copy_size, channels
    );
    this->buffer_cur_ += copy_size;
    if (this->buffer_cur_ >= this->process_size_) {
        // switch buffers & process FFT
        pthread_mutex_lock(&this->mutex_);
        this->current_buffer_index_ = 1 - this->current_buffer_index_;
        // copy overlap data to the beginning of input_buffer
        int overlap_size = this->process_size_ - this->update_size_;
        if (overlap_size > 0) {
            memcpy(
                &this->input_buffers_[this->current_buffer_index_][0],
                &this->input_buffers_[1 - this->current_buffer_index_][this->process_size_ - overlap_size],
                overlap_size * sizeof(double)
            );
            this->buffer_cur_ = overlap_size;
        } else {
            this->buffer_cur_ = 0;
        }
        // process FFT on filled buffer
        pthread_cond_signal(&this->cond_);
        pthread_mutex_unlock(&this->mutex_);
        // copy rest of input_buffer
        int remaining_frames = frames - copy_size;
        if (remaining_frames > 0) {
            copy_with_mix(
                &this->input_buffers_[this->current_buffer_index_][this->buffer_cur_],
                input_buffer + (copy_size * channels),
                remaining_frames, channels
            );
            this->buffer_cur_ += remaining_frames;
        }
    }
}

void SpectrumAnalyzer::thread_loop() {
    while (1) {
        pthread_mutex_lock(&this->mutex_);
        pthread_cond_wait(&this->cond_, &this->mutex_);
        this->calculate_fft(1 - this->current_buffer_index_);
        pthread_mutex_unlock(&this->mutex_);
    }
}

std::string frequency_label(double freq) {
    if (freq >= 1000.0) {
        return std::to_string((int)(freq / 1000.0)) + "k";
    } else {
        return std::to_string((int)freq);
    }
}

const std::string *SpectrumAnalyzer::render_display(int width, int height) {
    static double m2_bins[DISPLAY_MAX_WIDTH - 2];
    static int col_bins[DISPLAY_MAX_WIDTH];

    width = std::clamp(width, 5, DISPLAY_MAX_WIDTH);
    height = std::clamp(height, 4, DISPLAY_MAX_HEIGHT);
    int cols = width - 2;
    int rows = height - 1;

    // calculate max of magnitude^2 for each frequency bin
    double *buffer = this->input_buffers_[this->current_result_index_];
    double left_freq = std::max(FREQUENCY_MIN, (double)this->sample_rate_ / (double)this->process_size_);
    double right_freq = (double)this->sample_rate_ / 2;
    double log_left_freq = log2(left_freq);
    double hratio = (double)(cols - 1) / (log2(right_freq) - log_left_freq);
    for (int i = 0; i < cols; ++i) {
        m2_bins[i] = 1e-20;
    }
    // ignore k = 0 DC, k = 1 DC * Hunnig window.
    for (int i = 2; i < this->process_size_ / 2; ++i) {
        double freq = (double)i * (double)this->sample_rate_ / (double)this->process_size_;
        int col = std::clamp((int)round(hratio * (log2(freq) - log_left_freq)), 0, cols - 1);
        double re = buffer[2 * i];
        double im = buffer[2 * i + 1];
        double m2 = re * re + im * im;
        if (m2_bins[col] < m2) {
            m2_bins[col] = m2;
        }
    }
    // convert to display rows
    double vratio = (rows - 1) / (0 - DISPLAY_DB_MIN);
    for (int i = 0; i < cols; ++i) {
        // x16 := (correction for attenuation by window function:2 * correction for FFT of real part only:2)^2
        double db = 10.0 * log10(16.0 * m2_bins[i] / (double)(this->process_size_ * this->process_size_));
        col_bins[i] = std::clamp((int)round(vratio * (db - DISPLAY_DB_MIN)), 0, rows);
    }
    // total level
    col_bins[cols] = 0;
    double total_db = 20.0 * log10(this->input_max_[this->current_result_index_]);
    col_bins[cols + 1] = std::clamp((int)round(vratio * (total_db - DISPLAY_DB_MIN)), 0, rows);

    // render to display_
    this->display_.clear();
    int level_green = rows - std::clamp((int)round(vratio * (-6 - DISPLAY_DB_MIN)), 0, rows) + 1;
    this->display_ += CHAR_COLOR_RED;
    for (int y = 0; y < rows; ++y) {
        if (y == 2) { this->display_ += CHAR_COLOR_YELLOW; }
        if (y == level_green) { this->display_ += CHAR_COLOR_GREEN; }
        for (int x = 0; x < width; ++x) {
            if (col_bins[x] >= rows - y) {
                this->display_ += '*';
            } else if (x == width - 2 && y == 1) {
                this->display_ += '-';
            } else {
                this->display_ += ' ';
            }
        }
        this->display_ += "\033[0K\n";
    }
    this->display_ += CHAR_COLOR_RESET;

    // frequency labels
    size_t cur = 0;
    size_t rest_width = cols;
    // first leftmost label
    std::string left_label = frequency_label(left_freq);
    if (left_label.length() <= cols - cur) {
        this->display_ += left_label + " ";
        cur += left_label.length() + 1;
    }
    // print labels
    static const double freq_base = 1000;
    double freq_cur = pow(2.0, (double)(cur - 1) / hratio + log_left_freq);
    double freq_label = pow(2.0, floor(log2(freq_cur / freq_base))) * freq_base;
    while(cur < cols) {
        freq_cur = pow(2.0, (double)cur / hratio + log_left_freq);
        if (freq_label * 2 < freq_cur) {
            while (freq_label * 2 < freq_cur) {
                freq_label *= 2;
            }
            std::string label = frequency_label(freq_label);
            if (label.length() <= cols - cur) {
                this->display_ += label + " ";
                cur += label.length() + 1;
            } else {
                break; // no more space for label
            }
            freq_cur = pow(2.0, (double)(cur - 1) / hratio + log_left_freq);
            freq_label = pow(2.0, floor(log2(freq_cur / freq_base))) * freq_base;
        } else {
            this->display_ += " ";
            cur++;
        }
    }
    while (cur <= cols) {
        this->display_ += " ";
        cur++;
    }
    this->display_ += "L\033[0K\r";
    // move up cursor for next update.
    this->display_ += std::format("\033[{}A", height - 1);

    return &this->display_;
}
