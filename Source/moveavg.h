
#pragma once
#include <vector>
#include <deque>
#include <numeric>

class MovingAverage
{
public:

	MovingAverage() {};

	~MovingAverage() {};

	void set_num_averages(int num_averages) {

		averages = num_averages;
		
	}

	void set_num_samples(int num_samples) {

		samples = num_samples;
		averaging_buffer.resize(samples);

	}
	
	void add_new_samples(std::vector<float> &input_samples) {

		for (int x = 0; x < averaging_buffer.size(); x++) {

			averaging_buffer[x].push_front(input_samples[x]);

			while (averaging_buffer[x].size() > averages) {

				averaging_buffer[x].pop_back();

			}

		}

	}

	std::vector<float> get_average() {

		std::vector<float> output_buffer;

		output_buffer.resize(samples);

		for (int x = 0; x < output_buffer.size(); x++) {

			output_buffer[x] = std::accumulate(averaging_buffer[x].begin(), averaging_buffer[x].end(), 0.0) / (averages*1.0);

		}

		return output_buffer;

	}

private:

	std::vector<std::deque<float>> averaging_buffer;

	int averages; //number of averages

	int samples; //number of samples

};