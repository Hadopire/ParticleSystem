#include "Application.h"

Application::~Application() {
}

void Application::run() {
	initOpenGL();
	initOpenCL();
	initBuffers(clInitPositions);
	mainLoop();
}

void Application::glfwErrorCallback(int error, const char *desc) {
	std::cerr << "glfwErrorCallback: " << desc << std::endl;
}

void CL_CALLBACK Application::clErrorCallback(const char *errinfo, const void *private_info, size_t cb, void *user_data) {
	std::cerr << "clErrorCallback: " << errinfo << std::endl;
}

void Application::initOpenGL() {
	if (!glfwInit()) {
		throw std::runtime_error("failed to initialize glfw");
	}

	glfwSetErrorCallback(glfwErrorCallback);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(1024, 768, "Particle System", nullptr, nullptr);
	if (!window.data()) {
		throw std::runtime_error("failed to initialize glfw window.");
	}

	glfwMakeContextCurrent(window.data());
	glfwSwapInterval(1);

	glfwSetWindowUserPointer(this->window.data(), this);
	glfwSetKeyCallback(this->window.data(), glfwKeyboardCallback);
#ifndef __APPLE__
	glewExperimental = GL_TRUE;
	glewInit();
#endif
	GLuint pVertShader = GlUtils::loadShader(Utils::getExecDir() + "/shader.vert", GL_VERTEX_SHADER);
	GLuint pFragShader = GlUtils::loadShader(Utils::getExecDir() + "/shader.frag", GL_FRAGMENT_SHADER);

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
		cl_uint deviceCount;
		clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, nullptr, &deviceCount);
		std::vector<cl_device_id> devices(deviceCount);
		clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, deviceCount, devices.data(), nullptr);

		for (auto device : devices) {
#ifdef __APPLE__
		CGLContextObj kCGLContext = CGLGetCurrentContext();
		CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);
#endif

			const cl_context_properties properties[]{
#ifdef _WIN32
				CL_CONTEXT_PLATFORM,
				reinterpret_cast<cl_context_properties>(platform),
				CL_GL_CONTEXT_KHR,
				reinterpret_cast<cl_context_properties>(wglGetCurrentContext()),
				CL_WGL_HDC_KHR,
				reinterpret_cast<cl_context_properties>(wglGetCurrentDC()),
				0
#elif defined __APPLE__
				CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
				reinterpret_cast<cl_context_properties>(kCGLShareGroup),
				CL_CONTEXT_PLATFORM,
				reinterpret_cast<cl_context_properties>(platform),
				0
#endif
			};

			cl_int err;
			this->context = clCreateContext(properties, 1, &device, clErrorCallback, this, &err);
			if (err == CL_SUCCESS) {
				size_t paramSize;
				clGetDeviceInfo(device, CL_DEVICE_NAME, 0, nullptr, &paramSize);
				std::vector<char> name(paramSize);
				clGetDeviceInfo(device, CL_DEVICE_NAME, paramSize, name.data(), nullptr);

				std::cout << name.data() << std::endl;

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
	commandQueue = clCreateCommandQueue(this->context.data(), this->deviceID, CL_QUEUE_PROFILING_ENABLE, &err);
	if (err != CL_SUCCESS) {
		std::cerr << "clCreateCommandQueue error code " << err << std::endl;
		throw std::runtime_error("failed to create cl command queue.");
	}
}

