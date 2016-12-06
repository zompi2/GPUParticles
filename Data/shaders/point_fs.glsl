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