#include "xx_auto.hpp"
#undef min
#undef max

#include <TlHelp32.h>
#include <Psapi.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

namespace xx_auto // window
{
    void image::pixel(int x, int y, unsigned char color[], const int color_channel) const
    {
        if (x < 0 || y < 0 || x > width - 1 || y > height - 1)
            return;
        for (auto c = 0; c < channel && c < color_channel; ++c)
            color[c] = data[(y * width + x) * channel + c];
    }
    void image::pixel(float x, float y, unsigned char color[], const int color_channel) const
    {
        if (x < 0 || y < 0 || x > width - 1 || y > height - 1)
            return;
        auto xl = static_cast<int>(x), yl = static_cast<int>(y);
        auto xr = x > xl ? xl + 1 : xl, yr = y > yl ? yl + 1 : yl;
        auto scale_x = xr - x, scale_y = yr - y;
        for (auto c = 0; c < channel && c < color_channel; ++c)
        {
            auto v = data[(yl * width + xl) * channel + c] * scale_x * scale_y;
            v += data[(yl * width + xr) * channel + c] * (1 - scale_x) * scale_y;
            v += data[(yr * width + xl) * channel + c] * scale_x * (1 - scale_y);
            v += data[(yr * width + xr) * channel + c] * (1 - scale_x) * (1 - scale_y);
            color[c] = static_cast<unsigned char>(v);
        }
    }
    color image::pixel(int x, int y) const
    {
        unsigned char rgb[3]{};
        pixel(x, y, rgb, 3);
        return color{rgb[0], rgb[1], rgb[2]};
    }
    color image::pixel(float x, float y) const
    {
        unsigned char rgb[3]{};
        pixel(x, y, rgb, 3);
        return color{rgb[0], rgb[1], rgb[2]};
    }
    image image::scoop(int x, int y, int width, int height) const
    {
        auto output = image(width, height, channel);
        for (auto iy = 0; iy < output.height; ++iy)
        {
            for (auto ix = 0; ix < output.width; ++ix)
                pixel(x + ix, y + iy, output.data.data() + (ix + iy * output.width) * channel, channel);
        }
        return output;
    }
    image image::scoop(int x, int y, int width, int height, const pointf &horizontal, const pointf &vertical) const
    {
        auto output = image(width, height, channel);
        for (auto iy = 0; iy < output.height; ++iy)
        {
            for (auto ix = 0; ix < output.width; ++ix)
                pixel(x + horizontal.x * ix + vertical.x * iy, y + horizontal.y * ix + vertical.y * iy, output.data.data() + (ix + iy * output.width) * channel, channel);
        }
        return output;
    }
    image image::resize(int new_width, int new_height) const
    {
        auto output = image(new_width, new_height, channel);
        auto scale_x = static_cast<float>(width) / new_width, scale_y = static_cast<float>(height) / new_height;
        for (auto y = 0; y < new_height; ++y)
        {
            for (auto x = 0; x < new_width; ++x)
                pixel(x * scale_x, y * scale_y, output.data.data() + (y * new_width + x) * channel, channel);
        }
        return output;
    }
    image image::limit(int new_width, int new_height, bool width_limit, bool height_limit) const
    {
        auto width_to = width, height_to = height;
        auto width_ratio = 1.0, height_ratio = 1.0;
        if (new_width > 0 && (!width_limit || width_to > new_width))
            width_ratio = static_cast<double>(new_width) / width_to;
        if (new_height > 0 && (!height_limit || height_to > new_height))
            height_ratio = static_cast<double>(new_height) / height_to;
        if (width_limit && height_limit)
            width_ratio = height_ratio = std::min(width_ratio, height_ratio);
        else if (width_limit != height_limit)
        {
            auto ratio = width_limit ? height_ratio : width_ratio;
            if (width_limit)
            {
                width_ratio = new_width > 0 && static_cast<double>(width_to * ratio) > new_width ? static_cast<double>(new_width) / width_to : ratio;
                height_ratio = ratio;
            }
            else
            {
                width_ratio = ratio;
                height_ratio = new_height > 0 && static_cast<double>(height_to * ratio) > new_height ? static_cast<double>(new_height) / height_to : ratio;
            }
        }
        width_to = static_cast<int>(width_to * width_ratio);
        height_to = static_cast<int>(height_to * height_ratio);

        auto output = resize(width_to, height_to);
        return output;
    }

    const menu menu::none = menu(nullptr);
    bool menu::show_as_context(const window &window) const
    {
        POINT pt;
        GetCursorPos(&pt);
        return TrackPopupMenu(_hmenu, TPM_RIGHTBUTTON | TPM_NONOTIFY, pt.x, pt.y, 0, window.hwnd(), nullptr);
    }

    const window window::none = window(nullptr);
    BOOL CALLBACK window::window_finds_proc(HWND hwnd, LPARAM param)
    {
        auto handler = reinterpret_cast<window_finds_handler *>(param);
        wchar_t buff[256];
        if (handler->title_name == nullptr || (GetWindowTextW(hwnd, buff, sizeof(buff) / sizeof(buff[0])) && std::wcscmp(buff, handler->title_name) == 0))
        {
            if (handler->class_name == nullptr || (GetClassNameW(hwnd, buff, sizeof(buff) / sizeof(buff[0])) && std::wcscmp(buff, handler->class_name) == 0))
            {
                BOOL cloaked = FALSE;
                auto result = DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
                if (!SUCCEEDED(result) || !cloaked)
                    handler->results.push_back(window(hwnd));
            }
        }
        return TRUE;
    }

    const widget widget::none = widget(nullptr);
}

namespace xx_auto // top
{
    window desktop() { return window(GetDesktopWindow()); }
    unsigned long process_id(const std::wstring &process_name)
    {
        unsigned long id = 0;
        auto hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE)
            return id;

        PROCESSENTRY32W pe32{};
        pe32.dwSize = sizeof(PROCESSENTRY32W);
        if (Process32FirstW(hSnapshot, &pe32))
        {
            do
            {
                if (std::wcscmp(pe32.szExeFile, process_name.c_str()) == 0)
                {
                    id = pe32.th32ProcessID;
                    break;
                }
            } while (Process32NextW(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
        return id;
    }
    std::wstring process_path(unsigned long process_id)
    {
        std::wstring path;
        auto hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);
        if (hProcess)
        {
            wchar_t cpath[MAX_PATH];
            if (GetModuleFileNameExW(hProcess, NULL, cpath, MAX_PATH))
                path = std::wstring(cpath);
            CloseHandle(hProcess);
        }
        return path;
    }

    void sleep(unsigned long milliseconds) { Sleep(milliseconds); }
    bool execute(const std::wstring &command, const std::wstring &args, const std::wstring &working_directory)
    {
        return (INT_PTR)ShellExecuteW(nullptr, L"open", command.c_str(), args.c_str(), working_directory.c_str(), SW_SHOWNORMAL) > 32;
    }
    bool execute(const std::wstring &command, const std::wstring &args)
    {
        return (INT_PTR)ShellExecuteW(nullptr, L"open", command.c_str(), args.c_str(), nullptr, SW_SHOWNORMAL) > 32;
    }
}
