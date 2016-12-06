#version 400

/**
* These are input variables with arranged locations.
* Others.x = time life left.
* Others.y = 1-particle was emitted, 0-particle is waiting for it's emission.
*/
layout(location = 1) in vec3 inPosition;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec3 inVelocity;
layout(location = 4) in vec2 inOthers;

/**
* Output variables (the same order as input variables!)
*/
out vec3 outPosition;
out vec4 outColor;
out vec3 outVelocity;
out vec2 outOthers;

/**
* This is uniform layout storing all needed particle parameters.
* Using this layout application will update the uniforms.
*/
layout( shared ) uniform ParticlesParams
{
	float	deltaTime;
	vec3	emitterPosition;
	int		particlesEmitted;
	float	emitterRotation;
    float	lifeTime;
	float	emitterRadius;
	float	emitterSpread;
	float	colorSaturation;
	float	speed;
	float	gravity;
};

/**
* Variable for seting the random seed for random numbers
* pseudo generator.
*/
uint randSeed;

/**
* 2*PI/3 = 120 degrees (because there are three streams on the circle edge)
*/
const float D120 = 2.09439510;

/**
* Random float number pseudo generator.
* It returns a float number <0;b>
*/
float randhash(float b)
{
    uint i=(randSeed^12345391u)*2654435769u;
    i^=(i<<6u)^(i>>26u);
    i*=2654435769u;
    i+=(i<<5u)^(i>>12u);
	randSeed = i;
    return float(b * i) * 0.000000000232830643;
}


void main()
{
	/// First of all set the default output values
	outPosition		= inPosition;
    outColor		= inColor;
	outVelocity		= inVelocity;
	outOthers		= inOthers;


	// If particle is not alive
	if (outOthers.x <= 0)
	{
		// Dead particle has to have 0 color, so it will be invisible.
		outColor = vec4(0);

		// Can this particle be emitted?
		if (particlesEmitted > gl_VertexID)
		{
			// Hasn't this particle been already emitted
			if (outOthers.y == 0)
			{
				// Particle can be emitted, because the emission counter is higher than the particle id.
				// It also wasn't emitted yet, we know it from outOthers.y value.
				// Emit the particle.

				// Set how long the particle will live
				outOthers.x = lifeTime;

				// Set flag that this particle was emited, so it has to wait
				// with it's next emission until the emission counter is zeroed.
				outOthers.y = 1;

				// Set the position in the center of the emitter. If the stream is not
				// in the center it will be edited soon
				outPosition = emitterPosition;
		
				// Remember the modulo of vertex id, so we can know in which stream it is.
				int mod = gl_VertexID % 4;

				// Set the rand seed
				randSeed = uint((outOthers.x+1) * 1000.0) + uint(gl_VertexID);

				// Set the base color (the center stream) using the randomized saturation
				float deltaSaturation = randhash(colorSaturation);
				outColor = vec4(deltaSaturation, deltaSaturation, deltaSaturation, 1);
		
				// If this is not a center stream
				if (mod > 0)
				{
					// Set the proper position on the edges of the circle with current emitter
					// rotation position.
					outPosition.x += (emitterRadius * sin(mod * D120 + emitterRotation));
					outPosition.z -= (emitterRadius * cos(mod * D120 + emitterRotation));

					// Set the proper color on one channel (the saturation from base color should
					// remains intact).
					switch (mod)
					{
						case 1:
							outColor.r = 1; break;
						case 2:
							outColor.g = 1; break;
						case 3:
							outColor.b = 1; break;
					}
				}		

				// Set the xz-axis velocity using the emitter spread (or 0 if spread is 0).
				outVelocity.x = emitterSpread==0?0:randhash(emitterSpread)-emitterSpread*0.5;
				outVelocity.z = emitterSpread==0?0:randhash(emitterSpread)-emitterSpread*0.5;

				// Set the y-axis velocity based on the speed. It has to be little randomized for
				// better visual effect.
				outVelocity.y = randhash(0.5) + speed;
			}
		}
		else
		{
			// This particle can't be emitted, because the emission counter
			// isn't count this particle yet. But this particle is not alive, so it 
			// waits for emission. Set the flag that this particle wasn't emitted.
			outOthers.y = 0;
		}
	}
	else
	{
		// This particle is alive, so just update it.

		// Set the new position based on the velocity
		outPosition		+= outVelocity*deltaTime;

		// Apply the gravity to the y-axis velocity
		outVelocity.y	-= gravity*deltaTime;

		// Update life time left
		outOthers.x		-= deltaTime;

		// If there is just one second left to die fade it nicely out.
		if (outOthers.x < 1)
		{
			outColor.a -= deltaTime;
		}
	}
}