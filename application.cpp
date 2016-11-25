#include "Application.h"

#define PARTICLE_COUNT 20000000
#define LOCAL_WORK_ITEM 256

Application::~Application() {
}

void Application::run() {
	initOpenGL();
	initOpenCL();
	initBuffers();
	mainLoop();
}

void Application::glfwErrorCallback(int error, const char *desc) {
	std::cerr << "glfwErrorCallback: " << desc << std::endl;
}

void CL_CALLBACK Application::clErrorCallback(const char *errinfo, const void *private_info, size_t cb, void *user_data) {
	std::cerr << "clErrorCallback: " << errinfo << std::endl;
}

cl_int Application::clGetGLContextInfoKHRCallback(const cl_platform_id platform, const cl_context_properties *properties, cl_gl_context_info param_name, size_t param_value_size, void *param_value, size_t *param_value_size_ret) {
	auto func = (clGetGLContextInfoKHR_fn)clGetExtensionFunctionAddressForPlatform(platform, "clGetGLContextInfoKHR");
	if (func) {
		return func(properties, param_name, param_value_size, param_value, param_value_size_ret);
	}
	else {
		throw std::runtime_error("call to clGetGLContextInfoKHR failed");
	}
}

void Application::initOpenGL() {
	if (!glfwInit()) {
		throw std::runtime_error("failed to initialize glfw");
	}
	

	glfwSetErrorCallback(glfwErrorCallback);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	window = glfwCreateWindow(1024, 768, "Particle System", nullptr, nullptr);
	if (!window.data()) {
		throw std::runtime_error("failed to initialize glfw window.");
	}
	
	glfwMakeContextCurrent(window.data());
	glfwSwapInterval(0);

	glfwSetWindowUserPointer(this->window.data(), this);
	glfwSetKeyCallback(this->window.data(), glfwKeyboardCallback);
	
	glewExperimental = GL_TRUE;
	glewInit();

	GLuint pVertShader = GlUtils::loadShader(Utils::getExecDir() + "\\shader.vert", GL_VERTEX_SHADER);
	GLuint pFragShader = GlUtils::loadShader(Utils::getExecDir() + "\\shader.frag", GL_FRAGMENT_SHADER);
	
	this->glProgram = glCreateProgram();
	glAttachShader(this->glProgram, pVertShader);
	glAttachShader(this->glProgram, pFragShader);
	glLinkProgram(this->glProgram);

	GLint linked;
	glGetProgramiv(this->glProgram, GL_LINK_STATUS, &linked);
	if (!linked) {
		GLsizei logSize;
		glGetProgramiv(this->glProgram, GL_INFO_LOG_LENGTH, &logSize);
		std::vector<char> log(logSize);
		glGetProgramInfoLog(this->glProgram, logSize, nullptr, log.data());
		std::cerr << log.data();
		throw std::runtime_error("failed to link gl program.");
	}

	glDeleteShader(pVertShader);
	glDeleteShader(pFragShader);


	glClearColor(0, 0, 0, 0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA);
	glDisable(GL_DEPTH_TEST);
}

void Application::createContext() {
	cl_uint platformCount;
	clGetPlatformIDs(0, nullptr, &platformCount);
	std::vector<cl_platform_id> platforms(platformCount);
	clGetPlatformIDs(platformCount, platforms.data() , nullptr);

	for (auto platform : platforms) {
		size_t extensionCount;
		clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS, 0, nullptr, &extensionCount);
		std::vector<char> extensions(extensionCount);
		clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS, extensionCount, extensions.data(), nullptr);
		if (!strstr(extensions.data(), "cl_khr_gl_sharing")) {
			continue;
		}

		size_t deviceCount;
		clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, nullptr, &deviceCount);
		std::vector<cl_device_id> devices(deviceCount);
		clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, deviceCount, devices.data(), nullptr);

		for (auto device : devices) {
			cl_uint deviceMaxComputeUnits;

			const cl_context_properties properties[]{
				CL_CONTEXT_PLATFORM,
				reinterpret_cast<cl_context_properties>(platform),
				CL_GL_CONTEXT_KHR,
				reinterpret_cast<cl_context_properties>(wglGetCurrentContext()),
				CL_WGL_HDC_KHR,
				reinterpret_cast<cl_context_properties>(wglGetCurrentDC()),
				0
			};

			cl_int err;
			this->context = clCreateContext(properties, 1, &device, clErrorCallback, this, &err);
			if (err == CL_SUCCESS) {
				this->platformID = platform;
				this->deviceID = device;
				return;
			}
		}
	}
	throw std::runtime_error("no OpenCL compatible device found.");
}

