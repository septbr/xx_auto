#pragma once

#include <Windows.h>

#include <functional>
#include <string>
#include <vector>

#ifdef XX_AUTO_SHARED
#define XX_AUTO_API __declspec(dllexport)
#else
#define XX_AUTO_API
#endif

namespace xx_auto // window
{
    template <typename T>
    struct __point {
        T x, y;
        __point &operator=(const __point &other);
        __point operator+(const __point &other) const;
        __point operator-(const __point &other) const;
        __point operator*(T value) const;
        __point operator/(T value) const;
        __point &operator+=(const __point &other);
        __point &operator-=(const __point &other);
        __point &operator*=(T value);
        __point &operator/=(T value);
        bool operator==(const __point &other) const;
        bool operator!=(const __point &other) const;
        // template <typename U>
        // operator __point<U>() const { return __point<U>(static_cast<U>(x), static_cast<U>(y)); }
    };
    using point = __point<int>;
    using pointf = __point<float>;
    using pointd = __point<double>;

    struct rect {
        int x;
        int y;
        int width;
        int height;
    };
    struct color {
        int r;
        int g;
        int b;
    };
    struct XX_AUTO_API image {
        int width, height, channel;
        std::vector<unsigned char> data;

        image();
        image(int width, int height, int channel = 3);
        void pixel(int x, int y, unsigned char color[], const int color_channel) const;
        void pixel(float x, float y, unsigned char color[], const int color_channel) const;
        color pixel(int x, int y) const;
        color pixel(float x, float y) const;
        image scoop(int x, int y, int width, int height) const;
        image scoop(int x, int y, int width, int height, const pointf &horizontal, const pointf &vertical) const;
        image resize(int new_width, int new_height) const;
        image limit(int new_width, int new_height, bool width_limit = false, bool height_limit = false) const;
    };

    enum class value_editor : char {
        replace,
        append,
        remove,
    };

    class window;
    class XX_AUTO_API menu {
    public:
        using proc_t = std::function<LRESULT(unsigned int id, bool command)>;
        static menu create(bool popup = false);

    private:
        HMENU _hmenu;

    public:
        explicit menu(const HMENU &hmenu = nullptr);
        menu(const menu &other);

    public:
        HMENU hmenu() const;
        bool operator==(const menu &other) const;
        bool operator==(HMENU hmenu) const;
        bool operator!=(const menu &other) const;
        bool operator!=(HMENU hmenu) const;
        operator bool() const;

        bool append(unsigned int id, const std::wstring &title) const;
        bool append(const menu &submenu, const std::wstring &title) const;
        bool append() const;
        bool insert(unsigned int where, unsigned int id, const std::wstring &title, bool use_position = false) const;
        bool insert(unsigned int where, const menu &submenu, const std::wstring &title, bool use_position = false) const;
        bool insert(unsigned int where, bool use_position = false) const;
        bool remove(unsigned int where, bool use_position = false) const;
        bool destroy() const;

        bool enable(unsigned int where, bool enabled, bool use_position = false) const;
        bool modify(unsigned int where, const std::wstring &title, bool use_position = false) const;
        bool check(unsigned int where, bool checked, bool use_position = false) const;

        bool show_as_context(const window &window) const;
    };

    class XX_AUTO_API window {
    public:
        enum class show_stat {
            hide = SW_HIDE,
            show = SW_SHOW,
            show_noactive = SW_SHOWNOACTIVATE,
            normal = SW_NORMAL,
            min = SW_SHOWMINIMIZED,
            max = SW_SHOWMAXIMIZED,
        };

    private:
        struct window_finds_handler {
            const wchar_t *title_name;
            const wchar_t *class_name;
            std::vector<window> &results;
        };
        static BOOL CALLBACK window_finds_proc(HWND hwnd, LPARAM param);

        static window find_impl(const wchar_t *title_name, const wchar_t *class_name);
        static std::vector<window> finds_impl(const wchar_t *title_name, const wchar_t *class_name);

    public:
        static window find(const std::wstring &title_name, const std::wstring &class_name);
        static window find(const std::wstring &name, bool by_class_name = false);
        static std::vector<window> finds(const std::wstring &title_name, const std::wstring &class_name);
        static std::vector<window> finds(const std::wstring &name, bool by_class_name = false);

    private:
        HWND _hwnd;

    public:
        explicit window(const HWND &hwnd = nullptr);
        window(const window &other);

