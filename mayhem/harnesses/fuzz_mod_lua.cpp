// Mod scripts: a mod's init.lua loaded into the sandboxed Lua 5.1 host (pt::ModLua::AddMod), then
// every event dispatched once. The host bounds each call with its own instruction budget and
// memory limit (ModLua::kInstructionBudget / kMemoryLimit). A fresh host per input.
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

#include "engine/script/mod_lua.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    pt::ModLua lua([](bool, std::string_view) {});
    lua.SetQueries({[] { return std::string("f040"); }, [] { return 3; }, [] { return 14; }});
    if (!lua.AddMod("fuzz", "fuzz/init.lua", std::span<const uint8_t>(data, size))) return 0;
    lua.FloorEnter("f050", 2);
    lua.StepChange(14, 15);
    lua.Tick(0.016f);
    lua.Message("OpenDoor", "controller");
    return 0;
}
