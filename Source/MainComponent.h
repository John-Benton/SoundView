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
	juce::Rectangle<int> display_window_visible_button_outline;

	juce::Rectangle<int> display_window_outline;

	juce::Rectangle<int> rta_outline;
	juce::Rectangle<int> rta_left_region;
	juce::Rectangle<int> rta_right_region;
	juce::Rectangle<int> rta_top_region;
	juce::Rectangle<int> rta_bottom_region;

	juce::Rectangle<int> spectrogram_outline;
	juce::Rectangle<int> spectrogram_left_region;
	juce::Rectangle<int> spectrogram_right_region;
	juce::Rectangle<int> spectrogram_top_region;
	juce::Rectangle<int> spectrogram_bottom_region;

	GLFWwindow *display_window;
	struct NVGcontext *nvg_context;

	int display_window_width, display_window_height;

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

		glClearColor(1.0, 1.0, 1.0, 1.0);

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

		rta_outline = display_window_outline.removeFromTop(display_window_outline.getHeight()*0.5);
		int rta_outline_width{ rta_outline.getWidth() }, rta_outline_height{ rta_outline.getHeight() };
		rta_left_region = rta_outline.removeFromLeft(rta_outline_width*0.05);
		rta_right_region = rta_outline.removeFromRight(rta_outline_width*0.05);
		rta_top_region = rta_outline.removeFromTop(rta_outline_height*0.10);
		rta_bottom_region = rta_outline.removeFromBottom(rta_outline_height*0.10);

		spectrogram_outline = display_window_outline;
		int spectrogram_outline_width{ spectrogram_outline.getWidth() }, spectrogram_outline_height{ spectrogram_outline.getHeight() };
		spectrogram_left_region = spectrogram_outline.removeFromLeft(spectrogram_outline_width*0.05);
		spectrogram_right_region = spectrogram_outline.removeFromRight(spectrogram_outline_width*0.05);
		spectrogram_top_region = spectrogram_outline.removeFromTop(spectrogram_outline_height*0.10);
		spectrogram_bottom_region = spectrogram_outline.removeFromBottom(spectrogram_outline_height*0.10);

		//==========//

		nvgStrokeWidth(ctx, 2);

		nvgStrokeColor(ctx, nvgRGBA(255, 255, 255, 255));
		
		render_juce_int_rect(ctx, rta_outline);
		render_juce_int_rect(ctx, rta_left_region);
		render_juce_int_rect(ctx, rta_right_region);
		render_juce_int_rect(ctx, rta_top_region);
		render_juce_int_rect(ctx, rta_bottom_region);
		
		render_juce_int_rect(ctx, spectrogram_outline);
		render_juce_int_rect(ctx, spectrogram_left_region);
		render_juce_int_rect(ctx, spectrogram_right_region);
		render_juce_int_rect(ctx, spectrogram_top_region);
		render_juce_int_rect(ctx, spectrogram_bottom_region);

		//==========//

		nvgEndFrame(ctx);

	}

	void render_text(NVGcontext *ctx, const char* text, int center_x_pix, int center_y_pix, int font_size_pix) {

		int font_x, font_y;

		float font_bounds[4];

		nvgFontSize(ctx, font_size_pix);

		nvgTextAlign(ctx, NVGalign::NVG_ALIGN_BOTTOM);

		nvgTextBounds(ctx, 0.0, 0.0, text, NULL, font_bounds);

		int font_width = font_bounds[2] - font_bounds[0];

		int font_height = font_bounds[3] - font_bounds[1];

		font_x = center_x_pix - (font_width / 2);

		font_y = center_y_pix + (font_height / 2);

		nvgText(ctx, font_x, font_y, text, NULL);

		//nvgBeginPath(ctx);

		//nvgRect(ctx, font_x, font_y - font_height, font_width, font_height);

		//nvgStroke(ctx);

	}

	void render_juce_int_rect(NVGcontext *ctx, juce::Rectangle<int> rect) {

		nvgBeginPath(ctx);

		nvgRect(ctx, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());

		nvgStroke(ctx);

	}

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
