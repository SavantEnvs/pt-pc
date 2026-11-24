// Mod metadata: mod.json (pt::mods::ParseManifest) and level.json (pt::mods::ParseLevel),
// both through the engine's tolerant JSON parser (src/engine/core/json.cpp).
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "engine/fs/mods.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    const std::string_view text(reinterpret_cast<const char*>(data), size);
    std::string error;
    pt::mods::Manifest manifest;
    pt::mods::ParseManifest(text, manifest, &error);
    std::vector<pt::mods::LevelEdit> edits;
    std::vector<pt::mods::LevelAdd> adds;
    std::vector<std::string> warnings;
    if (pt::mods::ParseLevel(text, "fuzzmod", edits, adds, &warnings, &error)) {
        for (const auto& e : edits) {
            pt::mods::LevelStageMatches(e.stage, "/Assets/pt/level/f000.fpkd");
            pt::mods::LevelNameMatches(e.model, "fuzz_entity");
        }
    }
    return 0;
}
