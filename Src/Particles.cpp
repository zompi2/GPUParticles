/**
* GPU Particles example.
*
* This is an tweaking bar class that handles the AntTweakBar, initialize it
* and set up the input.
*
* (c) 2014 Damian Nowakowski
*/

#include "Engine.h"
#include "Window.h"
#include "Camera.h"
#include "Particles.h"
#include "Shaders.h"

#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>

#include <random>

/**
* One particle contains:
*
* x,	y,		z			=> Position xyz								(vec3)
* r,	g,		b,		a	=> Color rgba								(vec4)
* vx,	vy,		vz			=> Velocity xyz								(vec3)
* lt,	we					=> Life time left and "was emitted" flag	(vec2)
*
* So one particle is a size of 12 GLfloats
*/
const int glFloatSize		= sizeof(GLfloat);					///< The size of GLfloat (instead of using sizeof in 
																///< future just remember it)
const int particleSize		= 12;								///< The size in floats of one particle
const int particleDataSize	= particleSize * glFloatSize;		///< The size of data of the one particle
																///< (it will be used many times, so better remember it here)
/**
* Simple constructor with initialization.
*/
Particles::Particles()
{
	// Save local ini reader so we won't have to get it every time
	INIReader * localINIReader = ENGINE->config;

	/// Get all needed data from configuration ini file
	particlesCount			= (int)localINIReader->GetInteger("Particles", "Count", 100);
	particlesEmitAtOnce		= (int)localINIReader->GetInteger("Particles", "EmitAtOnce", 100);
	particlePointSize		= (float)localINIReader->GetReal("Particles", "PointSize", 1.f);
	particleLifeTime		= (float)localINIReader->GetReal("Particles", "LifeTime", 1.f);
	particleSpeed			= (float)localINIReader->GetReal("Particles", "Speed", 1.f);
	particleColorSaturation	= (float)localINIReader->GetReal("Particles", "Saturation", 0.1f);
	emitPeriod				= (float)localINIReader->GetReal("Particles", "Period", 0.1f);
	emitterMoveSpeed		= (float)localINIReader->GetReal("Particles", "emitter_Speed", 1.f);
	emitterRotationSpeed	= (float)localINIReader->GetReal("Particles", "Rot_Speed", 1.f);
	gravity					= (float)localINIReader->GetReal("Particles", "Gravity", 0.f);

	emitterPosition			= glm::vec3(	(float)localINIReader->GetReal("Particles", "emitter_X", 0.f),
											(float)localINIReader->GetReal("Particles", "emitter_Y", 0.f),
											(float)localINIReader->GetReal("Particles", "emitter_Z", 0.f)
										);

	emitterRadius			= (float)localINIReader->GetReal("Particles", "Radius", 1.f);
	emitterSpread			= (float)localINIReader->GetReal("Particles", "Spread", 1.f);

	UseCPU					= (bool)localINIReader->GetBoolean("System", "UseCPU", false);

	/// Set initial values for some data
	particlesEmitted		= 0;
	timeToNextEmission		= 0;
	emitterRotation			= 0;

	/// Calculate the maximum life time the particle can have.
	/// If the life time is longer there might be some bugs, because the
	/// particle will live longer than the whole emit process.
	float maxLifeTime = particlesCount * emitPeriod / particlesEmitAtOnce;
	if (maxLifeTime < particleLifeTime)
	{
		particleLifeTime = maxLifeTime;
	}

	/// Create a shader for rendering particles
	Shaders::AttachShader(shader_render, GL_VERTEX_SHADER, "data/shaders/point_vs.glsl");
	Shaders::AttachShader(shader_render, GL_FRAGMENT_SHADER, "data/shaders/point_fs.glsl");
	Shaders::LinkProgram(shader_render);

	/// Create a shader for updating particles. Here we are defining which outputs will be transported
	/// back to the buffer. The order of inputs, outputs and names of variables in array below must be the same!
	Shaders::AttachShader(shader_compute, GL_VERTEX_SHADER, "data/shaders/point_update_vs.glsl");
	const char* shaderOutputs[4] = {
		"outPosition",
		"outColor",
		"outVelocity",
		"outOthers"
	};
	glTransformFeedbackVaryings(shader_compute, 4, shaderOutputs, GL_INTERLEAVED_ATTRIBS);
	Shaders::LinkProgram(shader_compute);

	/// Generate all necessary buffors for data
	glGenVertexArrays(1, &VAO);
	glGenBuffers(2, VBO);
	glGenBuffers(1, &UBO);
	
	/// Remember the size needed to store all particles data and create an empty
	/// array. The array will be used to fill buffers.
	int allParticlesDataSize = particlesCount * particleDataSize;
	char * nullData = new char[allParticlesDataSize]();
	std::fill(nullData, nullData + allParticlesDataSize, 0);

	if (UseCPU == true)
	{
		VBOCPP = new GLfloat[particlesCount * particleSize]();
		std::fill(VBOCPP, VBOCPP + particlesCount * particleSize, 0.f);
	}
	
	// Bind the vertex array object which will be used both for computing and rendering
	glBindVertexArray(VAO);
		
		// Enable attribute pointers. Because locations were used in shaders they could be
		// arranged from 1 to 4.
		for (int i = 1; i <= 4; i++)
		{
			glEnableVertexAttribArray(i);
		}
		
		/// Fill two buffers with zeroes so there won't be any junk data
		glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
			glBufferData(GL_ARRAY_BUFFER, allParticlesDataSize, nullData, GL_STREAM_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
			glBufferData(GL_ARRAY_BUFFER, allParticlesDataSize, nullData, GL_STREAM_DRAW);

	
	// Unbind the vertex array object for now, it won't be needed for a while
	glBindVertexArray(0);

	// Clean up now unnecessary data
	delete [] nullData;


	/// Declare the space in uniform buffer for shader where particles parameter are stored.
	/// Remember all parameters offsets so the uniform buffer can be easely fill and update after that.
	glBindBuffer(GL_UNIFORM_BUFFER, UBO);

	// Get the needed size of uniform buffer (the size of data structure in shader)
	GLint particlesParamsSize;
	GLuint particlesParamsIndex = glGetUniformBlockIndex(shader_compute, "ParticlesParams");
	glGetActiveUniformBlockiv(shader_compute, particlesParamsIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &particlesParamsSize);
	glBufferData(GL_UNIFORM_BUFFER, particlesParamsSize, NULL, GL_DYNAMIC_DRAW);

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, UBO);
	glUniformBlockBinding(shader_compute, particlesParamsIndex, 0);

	GLuint uniformsIndex[PARTICLES_UNIFORM_SIZE];
	const GLchar *uniformsName[PARTICLES_UNIFORM_SIZE] =
	{
		"deltaTime",
		"emitterPosition",
		"particlesEmitted",
		"emitterRotation",
		"lifeTime",
		"emitterRadius",
		"emitterSpread",
		"colorSaturation",
		"speed",
		"gravity"
	};

	// Get uniform buffers parameters offsets for future easily filling and update
	glGetUniformIndices(shader_compute, PARTICLES_UNIFORM_SIZE, uniformsName, uniformsIndex);
	glGetActiveUniformsiv(shader_compute, PARTICLES_UNIFORM_SIZE, uniformsIndex, GL_UNIFORM_OFFSET, uniformsOffset);

	// Fill the uniform buffer with first values.
	glBufferSubData(GL_UNIFORM_BUFFER, uniformsOffset[0], 4, 0);
	glBufferSubData(GL_UNIFORM_BUFFER, uniformsOffset[1], 12, glm::value_ptr(emitterPosition));
	glBufferSubData(GL_UNIFORM_BUFFER, uniformsOffset[2], 4, &particlesEmitted);
	glBufferSubData(GL_UNIFORM_BUFFER, uniformsOffset[3], 4, &emitterRotation);
	glBufferSubData(GL_UNIFORM_BUFFER, uniformsOffset[4], 4, &particleLifeTime);
	glBufferSubData(GL_UNIFORM_BUFFER, uniformsOffset[5], 4, &emitterRadius);
	glBufferSubData(GL_UNIFORM_BUFFER, uniformsOffset[6], 4, &emitterSpread);
	glBufferSubData(GL_UNIFORM_BUFFER, uniformsOffset[7], 4, &particleColorSaturation);
	glBufferSubData(GL_UNIFORM_BUFFER, uniformsOffset[8], 4, &particleSpeed);
	glBufferSubData(GL_UNIFORM_BUFFER, uniformsOffset[9], 4, &gravity);

	// Unbind the buffer object so program won't use them unnecessarily
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, 0);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

