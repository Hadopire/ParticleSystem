#ifdef _WIN32
	#include <Windows.h>
#elif defined (__APPLE__)
	#define __gl_h_
	#define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
#endif

#include "application.h"

#include <stdexcept>
#include <iostream>

int main()
{
	try {
		Application app;
		app.run();
	} catch (std::runtime_error &e) {
		std::cerr << e.what() << std::endl;
		std::cin.get();
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
