#pragma once
#ifdef _WIN32
	#include <Windows.h>
#elif __APPLE__
	#include <stdlib.h>
	#include <libproc.h>
	#include <unistd.h>
#endif

#include <string>
#include <fstream>
#include <sstream>


class Utils
{
public:
	static const std::string &getExecPath();
	static const std::string &getExecDir();
	static std::string readFile(const std::string &path);

private:
	Utils() {};
};

