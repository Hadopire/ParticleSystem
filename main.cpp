#ifdef _WIN32
	#include <Windows.h>
#endif

#include "application.h"

#include <stdexcept>
#include <iostream>

extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

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