void Application::createCommandQueue() {
	cl_int err;
	commandQueue = clCreateCommandQueue(this->context.data(), this->deviceID, 0, &err);
	if (err != CL_SUCCESS) {
		std::cerr << "clCreateCommandQueue error code " << err << std::endl;
		throw std::runtime_error("failed to create cl command queue.");
	}
}

void Application::buildKernels() {
	std::string src = Utils::readFile(Utils::getExecDir() + "\\particle.cl");
	const char *data = src.data();
	size_t len = src.length();

	cl_int err;
	this->clProgram = clCreateProgramWithSource(this->context.data(), 1, &data, &len, &err);
	if (err != CL_SUCCESS) {
		std::cerr << "clCreateProgramWithSource error code: " << err << std::endl;
		throw std::runtime_error("failed to load cl program");
	}

	err = clBuildProgram(this->clProgram.data(), 1, &this->deviceID, nullptr, nullptr, nullptr);
	if (err != CL_SUCCESS) {
		size_t logSize;
		clGetProgramBuildInfo(this->clProgram.data(), this->deviceID, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);
		std::vector<char> log(logSize);
		clGetProgramBuildInfo(this->clProgram.data(), this->deviceID, CL_PROGRAM_BUILD_LOG, logSize, log.data(), nullptr);
		std::cerr << log.data();
		throw std::runtime_error("failed to build cl program");
	}

	this->clInitPositions = clCreateKernel(this->clProgram.data(), "initPositions", &err);
	if (err != CL_SUCCESS) {
		std::cerr << "clCreateKernel error code " << err << std::endl;
		throw std::runtime_error("failed to create kernel.");
	}

	this->clUpdatePositions = clCreateKernel(this->clProgram.data(), "updatePositions", &err);
	if (err != CL_SUCCESS) {
		std::cerr << "clCreateKernel error code " << err << std::endl;
		throw std::runtime_error("failed to create kernel.");
	}
}

void Application::initOpenCL() {
	createContext();
	createCommandQueue();
	buildKernels();
}

