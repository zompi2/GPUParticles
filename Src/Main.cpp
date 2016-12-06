/**
 * Particles example.
 *
 * This is the main file of Particles example.
 *
 * This file is a part of a project from http://zompi.pl/gpu-particles
 * (c) 2014
 */

#include "Engine.h"

/**
 * Start the application with no console 
 */
int __stdcall wWinMain(HINSTANCE hInst, HINSTANCE prevInst, LPWSTR szCmdLine, int nCmdShow)
{
	// Init application engine so it can run
	ENGINE_INIT

	// As long as the engine is running update it
	while (ENGINE->IsRunning() == true)
	{
		ENGINE->Poll();
	}

	// When engine has been stopped clear the memory
	ENGINE_CLEAN

	// Exit application with no errors
	exit(EXIT_SUCCESS);
	return 0;
}