    public:
        HWND hwnd() const;
        bool operator==(const window &other) const;
        bool operator==(HWND hwnd) const;
        bool operator!=(const window &other) const;
        bool operator!=(HWND hwnd) const;
        operator bool() const;

    private:
        window child_impl(const wchar_t *title_name, const wchar_t *class_name = nullptr) const;
        std::vector<window> children_impl(const wchar_t *title_name = nullptr, const wchar_t *class_name = nullptr) const;

    public:
        bool show(show_stat stat = show_stat::show) const;
        bool update() const;
        bool enabled() const;
        bool enabled(bool enabled) const;
        bool foreground() const;
        bool active() const;
        bool focus() const;
        bool close() const;
        bool destroy() const;

        std::wstring title() const;
        std::wstring title(const std::wstring &title) const;
        xx_auto::menu menu() const;
        xx_auto::menu menu(const xx_auto::menu &menu) const;

        window parent() const;
        void parent(const window &parent) const;
        window child(const std::wstring &title_name, const std::wstring &class_name) const;
        window child(const std::wstring &name, bool by_class_name = false) const;
        std::vector<window> children(const std::wstring &title_name, const std::wstring &class_name) const;
        std::vector<window> children(const std::wstring &name, bool by_class_name = false) const;

        long style(bool ext = false) const;
        long style(long style, bool ext = false) const;
        long style(long style, value_editor setter, bool ext = false) const;
        xx_auto::rect rect(bool client) const;
        xx_auto::rect rect() const;
        bool rect(int x, int y, int width, int height) const;
        bool rect(const xx_auto::rect &rect) const;
        point position() const;
        bool position(int x, int y) const;
        bool position(const xx_auto::point &point) const;
        bool resize(int width, int height) const;
        point screen_point(int x, int y) const;
        point screen_point(const point &client_point) const;
        point client_point(int x, int y) const;
        point client_point(const point &screen_point) const;
        bool click(int x, int y) const;
        bool click(const xx_auto::point &point) const;
        bool drag(int x, int y, int horizontal, int vertical) const;
        bool drag(const xx_auto::point &point, int horizontal, int vertical) const;
        bool drag(int x, int y, const xx_auto::point &direction) const;
        bool drag(const xx_auto::point &from, const xx_auto::point &direction) const;
    };

    class XX_AUTO_API widget : public window {
    public:
        using proc_t = std::function<LRESULT(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)>;
        inline static constexpr auto default_proc = DefWindowProcW;
        static widget create(const std::wstring &title_name, proc_t widget_proc = nullptr);

    public:
        explicit widget(const HWND &hwnd = nullptr);
        widget(const widget &other);

        xx_auto::menu menu() const;
        xx_auto::menu menu(const xx_auto::menu &menu) const;
    };

    class XX_AUTO_API capturer {
    private:
        class capturer_impl;

    private:
        capturer_impl *_impl;
        window _target;

    public:
        capturer();
        capturer(const window &target);
        capturer &operator=(const capturer &) = delete;
        capturer(capturer &&other);
        capturer &operator=(capturer &&other);
        ~capturer();

        window target() const;
        operator bool() const;
        bool valid() const;
        bool snapshot(image &rgb_output) const;
        bool snapshot(image &rgb_output, const rect &rect) const;
    };
} // namespace xx_auto

namespace xx_auto // top
{
    XX_AUTO_API window desktop();

    struct ocr_result {
        point box[3];
        float score;
        std::string text;
    };
    XX_AUTO_API ocr_result ocr(const image &rgb_image);
    XX_AUTO_API std::vector<ocr_result> ocrs(const image &rgb_image, bool rotated = false);

    XX_AUTO_API unsigned long process_id(const std::wstring &process_name);
    XX_AUTO_API std::wstring process_path(unsigned long process_id);

    XX_AUTO_API void sleep(unsigned long milliseconds);
    XX_AUTO_API bool execute(const std::wstring &command, const std::wstring &args, const std::wstring &working_directory);
    XX_AUTO_API bool execute(const std::wstring &command, const std::wstring &args);
} // namespace xx_auto

namespace xx_auto // application
{
    struct XX_AUTO_API app final {
        using entry_t = std::function<void()>;
        static void name(const std::wstring &name);
        static bool notify_menu(const menu &menu, menu::proc_t menu_proc);
        static bool exit();
    };
} // namespace xx_auto
