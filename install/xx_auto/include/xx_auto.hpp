#pragma once

#include <Windows.h>

#include <string>
#include <vector>
#include <functional>

namespace xx_auto // window
{
    template <typename T>
    struct __point
    {
        T x, y;
        __point(T x = 0, T y = 0) : x(x), y(y) {}
        __point(const __point &other) : x(other.x), y(other.y) {}
        __point &operator=(const __point &other)
        {
            x = other.x;
            y = other.y;
            return *this;
        }
        __point operator+(const __point &other) const { return __point(x + other.x, y + other.y); }
        __point operator-(const __point &other) const { return __point(x - other.x, y - other.y); }
        __point &operator+=(const __point &other)
        {
            x += other.x;
            y += other.y;
            return *this;
        }
        __point &operator-=(const __point &other)
        {
            x -= other.x;
            y -= other.y;
            return *this;
        }
        bool operator==(const __point &other) const { return x == other.x && y == other.y; }
        bool operator!=(const __point &other) const { return x != other.x || y != other.y; }
        template <typename U>
        operator __point<U>() const { return __point<U>(static_cast<U>(x), static_cast<U>(y)); }
    };
    using point = __point<int>;
    using pointf = __point<float>;
    struct rect
    {
        long x;
        long y;
        long width;
        long height;
    };
    struct color
    {
        int r;
        int g;
        int b;
    };
    struct image
    {
        int width, height, channel;
        std::vector<unsigned char> data;

        image() : image(0, 0, 0) {}
        image(int width, int height, int channel) : width(width), height(height), channel(channel), data(width * height * channel, 0) {}
        void pixel(int x, int y, unsigned char color[], const int color_channel) const;
        void pixel(float x, float y, unsigned char color[], const int color_channel) const;
        color pixel(int x, int y) const;
        color pixel(float x, float y) const;
        image scoop(int x, int y, int width, int height) const;
        image scoop(int x, int y, int width, int height, const pointf &horizontal, const pointf &vertical) const;
        image resize(int new_width, int new_height) const;
        image limit(int new_width, int new_height, bool width_limit = false, bool height_limit = false) const;
    };

    enum class value_editor : char
    {
        replace,
        append,
        remove,
    };

    class window;
    class menu
    {
    public:
        using proc_t = std::function<LRESULT(unsigned int id, bool command)>;
        static menu create(bool popup = false);
        static const menu none;

    private:
        HMENU _hmenu;

    public:
        explicit menu(const HMENU &hmenu = nullptr) : _hmenu(hmenu) {}
        menu(const menu &other) : _hmenu(other._hmenu) {}

    public:
        HMENU hmenu() const { return _hmenu; }
        constexpr bool operator==(const menu &other) const { return _hmenu == other._hmenu; }
        constexpr bool operator==(HMENU hmenu) const { return _hmenu == hmenu; }
        constexpr bool operator!=(const menu &other) const { return _hmenu != other._hmenu; }
        constexpr bool operator!=(HMENU hmenu) const { return _hmenu != hmenu; }
        operator bool() const { return IsMenu(_hmenu); }

        bool append(unsigned short id, const std::wstring &title) const { return AppendMenuW(_hmenu, MF_STRING, id, title.c_str()); }
        bool append(const menu &submenu, const std::wstring &title) const { return AppendMenuW(_hmenu, MF_STRING | MF_POPUP, (UINT_PTR)submenu.hmenu(), title.c_str()); }
        bool append() const { return AppendMenuW(_hmenu, MF_SEPARATOR, 0, nullptr); }
        bool insert(unsigned short where, unsigned short id, const std::wstring &title, bool use_position = false) const { return InsertMenuW(_hmenu, where, MF_STRING | (use_position ? MF_BYPOSITION : MF_BYCOMMAND), id, title.c_str()); }
        bool insert(unsigned short where, const menu &submenu, const std::wstring &title, bool use_position = false) const { return InsertMenuW(_hmenu, where, MF_STRING | MF_POPUP | (use_position ? MF_BYPOSITION : MF_BYCOMMAND), (UINT_PTR)submenu.hmenu(), title.c_str()); }
        bool insert(unsigned short where, bool use_position = false) const { return InsertMenuW(_hmenu, where, MF_SEPARATOR | (use_position ? MF_BYPOSITION : MF_BYCOMMAND), 0, nullptr); }
        bool remove(unsigned short where, bool use_position = false) const { return RemoveMenu(_hmenu, where, use_position ? MF_BYPOSITION : MF_BYCOMMAND); }
        bool destroy() const;

