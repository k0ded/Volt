//#pragma once
//
//#include "Volt/Core/Application.h"
//#include <filesystem>
//
//extern Volt::Application* Volt::CreateApplication(const std::filesystem::path& appPath);
//
//namespace Volt
//{
//	void Create(const std::filesystem::path& appPath)
//	{
//		Application* app = Volt::CreateApplication(appPath);
//		app->Run();
//
//		delete app;
//	}
//
//	int Main(const std::filesystem::path& appPath)
//	{
//		Create(appPath);
//		return 0;
//	}
//}
//
//#ifdef VT_DIST
//
//#include <CoreUtilities/Platform/Windows/VoltWindows.h>
//
//int APIENTRY WinMain(HINSTANCE aHInstance, HINSTANCE aPrevHInstance, PSTR aCmdLine, int aCmdShow)
//{

//	
//	if (nArgs > 1)
//	{
//		return Volt::Main(szArglist[1]);
//	}
//
//	return Volt::Main("");
//}
//
//#else
//
//int main(int argc, char** argv)
//{
//	if (argc > 1)
//	{
//		return Volt::Main(argv[1]);
//	}
//
//	return Volt::Main("");
//}
//
//#endif
