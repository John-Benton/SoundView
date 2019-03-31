
#pragma once

#include <iostream>
#include <string>
#include <algorithm>
#include <fftw3.h>
#include <array>
#include <vector>
#include <deque>
#include <assert.h>

//Complex arrays use the following format: [Re][0], [Imag][1]

//Complex vectors use the following format: [0][Re], [1][Imag]

class fft
{

public:

	const int local_fft_size;
	int local_fft_bins;

	std::vector<double> hann_window_weights;

	/*---------------------------------------------------*/

	double *fft_input_samples;

	fftw_complex *out;

	fftw_plan plan;

	/*---------------------------------------------------*/

	std::vector<std::vector<double>> fftw_complex_out;

	/*---------------------------------------------------*/

	fft(int fft_size) : local_fft_size(fft_size)
	{
		local_fft_bins = (local_fft_size / 2) + 1;

		/*---------------------------------------------------*/ //set sizes of vectors

		hann_window_weights.resize(local_fft_size);
	
		fftw_complex_out.resize(2, std::vector<double>(local_fft_bins)); //two rows, each row has as many columns as there are fft bins

		/*---------------------------------------------------*/

		generate_hann_window_weights_array();

		/*---------------------------------------------------*/

		fft_input_samples = new double[local_fft_size];

		out = fftw_alloc_complex(local_fft_size);
		plan = fftw_plan_dft_r2c_1d(local_fft_size, fft_input_samples, out, FFTW_MEASURE);

	};

	~fft() {

		fftw_destroy_plan(plan);
		fftw_free(out);
		delete[]fft_input_samples;

	};

	void run_fft_analysis() {

		run_fftw(fftw_complex_out);

		normalize_fftw_output();

	}

private:
	
	void generate_hann_window_weights_array() {

		for (int n = 0; n < local_fft_size; n++) {
			hann_window_weights[n] = 0.5*(1 - cos((2 * 3.141592654*n) / (local_fft_size - 1)));
		}

	}

	void run_fftw(std::vector<std::vector<double>> & destinaton_vector) {
		
		for (int n = 0; n < local_fft_size; n++) {
			fft_input_samples[n] = fft_input_samples[n] * hann_window_weights[n];
		}

		fftw_execute(plan);

		for (int row = 0; row < 2; row++) {
			for (int col = 0; col < local_fft_bins; col++) {
				destinaton_vector[row][col] = out[col][row];
			}
		}
	}

	void normalize_fftw_output() {

		for (int col = 0; col < local_fft_bins; col++) {

			fftw_complex_out[0][col] = fftw_complex_out[0][col] / local_fft_size;
			fftw_complex_out[1][col] = fftw_complex_out[1][col] / local_fft_size;

		}

	}

};