void Application::initBuffers() {
	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		std::cout << "error: " << glewGetErrorString(error) << std::endl;
	}
	
	glDeleteVertexArrays(1, &this->vao);
	glDeleteBuffers(1, &this->pVbo);
	glDeleteBuffers(1, &this->vVbo);

	error = glGetError();
	if (error != GL_NO_ERROR) {
		std::cout << "glDeleteBuffer error: " << glewGetErrorString(error) << std::endl;
	}
	
	glGenBuffers(1, &this->vVbo);
	glBindBuffer(GL_ARRAY_BUFFER, this->vVbo);
	glBufferData(GL_ARRAY_BUFFER, PARTICLE_COUNT * sizeof(cl_float2), nullptr, GL_DYNAMIC_DRAW);

	error = glGetError();
	if (error != GL_NO_ERROR) {
		std::cout << "glBufferData vVbo error: " << glewGetErrorString(error) << std::endl;
	}

	glGenBuffers(1, &this->pVbo);
	glBindBuffer(GL_ARRAY_BUFFER, this->pVbo);
	glBufferData(GL_ARRAY_BUFFER, PARTICLE_COUNT * sizeof(cl_float2), nullptr, GL_DYNAMIC_DRAW);

	error = glGetError();
	if (error != GL_NO_ERROR) {
		std::cout << "glBufferData pVbo error: " << glewGetErrorString(error) << std::endl;
	}
	
	glGenVertexArrays(1, &this->vao);
	glBindVertexArray(this->vao);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
	glEnableVertexAttribArray(0);

	glFinish();

	cl_int err;
	this->clPos = clCreateFromGLBuffer(this->context.data(), CL_MEM_READ_WRITE, this->pVbo, &err);
	if (err != CL_SUCCESS) {
		std::cerr << "clCreateFromGLBuffer error code " << err << std::endl;
		throw std::runtime_error("failed to create cl buffer");
	}

	this->clVel = clCreateFromGLBuffer(this->context.data(), CL_MEM_READ_WRITE, this->vVbo, &err);
	if (err != CL_SUCCESS) {
		std::cerr << "clCreateFromGLBuffer error code " << err << std::endl;
		throw std::runtime_error("failed to create cl buffer");
	}

	cl_uint seed = static_cast<cl_uint>(time(NULL));
	clSetKernelArg(this->clInitPositions.data(), 0, sizeof(cl_mem), &this->clPos);
	clSetKernelArg(this->clInitPositions.data(), 1, sizeof(cl_mem), &this->clVel);
	clSetKernelArg(this->clInitPositions.data(), 2, sizeof(cl_uint), &seed);

	std::vector<cl_mem> objects{ this->clPos, this->clVel };
	err = clEnqueueAcquireGLObjects(this->commandQueue.data(), 2, objects.data(), 0, nullptr, nullptr);
	if (err != CL_SUCCESS) {
		std::cerr << "clEnqueueNDRangeKernel error code " << err << std::endl;
		throw std::runtime_error("enqueue command failed.");
	}

	clFinish(this->commandQueue.data());

	size_t workSize = PARTICLE_COUNT;
	size_t itemPerGroup = LOCAL_WORK_ITEM;
	err = clEnqueueNDRangeKernel(this->commandQueue.data(), this->clInitPositions.data(), 1, nullptr, &workSize, nullptr, 0, nullptr, nullptr);
	if (err != CL_SUCCESS) {
		std::cerr << "clEnqueueNDRangeKernel error code " << err << std::endl;
		throw std::runtime_error("enqueue command failed.");
	}
	clFinish(this->commandQueue.data());

	err = clEnqueueReleaseGLObjects(this->commandQueue.data(), objects.size(), objects.data(), 0, nullptr, nullptr);
	if (err != CL_SUCCESS) {
		std::cerr << "clEnqueueNDRangeKernel error code " << err << std::endl;
		throw std::runtime_error("enqueue command failed.");
	}

	clFinish(this->commandQueue.data());
}

cl_mem Application::pclCreateBuffer(cl_mem_flags flags, size_t size, void *data) {
	if (size == 0) {
		return nullptr;
	}

	cl_int err;
	cl_mem return_value = clCreateBuffer(this->context.data(), flags, size, data, &err);
	if (err != CL_SUCCESS) {
		std::cerr << "clCreateFromGLBuffer error code " << err << std::endl;
		throw std::runtime_error("failed to create cl buffer");
	}
	return return_value;
}

void Application::glfwKeyboardCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
	int winWidth;
	int winHeight;

	glfwGetWindowSize(window, &winWidth, &winHeight);
	Application *app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));

	if (key == GLFW_KEY_G && action == GLFW_PRESS) {
		double x;
		double y;
		glfwGetCursorPos(app->window.data(), &x, &y);

		cl_float2 attractor;
		attractor.x = x / static_cast<float>(winWidth) * 2.f - 1.f;
		attractor.y = -(y / static_cast<float>(winHeight) * 2.f - 1.f);
		app->attractors.push_back(attractor);

		app->clAttractors = app->pclCreateBuffer(CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, app->attractors.size() * sizeof(cl_float2), app->attractors.data());
	}
	else if (key == GLFW_KEY_D && action == GLFW_PRESS) {
		if (app->attractors.size() > 0) {
			app->attractors.erase(app->attractors.begin());
			app->clAttractors = app->pclCreateBuffer(CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, app->attractors.size() * sizeof(cl_float2), app->attractors.data());
		}
	}
}

