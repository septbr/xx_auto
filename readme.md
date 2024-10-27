# xx_auto
Windows自动化C++库，集成OCR

```c++
#include "xx_auto.hpp"

void my_entry()
{
    MessageBoxW("Hello World!", "xx_auto", MB_OK);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    return xx_auto::app::run(my_entry);
}

```

## Thanks
- [onnxruntime](https://github.com/microsoft/onnxruntime)
- [Ocrx](https://github.com/septbr/Ocrx)