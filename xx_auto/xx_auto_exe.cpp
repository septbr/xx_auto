#include "xx_auto.hpp"
#include "xx_auto_internal.hpp"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    std::wstring entry_path(L"xx_auto_entry");
    if (lpCmdLine && *lpCmdLine)
    {
        int argc;
        auto argv = CommandLineToArgvW(lpCmdLine, &argc);
        if (argv)
        {
            if (argc > 0)
                entry_path = argv[0];
            LocalFree(argv);
        }
    }
    auto moudle = LoadLibraryW(entry_path.c_str());
    if (!moudle)
    {
        MessageBoxW(0, (entry_path + L": moudle load failed").c_str(), L"xx_auto", MB_ICONERROR | MB_OK);
        return -1;
    }
    auto entry = reinterpret_cast<void (*)()>(GetProcAddress(moudle, "xx_auto_entry"));
    if (!entry)
    {
        MessageBoxW(0, (entry_path + L": entry load failed").c_str(), L"xx_auto", MB_ICONERROR | MB_OK);
        return -2;
    }
    return xx_auto::internal::run_entry(entry, false);
}
