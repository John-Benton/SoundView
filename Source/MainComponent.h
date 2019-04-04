#pragma once

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#define NANOVG_GL3_IMPLEMENTATION

#include <nanovg.h>
#include <nanovg_gl.h>

#include "../JuceLibraryCode/JuceHeader.h"

#include <deque>

#include "fft.h"
#include "moveavg.h"
#include "gl_shader.h"
#include "audio_performance.h"

#include "spline.h"

#include <chrono>
#include <assert.h>
#include <mutex>

class MainComponent   : public AudioAppComponent, public Button::Listener, public Timer, public Slider::Listener
{
public:

    MainComponent()
    {
		juce::Rectangle<int> screen = Desktop::getInstance().getDisplays().getMainDisplay().userArea; //Thank you Matthias Gehrmann
	    setSize (screen.getWidth()*0.20, screen.getWidth()*0.5);

        setAudioChannels (2, 2);

		setup_GL(screen.getWidth());

		startTimerHz(30);

		addAndMakeVisible(audio_device_selector_component);
		addAndMakeVisible(audio_performance_component);

		addAndMakeVisible(num_rta_averages_slider);
		num_rta_averages_slider.setRange(1.0, 100.0, 1.0);
		num_rta_averages_slider.setValue(20.0, dontSendNotification);
		num_rta_averages_slider.addListener(this);
		
		fft_sample_buffer.resize(fft_size);
		fft_bin_freqs.resize(fft_size / 2);
		fft_bin_amps.resize(fft_size / 2);

		generate_fft_bin_freq(fft_bin_freqs, fft_size);

		spectrogram_frequencies.resize(spectrogram_num_frequencies);
		spectrogram_amplitudes.resize(spectrogram_num_frequencies);

		generate_spectrogram_frequencies();

		fft_output_averager.set_num_averages(num_rta_averages_slider.getValue());
		fft_output_averager.set_num_samples(fft_bin_amps.size());
		
    } 

    ~MainComponent()
    {
        shutdownAudio();
		glfwTerminate();
    }

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override
    {
		if (active_sample_rate != sampleRate) {

			active_sample_rate = sampleRate;

			generate_fft_bin_freq(fft_bin_freqs, fft_size);

		}

    }

	void getNextAudioBlock(const AudioSourceChannelInfo& audio_device_buffer)
	{
	
		auto start = std::chrono::high_resolution_clock::now(); //Thanks to Giovanni Dicanio for timing method

		const float* device_input_buffer = audio_device_buffer.buffer->getReadPointer(0);

		std::vector<float> latest_device_samples(audio_device_buffer.numSamples);

		input_buffer_mtx.lock();

		for (int sample = 0; sample < audio_device_buffer.numSamples; ++sample) {

			input_sample_buffer.push_front(device_input_buffer[sample]);
			latest_device_samples[sample] = device_input_buffer[sample];

		}

		while (input_sample_buffer.size() > fft_size) {
			input_sample_buffer.pop_back();
		}

		input_buffer_mtx.unlock();

		audio_device_buffer.clearActiveBufferRegion();

		auto end = std::chrono::high_resolution_clock::now();

		std::chrono::duration<double> elapsed = end - start;

		double audio_time = elapsed.count() * 1000;

		callback_timer_mtx.lock();

		audio_callback_times.push_back(audio_time);
		while (audio_callback_times.size() > 100) { audio_callback_times.pop_back(); }

		callback_timer_mtx.unlock();

	}

    void releaseResources() override
    {

    }

    void paint (Graphics& g) override
    {

        g.fillAll (Colour(20,20,20));

		g.setColour(Colours::white);

		draw_divider(g, audio_device_selector_outline, 2, Colour(100, 100, 100));
		draw_divider(g, audio_performance_outline, 2, Colour(100, 100, 100));

		g.setFont(num_rta_averages_slider_label_outline.getHeight() * 0.75);
		g.drawText("RTA Averages", num_rta_averages_slider_label_outline, Justification::centred, false);

    }

	void draw_divider(Graphics& context, juce::Rectangle<int> rectangle_above_divider, int divider_height, Colour divider_color) {

		context.saveState();

		context.setColour(divider_color);

		context.fillRect(rectangle_above_divider.getBottomLeft().getX(),
			rectangle_above_divider.getBottomLeft().getY() - (divider_height/2),
			rectangle_above_divider.getWidth(),
			divider_height);

		context.restoreState();

	}

