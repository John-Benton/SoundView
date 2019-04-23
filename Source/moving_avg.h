
#pragma once

#include <vector>
#include <numeric>

class MovingAverageSmoother
{
public:
	
	MovingAverageSmoother()
	{
	}

	~MovingAverageSmoother()
	{
	}

	std::vector<float> process_samples(std::vector<float> &input_samples, int window_type, int window_size) {

		update_parameters(window_type, window_size);

		if (window_size == 1) {

			return input_samples;

		}

		else {

			process(input_samples);

			return output_buffer;

		}
	}

private:

	std::vector<float> processing_buffer;
	std::vector<float> output_buffer;

	std::vector<float> filter_kernel{}; 

	int active_window_size{ 3 }; //must be odd integer >= 3
	int active_window_type{ 0 };

	void update_parameters(int window_type, int window_size) {

		if (active_window_size != window_size) {

			active_window_size = window_size;

			if ((active_window_size % 2) == 0) {

				active_window_size -= 1; //make sure window size is odd

			}

			if (active_window_size < 3) {

				active_window_size = 3; //make sure window size is at least 3

			}

			calc_window();
			
		}

		if(active_window_type != window_type){
		
			active_window_type = window_type;

			calc_window();
		
		}
				
	}

	void calc_window() {

		filter_kernel.clear();

		filter_kernel.resize(active_window_size);

		int N = filter_kernel.size() - 1;

		switch (active_window_type) //See https://en.wikipedia.org/wiki/Window_function
		{
		
		case 1: //Triangle window

			for (int n = 0; n < filter_kernel.size(); n++) {
				filter_kernel[n] = 1.0 - abs((n - (N / 2.0)) / (N / 2.0));
			}

			break;

		case 2: //Hann window

			for (int n = 0; n < filter_kernel.size(); n++) {
				filter_kernel[n] = pow(sin(((3.14159*n) / (N))), 2.0);
			}

			break;

		default: // Rectangular window

			for (int n = 0; n < filter_kernel.size(); n++) {
				filter_kernel[n] = 1.0;
			}

			break;

		}

		float sum_weights = std::accumulate(filter_kernel.begin(), filter_kernel.end(), 0.0);

		for (int n = 0; n < filter_kernel.size(); n++) { //renormalize filter weights to sum to 1

			filter_kernel[n] /= sum_weights;
			
		}

	}

	//template <class data>
	//data process(data samples) {

	//	processing_buffer.resize(samples.size());

	//	std::copy(samples.begin(), samples.end(), processing_buffer.begin());

	//}

	void process(std::vector<float> samples) {

		output_buffer.resize(samples.size());

		int kernel_overhang = (filter_kernel.size() - 1) / 2;

		int needed_processing_buffer_size = samples.size() + (kernel_overhang * 2);

		processing_buffer.resize(needed_processing_buffer_size);

		std::copy(samples.begin(), samples.end(), processing_buffer.begin() + kernel_overhang);

		std::fill(	processing_buffer.begin(), 
					processing_buffer.begin() + kernel_overhang, 
					processing_buffer[kernel_overhang]);

		std::fill(	processing_buffer.end() - kernel_overhang,
					processing_buffer.end(),
					processing_buffer[kernel_overhang + samples.size() - 1]);

		int kernel_size = filter_kernel.size();
		int num_samples = samples.size();

		for (int output_sample = 0; output_sample < num_samples; output_sample++) {

			float convolution_sum{ 0.0 };

			for (int index = 0; index < kernel_size; index++) {
				
				convolution_sum += 
					
				processing_buffer[output_sample + index] *
					
				filter_kernel[(kernel_size - 1) - index];
			
			}

			output_buffer[output_sample] = convolution_sum;

		}

	}

};