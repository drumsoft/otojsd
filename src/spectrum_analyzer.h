#ifndef SPECTRUM_ANALYZER_H
#define SPECTRUM_ANALYZER_H
// SpectrumAnalyzer - A class for analyzing audio spectrum.

#include <string>

class SpectrumAnalyzer {
  private:
    int sample_rate_;
    int update_size_;
    int process_size_;

    double *input_buffers_[2];
    double input_max_[2];
    int *work_area_;
    double *cos_sin_table_;

    int current_buffer_index_ = 0;
    int buffer_cur_ = 0;
    int current_result_index_ = 0;

    void (*result_callback_)(SpectrumAnalyzer *sa);

    std::string display_ = std::string();

    void calculate_fft(int buffer_index);
    void copy_with_mix(double *dest, const float *src, unsigned int frames, unsigned int channels);

  public:
    SpectrumAnalyzer(void (*result_callback)(SpectrumAnalyzer *sa));
    ~SpectrumAnalyzer();

    void initialize(unsigned int sample_rate, unsigned int updates_per_second);
    void process(const float *input_buffer, unsigned int frames, unsigned int channels);

    const std::string *render_display(int width, int height);
};

#endif // SPECTRUM_ANALYZER_H