    void resized() override
    {
		control_window_outline = getLocalBounds();
		int control_window_height = control_window_outline.getHeight();

		audio_device_selector_outline = control_window_outline.removeFromTop(225);
		audio_device_selector_component.setBounds(audio_device_selector_outline);

		audio_performance_outline = control_window_outline.removeFromTop(control_window_height * 0.10);
		audio_performance_component.setBounds(audio_performance_outline);

		num_rta_averages_slider_label_outline = control_window_outline.removeFromTop(control_window_height * 0.025);
		num_rta_averages_slider.setBounds(control_window_outline.removeFromTop(control_window_height * 0.025));

    }

private:

	std::deque<float> input_sample_buffer;
	std::mutex input_buffer_mtx, callback_timer_mtx;

	const int fft_size = 16384;
	int active_sample_rate = 44100;
	fft fft0{ fft_size };
	std::vector<float> fft_bin_freqs;
	std::vector<float> fft_bin_amps;

	double dBFS_lower_limit = -96.0;

	AudioDeviceSelectorComponent audio_device_selector_component{ this->deviceManager, 1,1,0,0,0,0,0,0 };

	std::vector<double> fft_sample_buffer;
	MovingAverage fft_output_averager;
	AudioPeformanceEngine audio_performance_engine{1};
	AudioPerformanceComponent audio_performance_component;
	std::vector<float> audio_performance_buffer;
	std::deque<float> audio_callback_times;
	
	juce::Rectangle<int> control_window_outline;
	juce::Rectangle<int> audio_device_selector_outline;
	juce::Rectangle<int> audio_performance_outline;

	juce::Rectangle<int> display_window_outline;

	juce::Rectangle<int> rta_outline, frequency_label_outline, spectrogram_outline;

	juce::Rectangle<int> num_rta_averages_slider_label_outline;
	Slider num_rta_averages_slider;
	int num_rta_averages_slider_value;

	//====================//

	GLFWwindow *display_window;
	struct NVGcontext *nvg_context;

	int gl_success{ 0 };
	char gl_infolog[512];

	unsigned int gl_texture;

	unsigned int VAO[2], VBO[2];

	int gl_shader_program;

	std::deque<unsigned char> spectrogram_texture_pixel_values;

	int spectrogram_num_frequencies = 1024; //must be a multiple of 4
	int spectrogram_num_past_rows = 256;

	std::vector<float> spectrogram_frequencies, spectrogram_amplitudes;
	
	//====================//

	int display_window_width, display_window_height;

	std::vector<int> frequency_label_values{20,50,100,500,1000,5000,10000,20000}; //the first and last values also control the upper/lower bounds of the display
	
	std::vector<int> frequency_gridlines{	20,30,40,50,60,70,80,90,100,
											200,300,400,500,600,700,800,900,1000,
											2000,3000,4000,5000,6000,7000,8000,9000,10000,
											20000 };

	std::vector<int> rta_amplitude_gridlines{0,-12,-24,-36,-48,-60,-72,-84,-96}; //in dBFS

	tk::spline cubic_interpolator;
			
	void timerCallback() override
	{

		input_buffer_mtx.lock();

		std::copy(input_sample_buffer.begin(), input_sample_buffer.begin() + (fft0.local_fft_size), fft0.fft_input_samples);
		
		if (audio_performance_buffer.size() != input_sample_buffer.size()) {

			audio_performance_buffer.resize(input_sample_buffer.size());

		}

		std::copy(input_sample_buffer.begin(), input_sample_buffer.end(), audio_performance_buffer.begin());

		input_buffer_mtx.unlock();

		run_performance_calcs();
						
		fft0.run_fft_analysis();

		get_fft_amplitudes(fft0.fftw_complex_out, fft_bin_amps);

		update_averages();

		render_display();

	}

	void run_performance_calcs() {

		audio_performance_component.set_ape_analysis_results(audio_performance_engine.analyse_samples(audio_performance_buffer));

		callback_timer_mtx.lock();

		float sum_callback_times = std::accumulate(audio_callback_times.begin(), audio_callback_times.end(), 0.0);
		audio_performance_component.set_indicated_callback_time(sum_callback_times / (audio_callback_times.size()*1.0));
		audio_performance_component.repaint();

		callback_timer_mtx.unlock();

	}

	void sliderValueChanged(Slider* slider) override
		
	{
		if (slider = &num_rta_averages_slider) {

			num_rta_averages_slider_value = num_rta_averages_slider.getValue();

			fft_output_averager.set_num_averages(num_rta_averages_slider_value);

		}
		
		
	}

	void buttonClicked(Button* button) override 
	{
		
	}

