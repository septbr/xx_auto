#include "xx_auto.hpp"
#include "xx_auto_internal.hpp"
#include "xx_auto_rc.h"
#undef min
#undef max

#include <unordered_map>

namespace xx_auto // application
{
    class app_impl
    {
        friend widget widget::create(const std::wstring &title_name, widget::proc_t widget_proc);
        friend menu menu::create(bool popup);
        friend bool menu::destroy() const;

    private:
        inline static constexpr const wchar_t *APP_CLASS_NAME = L"xx_auto";
        inline static constexpr const wchar_t *WIDGET_CLASS_NAME = L"xx_auto_widget";
        inline static constexpr int WM_APP_NOTIFYTRAY = WM_USER + 1;
        inline static constexpr int WM_APP_EXIT = 1;
        inline static bool class_registered = false;

        inline static bool _running = false;
        inline static widget _app_widget = widget();
        inline static std::unordered_map<HWND, widget::proc_t> _widgets{};
        inline static NOTIFYICONDATA _app_notify{};
        inline static menu _app_notify_menu = menu();
        inline static menu::proc_t _app_notify_menu_proc = nullptr;
        inline static std::vector<HMENU> _menus{};

        static LRESULT app_widget_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
        {
            switch (message)
            {
            case WM_CLOSE:
                _app_widget.show(window::show_stat::hide);
                return 0;
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
            case WM_COMMAND:
            {
                auto id = LOWORD(wParam);
                if (_app_notify_menu_proc)
                    return _app_notify_menu_proc(id, true);
                switch (id)
                {
                case WM_APP_EXIT:
                    _app_widget.destroy();
                    return 0;
                }
                break;
            }
            case WM_APP_NOTIFYTRAY:
            {
                auto id = LOWORD(lParam);
                switch (id)
                {
                case WM_RBUTTONDOWN:
                    _app_widget.foreground();
                    _app_notify_menu.show_as_context(_app_widget);
                    break;
                }
                if (_app_notify_menu_proc)
                    return _app_notify_menu_proc(id, false);
                break;
            }
            default:
                if (message == RegisterWindowMessageW(L"TaskbarCreated"))
                    Shell_NotifyIconW(NIM_ADD, &_app_notify);
                break;
            }
            return widget::default_proc(hwnd, message, wParam, lParam);
        }
        static bool create()
        {
            if (_running)
                return false;
            if (!class_registered)
            {
                auto hInstance = GetModuleHandleW(nullptr);
                for (auto class_name : {APP_CLASS_NAME, WIDGET_CLASS_NAME})
                {
                    WNDCLASS wc{};
                    wc.style = CS_HREDRAW | CS_VREDRAW;
                    wc.hInstance = hInstance;
                    wc.lpszClassName = class_name;
                    wc.hbrBackground = (HBRUSH)COLOR_WINDOWFRAME;
                    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_ICON));
                    wc.lpfnWndProc = widget_proc;
                    RegisterClassW(&wc);
                }
                class_registered = true;
            }