void Application::buildKernels() {
	std::string src = Utils::readFile(Utils::getExecDir() + "/particle.cl");
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

	this->clInitPositionsInSphere = clCreateKernel(this->clProgram.data(), "initPositionsInSphere", &err);
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

void Application::initBuffers(const Deleter<cl_kernel> & initFunc) {
	glDeleteVertexArrays(1, &this->vao);
	glDeleteBuffers(1, &this->pVbo);
	glDeleteBuffers(1, &this->vVbo);
	glDeleteBuffers(1, &this->cVbo);

	glGenBuffers(1, &this->vVbo);
	glBindBuffer(GL_ARRAY_BUFFER, this->vVbo);
	glBufferData(GL_ARRAY_BUFFER, particleCount * sizeof(cl_float4), nullptr, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &this->pVbo);
	glBindBuffer(GL_ARRAY_BUFFER, this->pVbo);
	glBufferData(GL_ARRAY_BUFFER, particleCount * sizeof(cl_float4), nullptr, GL_DYNAMIC_DRAW);

	glGenVertexArrays(1, &this->vao);
	glBindVertexArray(this->vao);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
	glEnableVertexAttribArray(0);

	glGenBuffers(1, &this->cVbo);
	glBindBuffer(GL_ARRAY_BUFFER, this->cVbo);
	glBufferData(GL_ARRAY_BUFFER, particleCount * sizeof(cl_float4), nullptr, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
	glEnableVertexAttribArray(1);

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

	this->clCol = clCreateFromGLBuffer(this->context.data(), CL_MEM_READ_WRITE, this->cVbo, &err);
	if (err != CL_SUCCESS) {
		std::cerr << "clCreateFromGLBuffer error code " << err << std::endl;
		throw std::runtime_error("failed to create cl buffer");
	}

	cl_uint seed = static_cast<cl_uint>(time(NULL));
	clSetKernelArg(initFunc.data(), 0, sizeof(cl_mem), &this->clPos);
	clSetKernelArg(initFunc.data(), 1, sizeof(cl_uint), &seed);
	clSetKernelArg(initFunc.data(), 2, sizeof(cl_mem), &this->clCol);

	std::vector<cl_mem> objects{ this->clPos, this->clVel, this->clCol };
	err = clEnqueueAcquireGLObjects(this->commandQueue.data(), objects.size() , objects.data(), 0, nullptr, nullptr);
	if (err != CL_SUCCESS) {
		std::cerr << "clEnqueueNDRangeKernel error code " << err << std::endl;
		throw std::runtime_error("enqueue command failed.");
	}

	size_t workSize = particleCount;
	err = clEnqueueNDRangeKernel(this->commandQueue.data(), initFunc.data(), 1, nullptr, &workSize, nullptr, 0, nullptr, nullptr);
	if (err != CL_SUCCESS) {
		std::cerr << "clEnqueueNDRangeKernel error code " << err << std::endl;
		throw std::runtime_error("enqueue command failed.");
	}
	
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
	double x;
	double y;
	cl_float4 attractor;

	Application *app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));

	glfwGetWindowSize(window, &winWidth, &winHeight);
	
	glfwGetCursorPos(app->window.data(), &x, &y);
	attractor.s[0] = x / static_cast<float>(winWidth) * 2.f - 1.f;
	attractor.s[1] = -(y / static_cast<float>(winHeight) * 2.f - 1.f);
	attractor.s[2] = 0.f;
	
	if (action == GLFW_PRESS) {
		switch (key) {
			case GLFW_KEY_G:
				attractor.s[3] = 1.f;
				app->attractors.push_back(attractor);

				app->clAttractors = app->pclCreateBuffer(CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, app->attractors.size() * sizeof(cl_float4), app->attractors.data());
				break;

			case GLFW_KEY_H:
				attractor.s[3] = -1.f;
				app->attractors.push_back(attractor);

				app->clAttractors = app->pclCreateBuffer(CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, app->attractors.size() * sizeof(cl_float4), app->attractors.data());
				break;

			case GLFW_KEY_D:
				if (app->attractors.size() > 0) {
					app->attractors.erase(app->attractors.begin());
					app->clAttractors = app->pclCreateBuffer(CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, app->attractors.size() * sizeof(cl_float4), app->attractors.data());
				}
				break;

			case GLFW_KEY_O:
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA);
				break;

			case GLFW_KEY_P:
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				break;

			case GLFW_KEY_B:
				glBlendFunc(GL_ONE, GL_ZERO);
				break;

			case GLFW_KEY_SPACE:
				app->initBuffers(app->clInitPositions);
				break;

			case GLFW_KEY_KP_ADD:
				if (app->particleCount < 50000000) {
					app->particleCount += 1000000;
					app->initBuffers(app->clInitPositions);
				}
				break;

			case GLFW_KEY_KP_SUBTRACT:
				if (app->particleCount > 1000000) {
					app->particleCount -= 1000000;
					app->initBuffers(app->clInitPositions);
				}
				break;

			case GLFW_KEY_S:
				app->initBuffers(app->clInitPositionsInSphere);
				break;
		}
	}
}

