// Fox models (.fmdl, replaceable by mods): pt::LoadFmdl (section tables, vertex formats, bone groups,
// materials) and pt::anim::ReadFmdlSkeleton on the same bytes.
#include <cstddef>
#include <cstdint>
#include <span>

#include "engine/anim/skeleton.h"
#include "engine/assets/fmdl.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    const std::span<const uint8_t> file(data, size);
    pt::FmdlModel model;
    pt::LoadFmdl(file, "fuzz.fmdl", model);
    pt::anim::Skeleton skeleton;
    pt::anim::ReadFmdlSkeleton(file, skeleton);
    return 0;
}