            _app_widget = create_widget(APP_CLASS_NAME, app_widget_proc, APP_CLASS_NAME);
            if (_app_widget)
            {
                internal::init_ocr(L"models/PPOCRv3/ch_PP-OCRv3_det.onnx", L"models/PPOCRv3/ch_ppocr_mobile_v2.0_cls.onnx", L"models/PPOCRv3/ch_PP-OCRv3_rec.onnx", L"models/PPOCRv3/ppocr_keys_v1.txt");

                _running = true;
                _app_widget.style(WS_OVERLAPPED, false);
                _app_widget.update();

                _app_notify = {};
                _app_notify.cbSize = sizeof(NOTIFYICONDATA);
                _app_notify.hWnd = _app_widget.hwnd();
                _app_notify.uID = 0;
                _app_notify.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
                _app_notify.uCallbackMessage = WM_APP_NOTIFYTRAY;
                _app_notify.hIcon = (HICON)GetClassLongPtrW(_app_widget.hwnd(), GCLP_HICON);
                wcscpy_s(_app_notify.szTip, sizeof(_app_notify.szTip) / sizeof(_app_notify.szTip[0]), APP_CLASS_NAME);
                Shell_NotifyIconW(NIM_ADD, &_app_notify);

                notify_menu(menu(), nullptr);
            }
            return !!_app_widget;
        }
        static bool destroy()
        {
            internal::uninit_ocr();

            Shell_NotifyIconW(NIM_DELETE, &_app_notify);
            _widgets.clear();
            for (auto menu : _menus)
                DestroyMenu(menu);
            _menus.clear();
            _app_notify = {};
            _app_notify_menu_proc = nullptr;
            _app_notify_menu = menu();
            _app_widget = widget();
            _running = false;
            return true;
        }
        static LRESULT CALLBACK widget_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
        {
            if (auto it = _widgets.find(hwnd); it != _widgets.end())
            {
                if (it->second)
                {
                    auto result = it->second(hwnd, message, wParam, lParam);
                    if (message == WM_DESTROY)
                        _widgets.erase(it);
                    return result;
                }
            }
            return widget::default_proc(hwnd, message, wParam, lParam);
        }

        static menu create_menu(bool popup)
        {
            auto hmenu = popup ? CreatePopupMenu() : CreateMenu();
            if (hmenu)
                _menus.push_back(hmenu);
            return menu(hmenu);
        }
        static widget create_widget(const std::wstring &title_name, widget::proc_t widget_proc = nullptr, const std::wstring &class_name = WIDGET_CLASS_NAME)
        {
            auto widget = xx_auto::widget(CreateWindowW(
                class_name.c_str(), title_name.c_str(),
                WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                nullptr,
                nullptr, GetModuleHandleW(nullptr), nullptr));
            if (widget)
            {
                auto widget_proc_fptr = widget_proc.target<decltype(widget::default_proc)>();
                _widgets[widget.hwnd()] = widget_proc_fptr && *widget_proc_fptr == widget::default_proc ? nullptr : widget_proc;
            }
            return widget;
        }

        static void refresh_menus()
        {
            for (auto it = _menus.begin(); it != _menus.end();)
            {
                if (!IsMenu(*it))
                    it = _menus.erase(it);
                else
                    ++it;
            }
        }

    public:
        static void name(const std::wstring &name)
        {
            if (!_running)
                return;
            _app_widget.title(name);
            wcscpy_s(_app_notify.szTip, sizeof(_app_notify.szTip) / sizeof(_app_notify.szTip[0]), name.c_str());
        }
        static bool notify_menu(const menu &menu, menu::proc_t menu_proc)
        {
            if (!_running)
                return false;
            if (menu && menu_proc)
            {
                _app_notify_menu = menu;
                _app_notify_menu_proc = menu_proc;
                return true;
            }
            _app_notify_menu = create_menu(true);
            _app_notify_menu_proc = nullptr;
            _app_notify_menu.append(WM_APP_EXIT, L"退出");
            return false;
        }
        static bool exit() { return _running && _app_widget.destroy(); }

        static int run(app::entry_t entry, bool singleton = false)
        {
            if (_running)
            {
                MessageBoxW(nullptr, L"Application is running.", APP_CLASS_NAME, MB_OK | MB_ICONERROR);
                return -1;
            }

            HANDLE instanceMutex = nullptr;
            if (singleton)
            {
                instanceMutex = CreateMutexW(NULL, FALSE, APP_CLASS_NAME);
                if (!instanceMutex)
                {
                    MessageBoxW(nullptr, L"Application failed to run!", APP_CLASS_NAME, MB_OK | MB_ICONERROR);
                    return -1;
                }
                auto lastError = GetLastError();
                if (lastError == ERROR_ALREADY_EXISTS)
                {
                    CloseHandle(instanceMutex);
                    return -1;
                }
            }

            if (!create())
            {
                MessageBoxW(nullptr, L"Application failed to run!", APP_CLASS_NAME, MB_OK | MB_ICONERROR);
                return -1;
            }
            try
            {
                entry();
            }
            catch (...)
            {
            }
            MSG msg;
            while (GetMessageW(&msg, nullptr, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            destroy();

            if (singleton && instanceMutex)
                CloseHandle(instanceMutex);

            return (int)msg.wParam;
        }
    };

    menu menu::create(bool popup) { return app_impl::create_menu(popup); }
    bool menu::destroy() const
    {
        auto result = DestroyMenu(_hmenu);
        app_impl::refresh_menus();
        return result;
    }
    widget widget::create(const std::wstring &title_name, widget::proc_t widget_proc) { return app_impl::create_widget(title_name, widget_proc); }

    void app::name(const std::wstring &name) { app_impl::name(name); }
    bool app::notify_menu(const menu &menu, menu::proc_t menu_proc) { return app_impl::notify_menu(menu, menu_proc); }
    bool app::exit() { return app_impl::exit(); }

    int internal::run_entry(app::entry_t entry, bool singleton) { return app_impl::run(entry, singleton); }
}
