/**
 * Fragment shader used to draw a single particle point.
 * (c) 2014 Damian Nowakowski
 */

#version 400

out vec4 outColor;
in vec4 inoutColor;

void main()
{
	// Discard the barely visible particles to avoid artefacts.
	if (inoutColor.a < 0.01)
	{
		discard;
	}

    outColor = inoutColor;
}