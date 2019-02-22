#pragma once

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#define NANOVG_GL3_IMPLEMENTATION

#include <nanovg.h>
#include <nanovg_gl.h>

#include "../JuceLibraryCode/JuceHeader.h"

class MainComponent   : public AudioAppComponent, public Button::Listener, public Timer
{
public:

    MainComponent()
    {
		juce::Rectangle<int> screen = Desktop::getInstance().getDisplays().getMainDisplay().userArea; //Thank you Matthias Gehrmann
	    setSize (screen.getWidth()*0.20, screen.getWidth()*0.5);

        setAudioChannels (2, 2);

		setup_GL(screen.getWidth());

		startTimerHz(30);

    } 

    ~MainComponent()
    {
        shutdownAudio();
		glfwTerminate();
    }

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override
    {

    }

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {

        bufferToFill.clearActiveBufferRegion();
    }

    void releaseResources() override
    {

    }

    void paint (Graphics& g) override
    {

        g.fillAll (Colour(20,20,20));

    }

    void resized() override
    {
		control_window_outline = getLocalBounds();
		int control_window_height = control_window_outline.getHeight();

    }

private:

	juce::Rectangle<int> control_window_outline;

	juce::Rectangle<int> display_window_outline;

	juce::Rectangle<int> rta_outline, frequency_label_outline, spectrogram_outline;

	GLFWwindow *display_window;
	struct NVGcontext *nvg_context;

	int display_window_width, display_window_height;

	std::vector<int> frequency_label_values{20,50,100,500,1000,5000,10000, 20000}; //the first and last values also control the upper/lower bounds of the display
	
	std::vector<int> frequency_gridlines{	20,30,40,50,60,70,80,90,100,
											200,300,400,500,600,700,800,900,1000,
											2000,3000,4000,5000,6000,7000,8000,9000,10000,
											20000 };

	std::vector<int> rta_amplitude_gridlines{0,-10,-20,-30,-40,-50,-60,-70,-80,-90,-100 }; //in dBFS
											
	void timerCallback() override
	{

		render_display();

	}

	void buttonClicked(Button* button) override 
	{
		
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

		nvg_context = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);

		nvgCreateFont(nvg_context, "Arial", "C://Windows//Fonts//arial.ttf");
		
	}

	void render_display() {

		if (glfwWindowShouldClose(display_window))
		{
			JUCEApplicationBase::quit();
		}

		glClearColor(0.0, 0.0, 0.0, 1.0);

		glClear(GL_COLOR_BUFFER_BIT);

		glfwGetWindowSize(display_window, &display_window_width, &display_window_height);

		glViewport(0, 0, display_window_width, display_window_height);

		nvg_render(nvg_context);

		glfwSwapBuffers(display_window);

	}

	void nvg_render(NVGcontext *ctx)
	{
		nvgBeginFrame(ctx, display_window_width, display_window_height, 1.0f);

		//==========//

		display_window_outline = juce::Rectangle<int>{ 0, 0, display_window_width, display_window_height };

		rta_outline = display_window_outline.removeFromTop(display_window_outline.getHeight()*0.48);

		frequency_label_outline = display_window_outline.removeFromTop(display_window_outline.getHeight()*0.04);

		spectrogram_outline = display_window_outline;

		//////////

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

	float frequency_to_x_proportion(int frequency)
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
