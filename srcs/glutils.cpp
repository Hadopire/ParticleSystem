#include "glutils.h"

GLuint GlUtils::loadShader(const std::string &path, GLenum type) {
	std::string content = Utils::readFile(path);
	const char *data = content.data();
	const GLint len = content.length();

	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &data, &len);
	glCompileShader(shader);

	GLint compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		GLsizei logSize;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);
		std::vector<char> log(logSize);
		glGetShaderInfoLog(shader, logSize, nullptr, log.data());
		std::cerr << log.data();
		throw std::runtime_error("failed to compile shader at path: " + path);
	}

	return shader;
}
