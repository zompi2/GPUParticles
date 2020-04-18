#version 400

/**
 * Vertex shader used to draw a single particle point.
 * (c) 2014 Damian Nowakowski
 */

/**
* Use the location the same as in compute shader for
* easy getting attribute pointers address.
*/
layout(location = 1) in vec3 inPosition;
layout(location = 2) in vec4 inColor;

out vec4 inoutColor;

uniform mat4 viewProjectionMatrix;
uniform float pointSize;

void main()
{
	/// Simply pass the color next and set the vertex position and size.
	inoutColor = inColor;
	gl_Position = viewProjectionMatrix * vec4(inPosition, 1.0);
	gl_PointSize = pointSize;
}