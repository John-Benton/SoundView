
#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include <deque>

struct AudioPerformanceTextIndicator
{

	juce::Rectangle<int> indicator_outline;
	juce::Rectangle<int> indicator_active_area;
	juce::Rectangle<int> indicator_label_area;
	juce::Rectangle<int> indicator_value_area;

	String indicator_label_text{ "NO_LABEL_TEXT" };
	String indicator_value{ "" };

	void set_indicator_outline(juce::Rectangle<int> outline_rectangle) {

		indicator_outline = outline_rectangle;

		indicator_active_area = indicator_outline.reduced(2);
		indicator_label_area = indicator_active_area.removeFromLeft(indicator_active_area.getWidth()*0.75);
		indicator_value_area = indicator_active_area;

	}

	void draw_indicator(Graphics& context) {

		context.saveState();

		context.setColour(Colours::white);

		context.setFont(indicator_label_area.getHeight()*0.75);

		context.drawText(indicator_label_text, indicator_label_area, Justification::centred, false);
		context.drawText(String(indicator_value), indicator_value_area, Justification::centred, false);

		context.restoreState();

	}
	
};

class AudioPerformanceComponent: public Component
{
public:
	
	AudioPerformanceComponent() {
	
		indicator_1.indicator_label_text = "Zeroed Samples";
		indicator_2.indicator_label_text = "Clipped Samples";
		indicator_3.indicator_label_text = "Invalid Samples";
		indicator_4.indicator_label_text = "Audio Callback Time (ms)";
		indicator_5.indicator_label_text = "Total Audio Over/Underruns";
	
	};
	
	~AudioPerformanceComponent() {};

	void paint(Graphics& g) override
	{

		g.setColour(Colours::white);

		indicator_1.indicator_value = String(ape_analysis_results[0]);
		indicator_1.draw_indicator(g);
		
		indicator_2.indicator_value = String(ape_analysis_results[1]);
		indicator_2.draw_indicator(g);
		
		indicator_3.indicator_value = String(ape_analysis_results[2]);
		indicator_3.draw_indicator(g);

		indicator_4.indicator_value = String(indicated_audio_callback_time);
		indicator_4.draw_indicator(g);

		indicator_5.indicator_value = String(indicated_xruns);
		indicator_5.draw_indicator(g);

	}

	void resized() override
	{

		component_outline = getLocalBounds();
		int component_height = component_outline.getHeight();

		component_outline.removeFromTop(component_height*0.05);
		
		indicator_1.set_indicator_outline(component_outline.removeFromTop(component_height*0.18));
		indicator_2.set_indicator_outline(component_outline.removeFromTop(component_height*0.18));
		indicator_3.set_indicator_outline(component_outline.removeFromTop(component_height*0.18));
		indicator_4.set_indicator_outline(component_outline.removeFromTop(component_height*0.18));
		indicator_5.set_indicator_outline(component_outline.removeFromTop(component_height*0.18));

		component_outline.removeFromTop(component_height*0.05);

	}

	void set_ape_analysis_results(std::vector<int> analysis_results) {

		assert(analysis_results.size() == 3);

		ape_analysis_results = analysis_results;

	}

	void set_indicated_callback_time(float callback_time) {

		indicated_audio_callback_time = callback_time;

	}

	void set_indicated_xruns(int xruns) {

		indicated_xruns = xruns;

	}
		
private:

	std::vector<int> ape_analysis_results{0,0,0};
	float indicated_audio_callback_time{ 0.0 };
	int indicated_xruns{ 0 };

	juce::Rectangle<int> component_outline;
	AudioPerformanceTextIndicator indicator_1;
	AudioPerformanceTextIndicator indicator_2;
	AudioPerformanceTextIndicator indicator_3;
	AudioPerformanceTextIndicator indicator_4;
	AudioPerformanceTextIndicator indicator_5;

};

class AudioPeformanceEngine
{
public:

	AudioPeformanceEngine(int num_periods) {
	
		num_sampling_periods = num_periods;
		zeroed_samples.resize(num_sampling_periods);
		clipped_samples.resize(num_sampling_periods);
		invalid_samples.resize(num_sampling_periods);
	
	};

	~AudioPeformanceEngine() {};

	std::vector<int> analyse_samples(std::vector<float> &input_samples) {

		std::vector<int> sample_analysis_results;
		sample_analysis_results.resize(3);

		//sample_analysis_results[0] indicates sum of zeroed samples over all sampling periods 
		//sample_analysis_results[1] indicates sum of clipped samples over all sampling periods
		//sample_analysis_results[2] indicates sum of invalid samples over all sampling periods

		check_samples(input_samples);

		sample_analysis_results[0] = std::accumulate(zeroed_samples.begin(), zeroed_samples.end(), 0);
		sample_analysis_results[1] = std::accumulate(clipped_samples.begin(), clipped_samples.end(), 0);
		sample_analysis_results[2] = std::accumulate(invalid_samples.begin(), invalid_samples.end(), 0);

		return sample_analysis_results;
				
	}

private:

	std::deque<int> zeroed_samples, clipped_samples, invalid_samples; //sum of all sampling periods;

	int num_sampling_periods;

	void check_samples(std::vector<float> samples) {

		int sum_zeroed_samples{ 0 }, sum_clipped_samples{ 0 }, sum_invalid_samples{ 0 };

		for (int x = 0; x < samples.size(); x++) {
			
			if (samples[x] == 0.0) {

				sum_zeroed_samples++;

			}

			if (samples[x] == -1.0 || samples[x] == 1.0) {

				sum_clipped_samples++;

			}

			if (samples[x] < -1.0 || samples[x] > 1.0) {

				sum_invalid_samples++;

			}

		}

		zeroed_samples.push_front(sum_zeroed_samples);
		while (zeroed_samples.size() > num_sampling_periods) { zeroed_samples.pop_back(); }

		clipped_samples.push_front(sum_clipped_samples);
		while (clipped_samples.size() > num_sampling_periods) { clipped_samples.pop_back(); }

		invalid_samples.push_front(sum_invalid_samples);
		while (invalid_samples.size() > num_sampling_periods) { invalid_samples.pop_back(); }
		
	}

};