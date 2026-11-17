#include "app.h"
#include <shellapi.h>

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR, int show)
{
	static App app;   
	if (!app.create(inst, show))
	{
		return 1;
	}

	int argc = 0;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	std::vector<std::wstring> files;
	for (int i = 1; argv && i < argc; ++i)
	{
		files.push_back(argv[i]);
	}
	LocalFree(argv);
	if (!files.empty())
	{
		app.addFiles(files, true);
	}

	MSG msg = {};
	while (GetMessageW(&msg, nullptr, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	return 0;
}
