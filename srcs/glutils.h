#pragma once
#ifdef _WIN32
	#include <GL/glew.h>
#elif __APPLE__
	#include <opengl/gl3.h>
#endif

#include "utils.h"

#include <vector>
#include <string>
#include <iostream>

class GlUtils
{
public:
	static GLuint loadShader(const std::string &path, GLenum type);

private:
	GlUtils() {}
};

