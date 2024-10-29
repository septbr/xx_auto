#include "xx_auto.hpp"
#undef min
#undef max

#include <TlHelp32.h>
#include <Psapi.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

namespace xx_auto // window
{
    template <typename T>
    __point<T>::__point(T x, T y) : x(x), y(y) {}
    template <typename T>
    __point<T>::__point(const __point &other) : x(other.x), y(other.y) {}
    template <typename T>
    __point<T> &__point<T>::operator=(const __point &other)
    {
        x = other.x;
        y = other.y;
        return *this;
    }
    template <typename T>
    __point<T> __point<T>::operator+(const __point &other) const { return __point(x + other.x, y + other.y); }
    template <typename T>
    __point<T> __point<T>::operator-(const __point &other) const { return __point(x - other.x, y - other.y); }
    template <typename T>
    __point<T> &__point<T>::operator+=(const __point &other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }
    template <typename T>
    __point<T> &__point<T>::operator-=(const __point &other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    template <typename T>
    bool __point<T>::operator==(const __point &other) const { return x == other.x && y == other.y; }
    template <typename T>
    bool __point<T>::operator!=(const __point &other) const { return x != other.x || y != other.y; }
    // template <typename T>
    // template <typename U>
    // __point<T>::operator __point<U>() const { return __point<U>(static_cast<U>(x), static_cast<U>(y)); }
    template class XX_AUTO_API __point<int>;
    template class XX_AUTO_API __point<float>;
    template class XX_AUTO_API __point<double>;
    // template XX_AUTO_API __point<int>::operator __point<float>() const;
    // template XX_AUTO_API __point<int>::operator __point<double>() const;
    // template XX_AUTO_API __point<float>::operator __point<int>() const;
    // template XX_AUTO_API __point<float>::operator __point<double>() const;
    // template XX_AUTO_API __point<double>::operator __point<int>() const;
    // template XX_AUTO_API __point<double>::operator __point<float>() const;

    image::image() : image(0, 0, 0) {}
    image::image(int width, int height, int channel) : width(width), height(height), channel(channel), data(width * height * channel, 0) {}

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

    menu::menu(const HMENU &hmenu) : _hmenu(hmenu) {}
    menu::menu(const menu &other) : _hmenu(other._hmenu) {}
    HMENU menu::hmenu() const { return _hmenu; }
    bool menu::operator==(const menu &other) const { return _hmenu == other._hmenu; }
    bool menu::operator==(HMENU hmenu) const { return _hmenu == hmenu; }
    bool menu::operator!=(const menu &other) const { return _hmenu != other._hmenu; }
    bool menu::operator!=(HMENU hmenu) const { return _hmenu != hmenu; }
    menu::operator bool() const { return IsMenu(_hmenu); }
    bool menu::append(unsigned short id, const std::wstring &title) const { return AppendMenuW(_hmenu, MF_STRING, id, title.c_str()); }
    bool menu::append(const menu &submenu, const std::wstring &title) const { return AppendMenuW(_hmenu, MF_STRING | MF_POPUP, (UINT_PTR)submenu.hmenu(), title.c_str()); }
    bool menu::append() const { return AppendMenuW(_hmenu, MF_SEPARATOR, 0, nullptr); }
    bool menu::insert(unsigned short where, unsigned short id, const std::wstring &title, bool use_position) const { return InsertMenuW(_hmenu, where, MF_STRING | (use_position ? MF_BYPOSITION : MF_BYCOMMAND), id, title.c_str()); }
    bool menu::insert(unsigned short where, const menu &submenu, const std::wstring &title, bool use_position) const { return InsertMenuW(_hmenu, where, MF_STRING | MF_POPUP | (use_position ? MF_BYPOSITION : MF_BYCOMMAND), (UINT_PTR)submenu.hmenu(), title.c_str()); }
    bool menu::insert(unsigned short where, bool use_position) const { return InsertMenuW(_hmenu, where, MF_SEPARATOR | (use_position ? MF_BYPOSITION : MF_BYCOMMAND), 0, nullptr); }
    bool menu::remove(unsigned short where, bool use_position) const { return RemoveMenu(_hmenu, where, use_position ? MF_BYPOSITION : MF_BYCOMMAND); }
    bool menu::enable(unsigned short where, bool enabled, bool use_position) const { return EnableMenuItem(_hmenu, where, (enabled ? MF_ENABLED : MF_DISABLED | MF_GRAYED) | (use_position ? MF_BYPOSITION : MF_BYCOMMAND)); }
    bool menu::modify(unsigned short where, const std::wstring &title, bool use_position) const { return ModifyMenuW(_hmenu, where, MF_STRING | (use_position ? MF_BYPOSITION : MF_BYCOMMAND), where, title.c_str()); }
    bool menu::check(unsigned short where, bool checked, bool use_position) const { return CheckMenuItem(_hmenu, where, (checked ? MF_CHECKED : MF_UNCHECKED) | (use_position ? MF_BYPOSITION : MF_BYCOMMAND)); }
    bool menu::show_as_context(const window &window) const
    {
        POINT pt;
        GetCursorPos(&pt);
        return TrackPopupMenu(_hmenu, TPM_RIGHTBUTTON | TPM_NONOTIFY, pt.x, pt.y, 0, window.hwnd(), nullptr);
    }

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
    window window::find_impl(const wchar_t *title_name, const wchar_t *class_name) { return window(FindWindowW(class_name, title_name)); }
    std::vector<window> window::finds_impl(const wchar_t *title_name, const wchar_t *class_name)
    {
        std::vector<window> windows;
        window_finds_handler handler{title_name, class_name, windows};
        EnumWindows(window_finds_proc, reinterpret_cast<LPARAM>(&handler));
        return windows;
    }
    window window::find(const std::wstring &title_name, const std::wstring &class_name) { return find_impl(title_name.c_str(), class_name.c_str()); }
    window window::find(const std::wstring &name, bool by_class_name) { return find_impl(by_class_name ? nullptr : name.c_str(), by_class_name ? name.c_str() : nullptr); }
    std::vector<window> window::finds(const std::wstring &title_name, const std::wstring &class_name) { return finds_impl(title_name.c_str(), class_name.c_str()); }
    std::vector<window> window::finds(const std::wstring &name, bool by_class_name) { return finds_impl(by_class_name ? nullptr : name.c_str(), by_class_name ? name.c_str() : nullptr); }

    window::window(const HWND &hwnd) : _hwnd(hwnd) {}
    window::window(const window &other) : _hwnd(other._hwnd) {}
    HWND window::hwnd() const { return _hwnd; }
    bool window::operator==(const window &other) const { return _hwnd == other._hwnd; }
    bool window::operator==(HWND hwnd) const { return _hwnd == hwnd; }
    bool window::operator!=(const window &other) const { return _hwnd != other._hwnd; }
    bool window::operator!=(HWND hwnd) const { return _hwnd != hwnd; }
    window::operator bool() const { return IsWindow(_hwnd); }
    window window::child_impl(const wchar_t *title_name, const wchar_t *class_name) const { return window(FindWindowExW(_hwnd, nullptr, class_name, title_name)); }
    std::vector<window> window::children_impl(const wchar_t *title_name, const wchar_t *class_name) const
    {
        std::vector<window> windows;
        window_finds_handler handler{title_name, class_name, windows};
        EnumChildWindows(_hwnd, window_finds_proc, reinterpret_cast<LPARAM>(&handler));
        return windows;
    }
    bool window::show(show_stat stat) const { return ShowWindow(_hwnd, (int)stat); }
    bool window::update() const { return UpdateWindow(_hwnd); }
    bool window::enabled() const { return IsWindowEnabled(_hwnd); }
    bool window::enabled(bool enabled) const { return EnableWindow(_hwnd, enabled) && enabled; }
    bool window::foreground() const { return SetForegroundWindow(_hwnd); }
    bool window::active() const { return SetActiveWindow(_hwnd); }
    bool window::focus() const { return SetFocus(_hwnd); }
    bool window::close() const { return CloseWindow(_hwnd); }
    bool window::destroy() const { return DestroyWindow(_hwnd); }
    std::wstring window::title() const
    {
        auto length = GetWindowTextLengthW(_hwnd);
        wchar_t *buffer = new wchar_t[length + 1];
        GetWindowTextW(_hwnd, buffer, length + 1);
        auto text = std::wstring(buffer);
        delete[] buffer;
        return text;
    }
    std::wstring window::title(const std::wstring &title) const { return SetWindowTextW(_hwnd, title.c_str()) ? title : std::wstring(); }
    xx_auto::menu window::menu() const { return xx_auto::menu(GetMenu(_hwnd)); }
    xx_auto::menu window::menu(const xx_auto::menu &menu) const { return SetMenu(_hwnd, menu.hmenu()) ? menu : xx_auto::menu(); }

    window window::parent() const { return window(GetParent(_hwnd)); }
    void window::parent(const window &parent) const { SetParent(_hwnd, parent._hwnd); }
    window window::child(const std::wstring &title_name, const std::wstring &class_name) const { return child_impl(title_name.c_str(), class_name.c_str()); }
    window window::child(const std::wstring &name, bool by_class_name) const { return child_impl(by_class_name ? nullptr : name.c_str(), by_class_name ? name.c_str() : nullptr); }
    std::vector<window> window::children(const std::wstring &title_name, const std::wstring &class_name) const { return children_impl(title_name.c_str(), class_name.c_str()); }
    std::vector<window> window::children(const std::wstring &name, bool by_class_name) const { return children_impl(by_class_name ? nullptr : name.c_str(), by_class_name ? name.c_str() : nullptr); }

    long window::style(bool ext) const { return GetWindowLongW(_hwnd, ext ? GWL_EXSTYLE : GWL_STYLE); }
    long window::style(long style, bool ext) const { return SetWindowLongW(_hwnd, ext ? GWL_EXSTYLE : GWL_STYLE, style); }
    long window::style(long style, value_editor setter, bool ext) const
    {
        switch (setter)
        {
        case value_editor::replace:
            return SetWindowLongW(_hwnd, ext ? GWL_EXSTYLE : GWL_STYLE, style);
        case value_editor::append:
            return SetWindowLongW(_hwnd, ext ? GWL_EXSTYLE : GWL_STYLE, style | this->style(ext));
        case value_editor::remove:
            return SetWindowLongW(_hwnd, ext ? GWL_EXSTYLE : GWL_STYLE, style & ~this->style(ext));
        }
        return 0;
    }
    xx_auto::rect window::rect(bool client) const
    {
        RECT rct{};
        if (client)
            GetClientRect(_hwnd, &rct);
        else
        {
            GetWindowRect(_hwnd, &rct);
            HWND parent = GetParent(_hwnd);
            if (parent)
            {
                POINT pt = {rct.left, rct.top};
                ScreenToClient(parent, &pt);
                rct.right = rct.right + pt.x - rct.left;
                rct.bottom = rct.bottom + pt.y - rct.top;
                rct.left = pt.x;
                rct.top = pt.y;
            }
        }
        return xx_auto::rect{rct.left, rct.top, rct.right - rct.left, rct.bottom - rct.top};
    }
    xx_auto::rect window::rect() const { return rect(false); }
    bool window::rect(long x, long y, long width, long height) const { return SetWindowPos(_hwnd, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE); }
    bool window::rect(const xx_auto::rect &rect) const { return this->rect(rect.x, rect.y, rect.width, rect.height); }
    bool window::move(long x, long y) const { return SetWindowPos(_hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE); }
    bool window::resize(long width, long height) const { return SetWindowPos(_hwnd, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE); }
    bool window::click(long x, long y) const
    {
        auto lParam = MAKELPARAM(x, y);
        return PostMessageW(_hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lParam) && PostMessageW(_hwnd, WM_LBUTTONUP, MK_LBUTTON, lParam);
    }

    widget::widget(const HWND &hwnd) : window(hwnd) {}
    widget::widget(const widget &other) : window(other) {}
    xx_auto::menu widget::menu() const { return xx_auto::menu(GetMenu(hwnd())); }
    xx_auto::menu widget::menu(const xx_auto::menu &menu) const { return SetMenu(hwnd(), menu.hmenu()) ? menu : xx_auto::menu(); }
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
