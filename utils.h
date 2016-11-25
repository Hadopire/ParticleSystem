#pragma once
#ifdef _WIN32
	#include <Windows.h>
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