/**
* Update particles' state. Here the compute shader will be ran.
* @param deltaTime - the portion of time thas passed from previous update.
*/
void Particles::Update(float deltaTime)
{
	if (UseCPU == true)
	{
		UpdateCPU(deltaTime);
		return;
	}
	// Bind the uniform buffer, because we will update it and use it soon.
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, UBO);

	// Set deltaTime to the compute shader
	glBufferSubData(GL_UNIFORM_BUFFER, uniformsOffset[0], 4, &deltaTime);

	/// Check if particle emitter position has to be update. If yes, then update it and also
	/// update it in the uniform buffer.
	if (HandleInput() == true)
	{
		emitterPosition += (emitterMoveDir * emitterMoveSpeed * deltaTime);
		glBufferSubData(GL_UNIFORM_BUFFER, uniformsOffset[1], 12, glm::value_ptr(emitterPosition));
	}

	/// Update the emitter current rotation position
	emitterRotation += emitterRotationSpeed * deltaTime;
	glBufferSubData(GL_UNIFORM_BUFFER, uniformsOffset[3], 4, &emitterRotation);
	
	// If all particles were emited zero the counter so particles will be emitted again.
	if (particlesEmitted == particlesCount)
	{
		particlesEmitted = 0;
	}
	
	/// Decrease time to nex emission and if this is a time for emission
	/// increase the emitted particles counter. Update this counter in shader too.
	timeToNextEmission -= deltaTime;
	if (timeToNextEmission <= 0)
	{
		particlesEmitted += particlesEmitAtOnce;
		glBufferSubData(GL_UNIFORM_BUFFER, uniformsOffset[2], 4, &particlesEmitted);

		if (particlesEmitted > particlesCount)
		{
			particlesEmitted = particlesCount;
		}
		timeToNextEmission = emitPeriod;
	}

	/// Now it is time for computing, using the compute shader.
	glUseProgram(shader_compute);
		// Use our vertex array object
		glBindVertexArray(VAO);

			/// Attribute pointers has to be set in every update to the first vertex buffer, because after every update
			/// buffers are swapped. So in fact we are setting attribute pointers to different buffer every update.
			/// Swapping has to be done, because we can't save data in the same buffer.
			char* pOffset = 0;
			glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, particleDataSize, pOffset);
			glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, particleDataSize, pOffset + glFloatSize * 3);
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, particleDataSize, pOffset + glFloatSize * 7);
			glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, particleDataSize, pOffset + glFloatSize * 10);

			// Enable rasterizer discard, because compute shader won't raster data
			glEnable(GL_RASTERIZER_DISCARD);

			// Bind the transform feedback buffer using the second vertex buffer object.
			// All transformed data will be stored to it.
			glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, VBO[1]);
	
			/// Draw Arrays using Transform Feedback
			glBeginTransformFeedback(GL_POINTS);
			glDrawArrays(GL_POINTS, 0, particlesCount);
			glEndTransformFeedback();

			// Unbind the transform feedback for safety
			glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);

			// Disable the rasterizer discard, because we need rasterization in drawing.
			glDisable(GL_RASTERIZER_DISCARD);

		/// Unbind vertex array object and disable computing program
		glBindVertexArray(0);
	glUseProgram(0);

	// Swap buffers, so the newly computed data will be used to the rendering and
	// they will be updated in next tick.
	std::swap(VBO[0], VBO[1]);

	// Unbind uniform buffer, because we don't need it for now
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, 0);
}