        bool enable(unsigned short where, bool enabled, bool use_position = false) const { return EnableMenuItem(_hmenu, where, (enabled ? MF_ENABLED : MF_DISABLED | MF_GRAYED) | (use_position ? MF_BYPOSITION : MF_BYCOMMAND)); }
        bool modify(unsigned short where, const std::wstring &title, bool use_position = false) const { return ModifyMenuW(_hmenu, where, MF_STRING | (use_position ? MF_BYPOSITION : MF_BYCOMMAND), where, title.c_str()); }
        bool check(unsigned short where, bool checked, bool use_position = false) const { return CheckMenuItem(_hmenu, where, (checked ? MF_CHECKED : MF_UNCHECKED) | (use_position ? MF_BYPOSITION : MF_BYCOMMAND)); }

        bool show_as_context(const window &window) const;
    };

    class window
    {
    public:
        enum class show_stat
        {
            hide = SW_HIDE,
            show = SW_SHOW,
            show_noactive = SW_SHOWNOACTIVATE,
            normal = SW_NORMAL,
            min = SW_SHOWMINIMIZED,
            max = SW_SHOWMAXIMIZED,
        };

    private:
        struct window_finds_handler
        {
            const wchar_t *title_name;
            const wchar_t *class_name;
            std::vector<window> &results;
        };
        static BOOL CALLBACK window_finds_proc(HWND hwnd, LPARAM param);

        static window find_impl(const wchar_t *title_name, const wchar_t *class_name) { return window(FindWindowW(class_name, title_name)); }
        static std::vector<window> finds_impl(const wchar_t *title_name, const wchar_t *class_name)
        {
            std::vector<window> windows;
            window_finds_handler handler{title_name, class_name, windows};
            EnumWindows(window_finds_proc, reinterpret_cast<LPARAM>(&handler));
            return windows;
        }

    public:
        static window find(const std::wstring &title_name, const std::wstring &class_name) { return find_impl(title_name.c_str(), class_name.c_str()); }
        static window find(const std::wstring &name, bool by_class_name = false) { return find_impl(by_class_name ? nullptr : name.c_str(), by_class_name ? name.c_str() : nullptr); }
        static std::vector<window> finds(const std::wstring &title_name, const std::wstring &class_name) { return finds_impl(title_name.c_str(), class_name.c_str()); }
        static std::vector<window> finds(const std::wstring &name, bool by_class_name = false) { return finds_impl(by_class_name ? nullptr : name.c_str(), by_class_name ? name.c_str() : nullptr); }
        static const window none;

    private:
        HWND _hwnd;

    public:
        explicit window(const HWND &hwnd = nullptr) : _hwnd(hwnd) {}
        window(const window &other) : _hwnd(other._hwnd) {}

    public:
        HWND hwnd() const { return _hwnd; }
        constexpr bool operator==(const window &other) const { return _hwnd == other._hwnd; }
        constexpr bool operator==(HWND hwnd) const { return _hwnd == hwnd; }
        constexpr bool operator!=(const window &other) const { return _hwnd != other._hwnd; }
        constexpr bool operator!=(HWND hwnd) const { return _hwnd != hwnd; }
        operator bool() const { return IsWindow(_hwnd); }

    private:
        window child_impl(const wchar_t *title_name, const wchar_t *class_name = nullptr) const { return window(FindWindowExW(_hwnd, nullptr, class_name, title_name)); }
        std::vector<window> children_impl(const wchar_t *title_name = nullptr, const wchar_t *class_name = nullptr) const
        {
            std::vector<window> windows;
            window_finds_handler handler{title_name, class_name, windows};
            EnumChildWindows(_hwnd, window_finds_proc, reinterpret_cast<LPARAM>(&handler));
            return windows;
        }

    public:
        bool show(show_stat stat = show_stat::show) const { return ShowWindow(_hwnd, (int)stat); }
        bool update() const { return UpdateWindow(_hwnd); }
        bool enabled() const { return IsWindowEnabled(_hwnd); }
        bool enabled(bool enabled) const { return EnableWindow(_hwnd, enabled) && enabled; }
        bool foreground() const { return SetForegroundWindow(_hwnd); }
        bool active() const { return SetActiveWindow(_hwnd); }
        bool focus() const { return SetFocus(_hwnd); }
        bool close() const { return CloseWindow(_hwnd); }
        bool destroy() const { return DestroyWindow(_hwnd); }

