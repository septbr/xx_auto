#include "xx_auto.hpp"

LRESULT widget_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_DESTROY: PostQuitMessage(0); break;
    }
    return xx_auto::widget::default_proc(hwnd, message, wParam, lParam);
}

extern "C" __declspec(dllexport) void xx_auto_entry() {
    auto widget = xx_auto::widget::create(L"xx_auto", widget_proc);
    widget.resize(600, 400);
    widget.show();
}