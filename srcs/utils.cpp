#include "utils.h"

#ifdef _WIN32
const std::string &Utils::getExecPath() {
	static std::string path;

	if (path.length() > 0) {
		return path;
	}
	else {
		char result[MAX_PATH];
		return path = std::string(result, GetModuleFileName(NULL, result, MAX_PATH));
	}
}

const std::string &Utils::getExecDir() {
	static std::string dir;

	if (dir.length() > 0) {
		return dir;
	}
	else {
		dir = getExecPath();
		dir.erase(dir.rfind('\\'), std::string::npos);
		return dir;
	}
}
#elif __APPLE__
const std::string &Utils::getExecPath() {
	static std::string path;

	if (path.length() > 0) {
		return path;
	}
	else {
		char result[PROC_PIDPATHINFO_MAXSIZE];

		int pid = getpid();
		proc_pidpath (pid, result, sizeof(result));
		return path = std::string(result);
	}
}

const std::string &Utils::getExecDir() {
	static std::string dir;

	if (dir.length() > 0) {
		return dir;
	}
	else {
		dir = getExecPath();
		dir.erase(dir.rfind('/'), std::string::npos);
		return dir;
	}
}
#endif

std::string Utils::readFile(const std::string &path) {
	std::ifstream in(path, std::ios::in | std::ios::binary);
	if (in.fail()) {
		std::stringstream ss;
		ss << "failed to open file: " << path;
		throw std::runtime_error(ss.str());
	}

	std::string contents;
	in.seekg(0, std::ios::end);
	contents.resize(static_cast<size_t>(in.tellg()));
	in.seekg(0, std::ios::beg);
	in.read(&contents[0], contents.size());
	in.close();
	return(contents);
}