void Application::mainLoop() {
	float previousTime = static_cast<float>(glfwGetTime());
	float currentTime;
	cl_float delta;
	float counter = 0;

	double mouseX;
	double mouseY;
	cl_float4 mouseVector;
	mouseVector.s[2] = 0.f;
	mouseVector.s[3] = 1.f;

	int winWidth;
	int winHeight;

	glUseProgram(this->glProgram);
	glBindVertexArray(this->vao);

	bool isCursorAttractor = false;
	bool isCursorRepulsor = false;

	while (!glfwWindowShouldClose(window.data())) {
		glfwPollEvents();
		glfwGetWindowSize(this->window.data(), &winWidth, &winHeight);

		glfwGetCursorPos(this->window.data(), &mouseX, &mouseY);
		mouseVector.s[0] = mouseX / static_cast<float>(winWidth) * 2.f - 1.f;
		mouseVector.s[1] = -(mouseY / static_cast<float>(winHeight) * 2.f - 1.f);

		currentTime = static_cast<float>(glfwGetTime());
		delta = currentTime - previousTime;
		counter += delta;
		if (counter > 1.f) {
			glfwSetWindowTitle(this->window.data(), (std::to_string(static_cast<int>(1.f / delta)) + " fps. " + std::to_string(this->particleCount) + " particles").c_str());
			counter = 0.f;
		}

		int state = glfwGetMouseButton(this->window.data(), GLFW_MOUSE_BUTTON_LEFT);
		if (state == GLFW_PRESS && !isCursorRepulsor) {
			cl_float4 attractor;
			attractor.s[0] = mouseX / static_cast<float>(winWidth) * 2.f - 1.f;
			attractor.s[1] = -(mouseY / static_cast<float>(winHeight) * 2.f - 1.f);
			attractor.s[2] = 0.f;
			attractor.s[3] = 1.f;
			this->attractors.push_back(attractor);

			isCursorAttractor = true;
			this->clAttractors = pclCreateBuffer(CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, this->attractors.size() * sizeof(cl_float4), this->attractors.data());
		}
		else {
			isCursorAttractor = false;
		}

		state = glfwGetMouseButton(this->window.data(), GLFW_MOUSE_BUTTON_RIGHT);
		if (state == GLFW_PRESS && !isCursorAttractor) {
			cl_float4 attractor;
			attractor.s[0] = mouseX / static_cast<float>(winWidth) * 2.f - 1.f;
			attractor.s[1] = -(mouseY / static_cast<float>(winHeight) * 2.f - 1.f);
			attractor.s[2] = 0.f;
			attractor.s[3] = -1.f;
			this->attractors.push_back(attractor);

			isCursorRepulsor = true;
			this->clAttractors = pclCreateBuffer(CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, this->attractors.size() * sizeof(cl_float4), this->attractors.data());
		}
		else {
			isCursorRepulsor = false;
		}

		cl_uint attractorCount = this->attractors.size();
		clSetKernelArg(this->clUpdatePositions.data(), 0, sizeof(cl_mem), &this->clPos);
		clSetKernelArg(this->clUpdatePositions.data(), 1, sizeof(cl_mem), &this->clVel);
		clSetKernelArg(this->clUpdatePositions.data(), 2, sizeof(cl_float), &delta);
		clSetKernelArg(this->clUpdatePositions.data(), 3, sizeof(cl_mem), &this->clAttractors);
		clSetKernelArg(this->clUpdatePositions.data(), 4, sizeof(cl_uint), &attractorCount);
		clSetKernelArg(this->clUpdatePositions.data(), 5, sizeof(cl_float4), &mouseVector);
		clSetKernelArg(this->clUpdatePositions.data(), 6, sizeof(cl_mem), &this->clCol);

		std::vector<cl_mem> objects{ this->clPos, this->clVel, this->clCol };
		cl_int err = clEnqueueAcquireGLObjects(this->commandQueue.data(), objects.size(), objects.data(), 0, nullptr, nullptr);
		if (err != CL_SUCCESS) {
			std::cerr << "clEnqueueNDRangeKernel error code " << err << std::endl;
			throw std::runtime_error("enqueue command failed.");
		}

		size_t workSize = particleCount;
		err = clEnqueueNDRangeKernel(this->commandQueue.data(), this->clUpdatePositions.data(), 1, nullptr, &workSize, nullptr, 0, nullptr, nullptr);
		if (err != CL_SUCCESS) {
			std::cerr << "clEnqueueNDRangeKernel error code " << err << std::endl;
			throw std::runtime_error("enqueue command failed.");
		}

		if (isCursorAttractor || isCursorRepulsor) {
			this->attractors.erase(this->attractors.end() - 1);
			this->clAttractors = pclCreateBuffer(CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, this->attractors.size() * sizeof(cl_float4), this->attractors.data());
		}

		err = clEnqueueReleaseGLObjects(this->commandQueue.data(), objects.size(), objects.data(), 0, nullptr, nullptr);
		if (err != CL_SUCCESS) {
			std::cerr << "clEnqueueNDRangeKernel error code " << err << std::endl;
			throw std::runtime_error("enqueue command failed.");
		}

		clFinish(this->commandQueue.data());

		glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_POINTS, 0, particleCount);
		glfwSwapBuffers(window.data());
		previousTime = currentTime;
	}
}
