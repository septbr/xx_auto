#include "xx_auto.hpp"

namespace xx_auto::internal
{
    void init_ocr(const wchar_t *det_path, const wchar_t *cls_path, const wchar_t *rec_path, const wchar_t *characters_path);
    void uninit_ocr();

    XX_AUTO_API int run_entry(app::entry_t entry, bool singleton);
}