        std::wstring title() const
        {
            auto length = GetWindowTextLengthW(_hwnd);
            wchar_t *buffer = new wchar_t[length + 1];
            GetWindowTextW(_hwnd, buffer, length + 1);
            auto text = std::wstring(buffer);
            delete[] buffer;
            return text;
        }
        std::wstring title(const std::wstring &title) const { return SetWindowTextW(_hwnd, title.c_str()) ? title : std::wstring(); }
        xx_auto::menu menu() const { return xx_auto::menu(GetMenu(_hwnd)); }
        xx_auto::menu menu(const xx_auto::menu &menu) const { return SetMenu(_hwnd, menu.hmenu()) ? menu : xx_auto::menu::none; }

        window parent() const { return window(GetParent(_hwnd)); }
        void parent(const window &parent) const { SetParent(_hwnd, parent._hwnd); }
        window child(const std::wstring &title_name, const std::wstring &class_name) const { return child_impl(title_name.c_str(), class_name.c_str()); }
        window child(const std::wstring &name, bool by_class_name = false) const { return child_impl(by_class_name ? nullptr : name.c_str(), by_class_name ? name.c_str() : nullptr); }
        std::vector<window> children(const std::wstring &title_name, const std::wstring &class_name) const { return children_impl(title_name.c_str(), class_name.c_str()); }
        std::vector<window> children(const std::wstring &name, bool by_class_name = false) const { return children_impl(by_class_name ? nullptr : name.c_str(), by_class_name ? name.c_str() : nullptr); }

        long style(bool ext = false) const { return GetWindowLongW(_hwnd, ext ? GWL_EXSTYLE : GWL_STYLE); }
        long style(long style, bool ext = false) const { return SetWindowLongW(_hwnd, ext ? GWL_EXSTYLE : GWL_STYLE, style); }
        long style(long style, value_editor setter, bool ext = false) const
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
        xx_auto::rect rect(bool client) const
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
        xx_auto::rect rect() const { return rect(false); }
        bool rect(long x, long y, long width, long height) const { return SetWindowPos(_hwnd, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE); }
        bool rect(const xx_auto::rect &rect) const { return this->rect(rect.x, rect.y, rect.width, rect.height); }
        bool move(long x, long y) const { return SetWindowPos(_hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE); }
        bool resize(long width, long height) const { return SetWindowPos(_hwnd, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE); }
        bool click(long x, long y) const
        {
            auto lParam = MAKELPARAM(x, y);
            return PostMessageW(_hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lParam) && PostMessageW(_hwnd, WM_LBUTTONUP, MK_LBUTTON, lParam);
        }
    };

    class widget : public window
    {
    public:
        static const widget none;

    public:
        using proc_t = std::function<LRESULT(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)>;
        inline static constexpr auto default_proc = DefWindowProcW;
        static widget create(const std::wstring &title_name, proc_t widget_proc = nullptr);

    public:
        explicit widget(const HWND &hwnd = nullptr) : window(hwnd) {}
        widget(const widget &other) : window(other) {}

        xx_auto::menu menu() const { return xx_auto::menu(GetMenu(hwnd())); }
        xx_auto::menu menu(const xx_auto::menu &menu) const { return SetMenu(hwnd(), menu.hmenu()) ? menu : xx_auto::menu::none; }
    };

    class capturer
    {
    private:
        class capturer_impl;

    private:
        capturer_impl *_impl;
        HWND _target;

    public:
        capturer(HWND target = nullptr);
        capturer(const window &window) : capturer(window.hwnd()) {}
        capturer(const widget &widget) : capturer(widget.hwnd()) {}
        capturer &operator=(const capturer &) = delete;
        capturer(capturer &&other) { *this = std::move(other); }
        capturer &operator=(capturer &&other);
        ~capturer();

        operator bool() const { return valid(); }
        bool valid() const;
        bool capture(image &rgb_output) const;
    };
}

namespace xx_auto // top
{
    window desktop();

    struct ocr_result
    {
        point box[3];
        float score;
        std::string text;
    };
    ocr_result ocr(const image &rgb_image);
    std::vector<ocr_result> ocrs(const image &rgb_image, bool rotated = false);

    unsigned long process_id(const std::wstring &process_name);
    std::wstring process_path(unsigned long process_id);

    void sleep(unsigned long milliseconds);
    bool execute(const std::wstring &command, const std::wstring &args, const std::wstring &working_directory);
    bool execute(const std::wstring &command, const std::wstring &args);
}

namespace xx_auto // application
{
    struct app final
    {
        using entry_t = std::function<void()>;
        static void name(const std::wstring &name);
        static bool notify_menu(const menu &menu, menu::proc_t menu_proc);
        static bool exit();

        static int run(entry_t entry, bool singleton = false);
    };
}