/**
* Update particles' state using the CPU.
* @param deltaTime - the portion of time thas passed from previous update.
*/
void Particles::UpdateCPU(float deltaTime)
{
	/// First updating the states of the emitter

	if (HandleInput() == true)
	{
		emitterPosition += (emitterMoveDir * emitterMoveSpeed * deltaTime);
	}
	emitterRotation += emitterRotationSpeed * deltaTime;

	if (particlesEmitted == particlesCount)
	{
		particlesEmitted = 0;
	}

	timeToNextEmission -= deltaTime;
	if (timeToNextEmission <= 0)
	{
		particlesEmitted += particlesEmitAtOnce;

		if (particlesEmitted > particlesCount)
		{
			particlesEmitted = particlesCount;
		}
		timeToNextEmission = emitPeriod;
	}

	/// Below it is simply a copy of compute shader calculations but written in C++

	/// Contants helping with "shader" writing
	const float D120 = 2.09439510f;
	const int POSITION = 0;
	const int COLOR = 3;
	const int VELOCITY = 7;
	const int OTHERS = 10;

	/// Random float number generators
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> particleSaturationRand(0.f, particleColorSaturation);
	std::uniform_real_distribution<float> emitterSpreadRand(0.f, emitterSpread);
	std::uniform_real_distribution<float> halfRand(0.f, 0.5f);

	int id = 0;
	for (int i = 0; i < particlesCount * particleSize; i += particleSize, id++)
	{
		if (VBOCPP[i + OTHERS] <= 0)
		{
			VBOCPP[i + COLOR + 0] = 0;
			VBOCPP[i + COLOR + 1] = 0;
			VBOCPP[i + COLOR + 2] = 0;
			VBOCPP[i + COLOR + 3] = 0;

			if (particlesEmitted > id)
			{
				if (VBOCPP[i + OTHERS + 1] == 0)
				{
					VBOCPP[i + OTHERS] = particleLifeTime;

					VBOCPP[i + OTHERS + 1] = 1;

					VBOCPP[i + POSITION + 0] = emitterPosition.x;
					VBOCPP[i + POSITION + 1] = emitterPosition.y;
					VBOCPP[i + POSITION + 2] = emitterPosition.z;

					int mod = id % 4;
					
					float deltaSaturation = particleSaturationRand(gen);
					VBOCPP[i + COLOR + 0] = deltaSaturation;
					VBOCPP[i + COLOR + 1] = deltaSaturation;
					VBOCPP[i + COLOR + 2] = deltaSaturation;
					VBOCPP[i + COLOR + 3] = 1;

					if (mod > 0)
					{
						VBOCPP[i + POSITION + 0] += (emitterRadius * sin(mod * D120 + emitterRotation));
						VBOCPP[i + POSITION + 2] -= (emitterRadius * cos(mod * D120 + emitterRotation));

						switch (mod)
						{
						case 1:
							VBOCPP[i + COLOR + 0] = 1; break;
						case 2:
							VBOCPP[i + COLOR + 1] = 1; break;
						case 3:
							VBOCPP[i + COLOR + 2] = 1; break;
						}
					}

					VBOCPP[i + VELOCITY + 0] = emitterSpread == 0 ? 0 : emitterSpreadRand(gen) - emitterSpread * 0.5f;
					VBOCPP[i + VELOCITY + 2] = emitterSpread == 0 ? 0 : emitterSpreadRand(gen) - emitterSpread * 0.5f;

					VBOCPP[i + VELOCITY + 1] = halfRand(gen) + particleSpeed;
				}
			}
			else
			{
				VBOCPP[i + OTHERS + 1] = 0;
			}
		}
		else
		{
			VBOCPP[i + POSITION + 0] += VBOCPP[i + VELOCITY + 0] * deltaTime;
			VBOCPP[i + POSITION + 1] += VBOCPP[i + VELOCITY + 1] * deltaTime;
			VBOCPP[i + POSITION + 2] += VBOCPP[i + VELOCITY + 2] * deltaTime;

			VBOCPP[i + VELOCITY + 0] -= gravity*deltaTime;
			VBOCPP[i + VELOCITY + 1] -= gravity*deltaTime;
			VBOCPP[i + VELOCITY + 2] -= gravity*deltaTime;

			VBOCPP[i + OTHERS] -= deltaTime;

			if (VBOCPP[i + OTHERS] < 1)
			{
				VBOCPP[i + COLOR + 3] -= deltaTime;
			}
		}
	}
}