	void generate_fft_bin_freq(std::vector<float> &freq_vect, int fft_size_N) {

		freq_vect[0] = 0.0;

		for (int x = 1; x < fft_size_N / 2; x++) {

			freq_vect[x] = x * (active_sample_rate * 1.0 / fft_size_N * 1.0);

		}

	}

	void generate_spectrogram_frequencies() {

		//this function generates log spaced frequencies based on the limits of frequency_label_values and resolution of the spectrogram.
		//these log spaced frequencies will be used to generate the spectrogram texture, which will align correctly with the log frequency 
		//axis of the display

		int lowest_freq = frequency_label_values.front();
		int highest_freq = frequency_label_values.back();

		float log_lowest_freq = log10(lowest_freq);
		float log_higest_freq = log10(highest_freq);

		float log_freq_range = log_higest_freq - log_lowest_freq;

		int num_freq = spectrogram_num_frequencies;

		for (int x = 0; x < num_freq; x++) {

			float exponent = log_lowest_freq + (log_freq_range)*((x*1.0) / (num_freq - 1.0));

			spectrogram_frequencies[x] = pow(10, exponent);

		}
		
	}

	void get_fft_amplitudes(std::vector<std::vector<double>> &fft_output_complex, std::vector<float> &amplitude_vector)
	{

		for (int x = 0; x < amplitude_vector.size(); x++) {

			amplitude_vector[x] = sqrtf((pow(fft_output_complex[0][x],2)) + (pow(fft_output_complex[1][x], 2)));

		}

	}

	void update_averages() {

		fft_output_averager.add_new_samples(fft_bin_amps);

	}

	void update_spectrogram_texture() {

		std::vector<double> interpolator_ref_freq, interpolator_ref_amp;
		interpolator_ref_freq.resize(fft_bin_freqs.size());
		interpolator_ref_amp.resize(fft_bin_amps.size());

		std::copy(fft_bin_freqs.begin(), fft_bin_freqs.end(), interpolator_ref_freq.begin());
		std::copy(fft_bin_amps.begin(), fft_bin_amps.end(), interpolator_ref_amp.begin());

		cubic_interpolator.set_points(interpolator_ref_freq, interpolator_ref_amp);

		std::vector<float> interpolated_amplitudes;
		interpolated_amplitudes.resize(spectrogram_num_frequencies);

		for (int frequency = 0; frequency < spectrogram_num_frequencies; frequency++) {

			interpolated_amplitudes[frequency] = cubic_interpolator(spectrogram_frequencies[frequency]);

		}

		int total_pixels = spectrogram_num_frequencies * spectrogram_num_past_rows;

		if (spectrogram_texture_pixel_values.size() != total_pixels) {

			spectrogram_texture_pixel_values.clear();

			spectrogram_texture_pixel_values.resize(spectrogram_num_frequencies * spectrogram_num_past_rows);

		}

		for (int texture_pixel = 0; texture_pixel < spectrogram_num_frequencies; texture_pixel++) {

			float frequency = spectrogram_frequencies[texture_pixel];

			float value_dBFS = fft_amp_to_dBFS(cubic_interpolator(frequency));

			int value_pixel = (((value_dBFS - (-96.0)) * 255) / 96);

			spectrogram_texture_pixel_values.push_back(value_pixel);

		}

		while (spectrogram_texture_pixel_values.size() > total_pixels) {

			spectrogram_texture_pixel_values.pop_front();

		}
		
	}

	void setup_GL(int screen_width) {

		glfwInit();

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		display_window = glfwCreateWindow(screen_width*0.5, screen_width*0.5, "SoundView Display", NULL, NULL);

		if (display_window == NULL) {

			AlertWindow::showMessageBox(AlertWindow::AlertIconType::WarningIcon, "ERROR", "GLFW could not set up a window", "OK", NULL);

		}

		glfwMakeContextCurrent(display_window);

		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {

			AlertWindow::showMessageBox(AlertWindow::AlertIconType::WarningIcon, "ERROR", "GLAD could not be initialized", "OK", NULL);

		}

		Shader glShader{
			"D://Active//SoundView//Source//vertex_shader.vert",
			"D://Active//SoundView//Source//fragment_shader.frag"
		};

		gl_shader_program = glShader.ID;

		//==========//
		
		glGenTextures(1, &gl_texture);
		glBindTexture(GL_TEXTURE_2D, gl_texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		//==========//

		glGenVertexArrays(2, VAO);
		glGenBuffers(2, VBO);
		
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); //this will configure OpenGL to render in wireframe mode
		
		//==//

		nvg_context = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);

