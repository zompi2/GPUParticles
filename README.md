GPU Particles
=====
This is a project that presents how to implement a gpu simulated particles with OpenGL.

## How to build
This project uses CMake to generate build solution.  
It requires OpenGL to be installed on the building machine.  
It requires [GLEW](http://glew.sourceforge.net) and [GLFW](https://www.glfw.org) libraries (give CMake paths to their libraries and includes).  
Data directory must be in the same directory the executable is.  

## Controls
Hold **right mouse** button to rotate camera  
**W/S/A/D** - move camera  
**Y/H/G/J** - move particles source  
**I/K** - move particles source up and down

## Configuration
You can change various settings in Data/config.ini to alter application behaviour like the amount of particles to spawn or forcing CPU calculations.

## More
You can read more about gpu particles in the blog entry: https://damnow.blogspot.com/2014/12/gpu-particles.html

You can download a working .exe file from here: https://drive.google.com/file/d/16rvR9DObKb9d1kJLrXQ76i3DRNq8g44W