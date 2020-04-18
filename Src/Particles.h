#pragma once

/**
* GPU Particles example.
*
* This is an tweaking bar class that handles the AntTweakBar, initialize it
* and set up the input.
*
* (c) 2014 Damian Nowakowski
*/

#include <GL/glew.h>

// Define the uniform buffer elements of particles compute shader
#define PARTICLES_UNIFORM_SIZE 10

// Predefine class for visibility
class Camera;

class Particles
{
public:
	/**
	* Simple constructor and destructor.
	*/
	Particles();
	~Particles();

	/**
	* Update particles' state. Here the compute shader will be ran.
	* @param deltaTime - the portion of time thas passed from previous update.
	*/
	void Update(float deltaTime);

	/**
	* Draw particles.
	* @param camera - the pointer to the currently used camera.
	*/
	void Draw(Camera * camera);

private:

	/**
	* Update particles' state using the CPU.
	* @param deltaTime - the portion of time thas passed from previous update.
	*/
	void UpdateCPU(float deltaTime);

	/**
	* Draw particles from data used in updating using CPU.
	* @param camera - the pointer to the currently used camera.
	*/
	void DrawCPU(Camera * camera);

	void UpdateCPUThread(int tid, float deltaTime, int from, int to);

	glm::vec3 emitterPosition;		///< Position of the particles emitter.
	glm::vec3 emitterMoveDir;		///< Current direction of emitter movement.

	float emitPeriod;				///< Period of emitting every portion of particles.
									///< Thanks to that particles will be emitting systematically,
									///< not at the same time.
	float timeToNextEmission;		///< Time to nex emission of portion of particles.
	float particlePointSize;		///< Size of the particle.
	float particleLifeTime;			///< Time of life of one particle.
	float particleColorSaturation;	///< Range of particle color saturation.
	float particleSpeed;			///< Speed of particle in y-axis.
	float emitterRotationSpeed;		///< Speed of emitter rotation.
	float emitterRotation;			///< Current rotation angle of the emitter.
	float emitterRadius;			///< Radius of the emitter (how far every stream is from the center).
	float emitterSpread;			///< Spread of every stream.
	float emitterMoveSpeed;			///< Speed of emitter movement.
	float gravity;					///< The gravity of the enviroment.

	int particlesEmitAtOnce;		///< How many particles will be emited with one portion.
	int particlesEmitted;			///< How many particles were already emited.
	int particlesCount;				///< How many particles are here at all (max amount of particles).

	int threadsCount;

	GLuint shader_render;			///< Id of the render shader.
	GLuint shader_compute;			///< Id of the compute shader.
	GLuint VAO;						///< Vertex array object for handling data to compute and render.
	GLuint VBO[2];					///< Vertex buffer object for handling data to compute and render.
									///< There are two, because computed data can't be saved into the same buffer.
	GLuint UBO;						///< Uniform buffer object for computed shader.

	GLint uniformsOffset[PARTICLES_UNIFORM_SIZE];	///< Array that stores offsets of values in uniform buffer.

	GLfloat* VBOCPP;
	bool UseCPU;
	
	/**
	* Handle the input controlling particle emitter position.
	* @returns true if there was an input.
	*/
	bool HandleInput();
};

