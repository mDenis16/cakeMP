// CakeMPCefLauncher.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include <include/cef_app.h>
#include <include/cef_browser.h>
#include <include/cef_client.h>
#include <include/cef_version.h>
#include <CakeWebView.h>
#include <CefModule.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    std::cout << "Initializing cef external \n";

	CefEnableHighDPISupport();

	void* sandbox_info = NULL;

	CefMainArgs main_args(hInstance);

	CefSettings settings;
	settings.no_sandbox = true;

	settings.windowless_rendering_enabled = true;
	CefRefPtr<WebApp> launcher(new WebApp());

	CefInitialize(main_args, settings, launcher.get(), sandbox_info);

	CefRunMessageLoop();

	CefShutdown();
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file


