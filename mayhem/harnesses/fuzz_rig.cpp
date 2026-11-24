// Skeleton rigs: pt::anim::Rig::Parse, pt::anim::HelpBones::Parse and pt::anim::SimRig::Load
// (sim physics). The first byte selects the parser, the rest is the file.
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include "engine/anim/help_bones.h"
#include "engine/anim/rig.h"
#include "engine/anim/sim_physics.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 1) return -1;
    const std::span<const uint8_t> file(data + 1, size - 1);
    std::string error;
    switch (data[0] % 3) {
    case 0: { pt::anim::Rig rig; rig.Parse(file, &error); break; }
    case 1: { pt::anim::HelpBones bones; bones.Parse(file, &error); break; }
    default: { pt::anim::SimRig sim; sim.Load("fuzz.sim", file, &error); break; }
    }
    return 0;
}
