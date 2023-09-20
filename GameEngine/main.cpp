// GameEngine.cpp : Defines the entry point for the application.
//

#include "Window.h"
#include <filesystem>
int main()
{
#ifdef PROJECT_DIR
	std::filesystem::current_path(PROJECT_DIR);
#endif // PROJECT_DIR

	WindowApp* Window = new WindowApp();
	Window->run();
}
