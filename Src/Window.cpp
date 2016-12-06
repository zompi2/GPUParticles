#include "Window.h"

/**
* Particles example.
*
* This is a window class. It creates and handles the window with proper
* opengl context.
*
* This file is a part of a project from http://zompi.pl/gpu-particles
* (c) 2014 Damian Nowakowski
*/

/**
* Simple cosntructor
*/
Window::Window()
{
	// Just make sure that the handler is pointing to null
	glfwWindow = NULL;
}

/**
* Function that runs when the window was closed and stops the engine.
*/
void OnClose(GLFWwindow * thisWindow)
{
	ENGINE->StopEngine();
}

/**
* Initialize the window.
* It can't be used inside constructor because we need an information
* about window creation successful. When there was a problem the application
* has to be closed.
* @return tru if window and opengl context were created successfuly.
*/
bool Window::Init()
{
	// First try initialize glfw and return false when there was any problem
	// (it will cause stop of the application).
	if (glfwInit() == false)
	{
		return false;
	}

	// Remember the configuration reader so we can use it in the future.
	INIReader * localINIReader = ENGINE->config;

	// Set the hint that window will not be resizable (it is easier for us)
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
		
	/// Check in configuration ini file if application will be in fullscreen
	/// If yes remember the handler to the current monitor. If no, then the handler
	/// has to remain null.
	GLFWmonitor * currentMonitor = NULL;
	if (localINIReader->GetBoolean("Window", "Fullscreen", false) == true)
	{
		currentMonitor = glfwGetPrimaryMonitor();
	}

	/// Create window with size and name specified in configuration ini file.
	/// When current monitor is null the application will open in window not in the fullscreen.
	glfwWindow = glfwCreateWindow(
		localINIReader->GetInteger("Window", "Width", 640),
		localINIReader->GetInteger("Window", "Height", 780),
		localINIReader->Get("Window", "Title", "NoName").c_str(),
		currentMonitor,
		NULL
	);

	// Check if window was created correctly. If not clear the glfw
	// and return false (it will cause stop of the application).
	if (glfwWindow == NULL)
	{
		glfwTerminate();
		return false;
	}

	// Set the close the window callback. Closing the window will cause
	// stopping the aplication.
	glfwSetWindowCloseCallback(glfwWindow, OnClose);

	// Bind the opengl context to newly created window
	glfwMakeContextCurrent(glfwWindow);

	// Initialize glew. If there is any problem with glew
	// return false (it will cause stop of the application).
	if (glewInit() != GLEW_OK)
	{
		glfwTerminate();
		return false;
	}

	/// At the end enable depth test and blending in opengl
	/// and clear the error buffer.
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glGetError();

	// Say that the window has been successfully initialized.
	return true;
}

/**
* Simple destructor cleaning all data
*/
Window::~Window()
{
	if (glfwWindow != NULL)
	{
		glfwTerminate();
	}
}