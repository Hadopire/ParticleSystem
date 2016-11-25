#pragma once
#include <GL/glew.h>
#include <string>
#include "utils.h"
#include <vector>
#include <iostream>

class GlUtils
{
public:
	static GLuint loadShader(const std::string &path, GLenum type);

private:
	GlUtils() {}
};

