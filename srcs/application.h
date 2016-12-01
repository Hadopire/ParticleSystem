#pragma once

#ifdef _WIN32
	#include <GL/glew.h>
	#include <CL/cl.h>
	#include <CL/cl_gl.h>
	#include <glm/vec4.hpp>
	#include <GLFW/glfw3.h>
#elif defined __APPLE__
	#define __gl_h_
	#define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
	#include <opencl/cl_gl.h>
	#include <opencl/cl.h>
	#include <Opencl/opencl.h>
	#include <opencl/cl_gl_ext.h>
	#include <opengl/gl3.h>
	#include <Opengl/Opengl.h>
	#include <GLFW/glfw3.h>
#endif


#include <stdexcept>
#include <iostream>
#include <vector>
#include <cstring>
#include <ctime>

#include "utils.h"
#include "glutils.h"
#include "deleter.h"

class Application
{
public:
	~Application();
	Application() {};

	void run();

private:
	Deleter<GLFWwindow*> window{ glfwDestroyWindow };
	GLuint glProgram;

	cl_platform_id platformID{ nullptr };
	cl_device_id deviceID{ nullptr };
	Deleter<cl_context> context{ clReleaseContext };
	Deleter<cl_command_queue> commandQueue{ clReleaseCommandQueue };
	Deleter<cl_program> clProgram{ clReleaseProgram };
	Deleter<cl_kernel> clInitPositions{ clReleaseKernel };
	Deleter<cl_kernel> clInitPositionsInSphere{ clReleaseKernel };
	Deleter<cl_kernel> clUpdatePositions{ clReleaseKernel };

	GLuint pVbo;
	GLuint vVbo;
	GLuint cVbo;
	GLuint vao;
	Deleter<cl_mem> clPos{ clReleaseMemObject };
	Deleter<cl_mem> clVel{ clReleaseMemObject };
	Deleter<cl_mem> clCol{ clReleaseMemObject };
	Deleter<cl_mem> clAttractors{ clReleaseMemObject };
	std::vector<cl_float4> attractors{ 0 };

	unsigned int particleCount{ 1000000 };

	static void glfwErrorCallback(int error, const char* desc);
	static void glfwKeyboardCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
	static void CL_CALLBACK clErrorCallback(const char *errinfo, const void *private_info, size_t cb, void *user_data);

	void initOpenGL();
	void createContext();
	void createCommandQueue();
	void buildKernels();
	void initOpenCL();
	void initBuffers(const Deleter<cl_kernel> & initFunc);
	cl_mem pclCreateBuffer(cl_mem_flags flags, size_t size, void *data);
	void mainLoop();
};

