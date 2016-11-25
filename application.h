#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <CL/cl.h>
#include <CL/cl_gl.h>
#include <glm/vec4.hpp>

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
	Deleter<cl_kernel> clUpdatePositions{ clReleaseKernel };
	
	GLuint pVbo;
	GLuint vVbo;
	GLuint vao;
	Deleter<cl_mem> clPos{ clReleaseMemObject };
	Deleter<cl_mem> clVel{ clReleaseMemObject };
	Deleter<cl_mem> clAttractors{ clReleaseMemObject };
	std::vector<cl_float2> attractors{ 0 };

	static void glfwErrorCallback(int error, const char* desc);
	static void glfwKeyboardCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
	static void CL_CALLBACK clErrorCallback(const char *errinfo, const void *private_info, size_t cb, void *user_data);
	static cl_int clGetGLContextInfoKHRCallback(
		const cl_platform_id platform,
		const cl_context_properties *properties,
		cl_gl_context_info param_name,
		size_t param_value_size,
		void *param_value,
		size_t *param_value_size_ret
	);

	
	void initOpenGL();
	void createContext();
	void createCommandQueue();
	void buildKernels();
	void initOpenCL();
	void initBuffers();
	cl_mem pclCreateBuffer(cl_mem_flags flags, size_t size, void *data);
	void mainLoop();
};