		nvgCreateFont(nvg_context, "Arial", "C://Windows//Fonts//arial.ttf");
		
	}

	void render_display() {

		if (glfwWindowShouldClose(display_window))
		{
			JUCEApplicationBase::quit();
		}

		glfwGetWindowSize(display_window, &display_window_width, &display_window_height);

		glViewport(0, 0, display_window_width, display_window_height);

		calc_layout();

		glUseProgram(gl_shader_program);

		float triangle1[] = {
			-1.0f, -1.0f, 0.0f,		0.0f,0.0f,  // bottom left
			-1.0f, 0.0f, 0.0f,		0.0f,1.0f,	// top left
			1.0f, 0.0f, 0.0f,		1.0f,1.0f	// top right
		};

		float triangle2[] = {
			-1.0f, -1.0f, 0.0f,		0.0f,0.0f,  // bottom left
			1.0f, 0.0f, 0.0f,		1.0f,1.0f,	// top right
			1.0f, -1.0f, 0.0f,		1.0f,0.0f	// bottom right
		};

		glBindVertexArray(VAO[0]);
		glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(triangle1), triangle1, GL_DYNAMIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		glBindVertexArray(VAO[1]);
		glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(triangle2), triangle2, GL_DYNAMIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		update_spectrogram_texture();
		
		int spectrogram_texture_pixel_count = spectrogram_num_frequencies * spectrogram_num_past_rows;

		unsigned char* spectrogram_texture_pixels;
		spectrogram_texture_pixels = (unsigned char*) malloc(spectrogram_texture_pixel_count * sizeof(unsigned char));

		std::copy(spectrogram_texture_pixel_values.begin(), spectrogram_texture_pixel_values.end(), spectrogram_texture_pixels);
		
		glBindTexture(GL_TEXTURE_2D, gl_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, spectrogram_num_frequencies, spectrogram_num_past_rows, 0, GL_RED, GL_UNSIGNED_BYTE, spectrogram_texture_pixels);
		glGenerateMipmap(GL_TEXTURE_2D);

		free(spectrogram_texture_pixels);

		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);

		glBindTexture(GL_TEXTURE_2D, gl_texture);
						
		glBindVertexArray(VAO[0]);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glBindVertexArray(VAO[1]);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		nvg_render(nvg_context);
		
		glfwSwapBuffers(display_window);

	}

	void calc_layout() {

		display_window_outline = juce::Rectangle<int>{ 0, 0, display_window_width, display_window_height };

		rta_outline = display_window_outline.removeFromTop(display_window_outline.getHeight()*0.48);

		frequency_label_outline = display_window_outline.removeFromTop(display_window_outline.getHeight()*0.04);

		spectrogram_outline = display_window_outline;

	}

	void nvg_render(NVGcontext *ctx)
	{
		nvgBeginFrame(ctx, display_window_width, display_window_height, 1.0f);

		//==========//

		nvgStrokeWidth(ctx, 2);

		nvgStrokeColor(ctx, nvgRGBA(255, 127, 0, 255));
		
		//render_juce_int_rect(ctx, rta_outline);

		//render_juce_int_rect(ctx, frequency_label_outline);
		
		//render_juce_int_rect(ctx, spectrogram_outline);

		//////////

		int font_size = frequency_label_outline.getHeight() * 0.8;

		char int_string[33];

		memset(int_string, 0, sizeof int_string);

		itoa(frequency_label_values.front(), int_string, 10);

		render_text(ctx, int_string, frequency_label_outline.getX() + 5, frequency_label_outline.getCentreY(), font_size, 1, FALSE);

		memset(int_string, 0, sizeof int_string);

		itoa(frequency_label_values.back(), int_string, 10);

		render_text(ctx, int_string, frequency_label_outline.getTopRight().getX() - 5, frequency_label_outline.getCentreY(), font_size, 2, FALSE);

		for (int x = 1; x < frequency_label_values.size() - 1; x++) {

			memset(int_string, 0, sizeof int_string);

			itoa(frequency_label_values[x], int_string, 10);

			render_text(ctx, 
						int_string, 
						frequency_label_outline.getX() + frequency_label_outline.getWidth() * frequency_to_x_proportion(frequency_label_values[x]), 
						frequency_label_outline.getCentreY(), 
						font_size,
						0, 
						FALSE);

		}

		//////////

		for (int x = 0; x < frequency_gridlines.size(); x++)
		{

			nvgStrokeWidth(ctx, 1);

			nvgStrokeColor(ctx, nvgRGBA(40, 40, 40, 255));
			
			nvgBeginPath(ctx);

			nvgMoveTo(ctx, rta_outline.getX() + rta_outline.getWidth() * frequency_to_x_proportion(frequency_gridlines[x]), rta_outline.getY());

			nvgLineTo(ctx, rta_outline.getX() + rta_outline.getWidth() * frequency_to_x_proportion(frequency_gridlines[x]), rta_outline.getBottomLeft().getY());

			nvgStroke(ctx);

		}

		//////////

		for (int x = 0; x < rta_amplitude_gridlines.size(); x++)
		{

			nvgStrokeWidth(ctx, 1);

			nvgStrokeColor(ctx, nvgRGBA(40, 40, 40, 255));

			nvgBeginPath(ctx);

			nvgMoveTo(ctx, rta_outline.getX(), rta_outline.getY() + rta_outline.getHeight() * rta_dBFS_to_y_proportion(rta_amplitude_gridlines[x]));

			nvgLineTo(ctx, rta_outline.getTopRight().getX(), rta_outline.getY() + rta_outline.getHeight() * rta_dBFS_to_y_proportion(rta_amplitude_gridlines[x]));

			nvgStroke(ctx);

		}

		//////////

		nvgStrokeWidth(ctx, 1);

		nvgStrokeColor(ctx, nvgRGBA(255, 127, 0, 255));

		nvgBeginPath(ctx);

		std::vector<float> rta_amplitudes = fft_output_averager.get_average();

		nvgMoveTo(	ctx, 
					rta_outline.getX() + rta_outline.getWidth() * frequency_to_x_proportion(fft_bin_freqs[1]),
					rta_outline.getY() + rta_outline.getHeight() * rta_dBFS_to_y_proportion(fft_amp_to_dBFS(rta_amplitudes[1])));

		for (int x = 2; x < rta_amplitudes.size(); x++)
		{
						
			nvgLineTo(ctx,
				rta_outline.getX() + rta_outline.getWidth() * frequency_to_x_proportion(fft_bin_freqs[x]),
				rta_outline.getY() + rta_outline.getHeight() * rta_dBFS_to_y_proportion(fft_amp_to_dBFS(rta_amplitudes[x])));

		}

		nvgStroke(ctx);

		//==========//

		nvgEndFrame(ctx);

	}

	void render_text(	NVGcontext *ctx, const char* text, int pos_x_pix, int pos_y_pix, 
						int font_size_pix, int positioning, bool show_bounding_box) {
		
		//Positioning values are 0 = center, 1 = middle left edge, 2 = middle right edge

		int font_x, font_y;

		float font_bounds[4];

		nvgFontSize(ctx, font_size_pix);

		nvgTextAlign(ctx, NVGalign::NVG_ALIGN_BOTTOM);

		nvgTextBounds(ctx, 0.0, 0.0, text, NULL, font_bounds);

		int font_width = font_bounds[2] - font_bounds[0];

		int font_height = font_bounds[3] - font_bounds[1];

		if (positioning == 0) {

			font_x = pos_x_pix - (font_width / 2);

			font_y = pos_y_pix + (font_height / 2);

		}

		if (positioning == 1) {

			font_x = pos_x_pix;

			font_y = pos_y_pix + (font_height / 2);

		}

		if (positioning == 2) {

			font_x = pos_x_pix - font_width;

			font_y = pos_y_pix + (font_height / 2);

		}

		nvgText(ctx, font_x, font_y, text, NULL);

		if (show_bounding_box == true) {

			nvgStrokeWidth(ctx, 1);

			nvgStrokeColor(ctx, nvgRGBA(255, 255, 255, 255));

			nvgBeginPath(ctx);

			nvgRect(ctx, font_x, font_y - font_height, font_width, font_height);

			nvgStroke(ctx);

		}
		
	}

	void render_juce_int_rect(NVGcontext *ctx, juce::Rectangle<int> rect) {

		nvgBeginPath(ctx);

		nvgRect(ctx, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());

		nvgStroke(ctx);

	}

	float frequency_to_x_proportion(float frequency)
	{

		float min_offset = log10f(frequency_label_values.front());
		float max_offset = log10f(frequency_label_values.back());
		float offset_range = max_offset - min_offset;

		float freq_offset = log10f(frequency) - min_offset;

		return freq_offset / offset_range;

	}

	float rta_dBFS_to_y_proportion(float dBFS)
	{

		float offset_range = rta_amplitude_gridlines.front() - rta_amplitude_gridlines.back();

		float amplitude_offset = abs(dBFS) - rta_amplitude_gridlines.front();

		return amplitude_offset / offset_range;

	}

	float fft_amp_to_dBFS(double amp) {

		return Decibels::gainToDecibels(amp, dBFS_lower_limit);

	}

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
