// Animation data: cutscene demo streams (pt::anim::DemoStreamFile::Parse + DecodeTrack),
// motion archives (pt::anim::MotionArchive::Parse) and .gani motions (pt::anim::ParseGani).
// The first byte selects the parser, the rest is the file.
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "engine/anim/demo_file.h"
#include "engine/anim/gani.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 1) return -1;
    const uint8_t selector = data[0] % 3;
    std::vector<uint8_t> bytes(data + 1, data + size);
    std::string error;
    if (selector == 0) {
        pt::anim::DemoStreamFile demo;
        if (demo.Parse(std::move(bytes), &error)) {
            const uint32_t last = demo.StreamFrames() < 256 ? demo.StreamFrames() : 256;
            for (size_t t = 0; t < demo.Tracks().size(); ++t) {
                pt::anim::DecodedTrack out;
                demo.DecodeTrack(t, last, out);
            }
        }
    } else if (selector == 1) {
        pt::anim::MotionArchive archive;
        archive.Parse(std::move(bytes), &error);
    } else {
        pt::anim::GaniMotion motion;
        pt::anim::ParseGani(std::span<const uint8_t>(bytes), 0, motion, &error);
    }
    return 0;
}
