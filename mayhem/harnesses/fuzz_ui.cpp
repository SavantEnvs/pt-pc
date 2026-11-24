// UI files: .uif models, .ffnt fonts, .uigb graphs, .uia animations, .uilb layouts and .lng
// string tables (pt::ui::*::Parse). The first byte selects the parser, the rest is the file.
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include "engine/ui/ffnt.h"
#include "engine/ui/lang_file.h"
#include "engine/ui/uia.h"
#include "engine/ui/uif.h"
#include "engine/ui/uigb.h"
#include "engine/ui/uilb.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 1) return -1;
    const std::span<const uint8_t> file(data + 1, size - 1);
    std::string error;
    switch (data[0] % 6) {
    case 0: { pt::ui::UifModel m; m.Parse(file, &error); break; }
    case 1: { pt::ui::FfntFont f; f.Parse(file, &error); break; }
    case 2: { pt::ui::UigbGraph g; g.Parse(file, &error); break; }
    case 3: { pt::ui::UiaAnimation a; a.Parse(file, &error); break; }
    case 4: { pt::ui::UilbLayout l; l.Parse(file, &error); break; }
    default: { pt::ui::LangFile l; l.Parse(file, &error); break; }
    }
    return 0;
}