/**
* Draw particles.
* @param camera - the pointer to the currently used camera.
*/
void Particles::Draw(Camera * camera)
{
	/// Use render shader and our vertex array object to render all particles
	glUseProgram(shader_render);
		glBindVertexArray(VAO);

			if (UseCPU == true)
			{
				DrawCPU(camera);
			}
			else
			{
				/// Because after swap in update attribute pointers are pointing to the old data. They have to be updated.
				/// We can bind only these data we need, so the position and the color.
				char* pOffset = 0;
				glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, particleDataSize, pOffset);
				glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, particleDataSize, pOffset + glFloatSize * 3);
		
				/// Set uniforms for rendering
				glUniformMatrix4fv(glGetUniformLocation(shader_render, "viewProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(camera->GetViewProjectionMatrix()));
				glUniform1f(glGetUniformLocation(shader_render, "pointSize"), particlePointSize);
		
				/// Draw particles as points.
				glDrawArrays(GL_POINTS, 0, particlesCount);	
			}

		/// Unbind vertex array object and render program, it is no need for them now.
		glBindVertexArray(0);
	glUseProgram(0);
}

/**
* Draw particles from data used in updating using CPU.
* @param camera - the pointer to the currently used camera.
*/
void Particles::DrawCPU(Camera * camera)
{
	/// This is drawing particles by using data from VBOCPP

	glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
	glBufferData(GL_ARRAY_BUFFER, particlesCount * particleDataSize, VBOCPP, GL_STREAM_DRAW);

	char* pOffset = 0;
	glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, particleDataSize, pOffset);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, particleDataSize, pOffset + glFloatSize * 3);

	glUniformMatrix4fv(glGetUniformLocation(shader_render, "viewProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(camera->GetViewProjectionMatrix()));
	glUniform1f(glGetUniformLocation(shader_render, "pointSize"), particlePointSize);

	glDrawArrays(GL_POINTS, 0, particlesCount);
}

/**
* Handle the input controlling particle emitter position.
* @returns true if there was an input.
*/
bool Particles::HandleInput()
{
	// Remember the local glfw Window so we won't have to get it all the time
	GLFWwindow * localWindow = ENGINE->window->glfwWindow;

	// Zero the moving state. This state will be used to determine if there was an input.
	bool isMoving = false;

	// Zero movement direction for now.
	emitterMoveDir = glm::vec3(0);

	/// Below there are key bindings. Every key is setting movement direction and set
	/// the movement state to true, so the light position will be updated.

	///Bindings:
	// Y - forward
	// H - backward
	// G - left
	// J - right
	// I - up
	// K - down

	if (glfwGetKey(localWindow, GLFW_KEY_Y) == GLFW_PRESS)
	{
		emitterMoveDir.z += -1;
		isMoving = true;
	}

	if (glfwGetKey(localWindow, GLFW_KEY_H) == GLFW_PRESS)
	{
		emitterMoveDir.z += 1;
		isMoving = true;
	}

	if (glfwGetKey(localWindow, GLFW_KEY_G) == GLFW_PRESS)
	{
		emitterMoveDir.x += -1;
		isMoving = true;
	}

	if (glfwGetKey(localWindow, GLFW_KEY_J) == GLFW_PRESS)
	{
		emitterMoveDir.x += 1;
		isMoving = true;
	}

	if (glfwGetKey(localWindow, GLFW_KEY_I) == GLFW_PRESS)
	{
		emitterMoveDir.y += 1;
		isMoving = true;
	}

	if (glfwGetKey(localWindow, GLFW_KEY_K) == GLFW_PRESS)
	{
		emitterMoveDir.y += -1;
		isMoving = true;
	}

	// Return if camera is moving and has to be updated
	return isMoving;
}

/**
* Simple destructor clearing all data.
*/
Particles::~Particles()
{
	Shaders::DeleteShaders(shader_render);
	Shaders::DeleteShaders(shader_compute);
	glDeleteProgram(shader_render);
	glDeleteProgram(shader_compute);
	glDeleteBuffers(2, VBO);
	glDeleteBuffers(1, &UBO);
	glDeleteVertexArrays(1, &VAO);

	if (UseCPU == true)
	{
		delete[] VBOCPP;
	}
}