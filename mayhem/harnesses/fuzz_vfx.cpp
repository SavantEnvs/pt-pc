// Effect graphs: pt::vfx::ParseFile (.vfx node graphs) and pt::vfx::LoadLensFlare.
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include "engine/vfx/vfx_file.h"
#include "engine/vfx/vfx_lensflare.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    const std::span<const uint8_t> file(data, size);
    std::string error;
    pt::vfx::File vfx;
    pt::vfx::ParseFile(file, vfx, &error);
    pt::vfx::LensFlareDef flare;
    pt::vfx::LoadLensFlare(file, "fuzz.vfx", flare);
    return 0;
}