void Application::mainLoop() {
	float previousTime = static_cast<float>(glfwGetTime());
	float currentTime;
	cl_float delta;
	float counter = 0;

	int winWidth;
	int winHeight;

	glUseProgram(this->glProgram);
	glBindVertexArray(this->vao);

	bool isCursorAttractor = false;
	bool isCursorRepulsor = false;

	while (!glfwWindowShouldClose(window.data())) {
		glfwPollEvents();
		glfwGetWindowSize(this->window.data(), &winWidth, &winHeight);

		currentTime = static_cast<float>(glfwGetTime());
		delta = currentTime - previousTime;
		counter += delta;
		if (counter > 1.f) {
			std::cout << "fps: " << 1.f / delta << ", delta: " << delta << std::endl;
			counter = 0.f;
		}

		int state = glfwGetKey(this->window.data(), GLFW_KEY_SPACE);
		if (state == GLFW_PRESS) {
			initBuffers();
		}

		state = glfwGetMouseButton(this->window.data(), GLFW_MOUSE_BUTTON_LEFT);
		if (state == GLFW_PRESS && !isCursorRepulsor) {
			double x;
			double y;
			glfwGetCursorPos(this->window.data(), &x, &y);

			cl_float2 attractor;
			attractor.x = x / static_cast<float>(winWidth) * 2.f - 1.f;
			attractor.y = -(y / static_cast<float>(winHeight) * 2.f - 1.f);
			this->attractors.push_back(attractor);

			isCursorAttractor = true;
			this->clAttractors = pclCreateBuffer(CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, this->attractors.size() * sizeof(cl_float2), this->attractors.data());
		}
		else {
			isCursorAttractor = false;
		}

		cl_uint attractorCount = this->attractors.size();
		clSetKernelArg(this->clUpdatePositions.data(), 0, sizeof(cl_mem), &this->clPos);
		clSetKernelArg(this->clUpdatePositions.data(), 1, sizeof(cl_mem), &this->clVel);
		clSetKernelArg(this->clUpdatePositions.data(), 2, sizeof(cl_float), &delta);
		clSetKernelArg(this->clUpdatePositions.data(), 3, sizeof(cl_mem), &this->clAttractors);
		clSetKernelArg(this->clUpdatePositions.data(), 4, sizeof(cl_uint), &attractorCount);

		std::vector<cl_mem> objects{ this->clPos, this->clVel };
		cl_int err = clEnqueueAcquireGLObjects(this->commandQueue.data(), objects.size(), objects.data(), 0, nullptr, nullptr);
		if (err != CL_SUCCESS) {
			std::cerr << "clEnqueueNDRangeKernel error code " << err << std::endl;
			throw std::runtime_error("enqueue command failed.");
		}

		clFinish(this->commandQueue.data());

		size_t workSize = PARTICLE_COUNT;
		size_t itemPerGroup = LOCAL_WORK_ITEM;
		err = clEnqueueNDRangeKernel(this->commandQueue.data(), this->clUpdatePositions.data(), 1, nullptr, &workSize, nullptr, 0, nullptr, nullptr);
		if (err != CL_SUCCESS) {
			std::cerr << "clEnqueueNDRangeKernel error code " << err << std::endl;
			throw std::runtime_error("enqueue command failed.");
		}
		clFinish(this->commandQueue.data());

		if (isCursorAttractor || isCursorRepulsor) {
			this->attractors.erase(this->attractors.end() - 1);
			this->clAttractors = pclCreateBuffer(CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, this->attractors.size() * sizeof(cl_float2), this->attractors.data());
		}

		err = clEnqueueReleaseGLObjects(this->commandQueue.data(), objects.size(), objects.data(), 0, nullptr, nullptr);
		if (err != CL_SUCCESS) {
			std::cerr << "clEnqueueNDRangeKernel error code " << err << std::endl;
			throw std::runtime_error("enqueue command failed.");
		}

		clFinish(this->commandQueue.data());

		glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_POINTS, 0, PARTICLE_COUNT);
		glfwSwapBuffers(window.data());
		previousTime = currentTime;
	}
}
