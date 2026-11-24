// Wwise .wem audio (mod-supplied or embedded in a bank): pt::audio::ParseWem (RIFF chunks, fmt/vorb/cue/
// smpl/LIST), then Media::Create and DecodeAll, which runs the PCM / IMA ADPCM / Wwise Vorbis decoders
// (wwise_vorbis.cpp rebuilding the Vorbis headers from the packed codebooks, then libvorbis synthesis).
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "engine/audio/wem.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    using namespace pt::audio;
    std::string error;
    WemInfo info;
    ParseWem(std::span<const uint8_t>(data, size), info, &error);

    auto storage = std::make_shared<const std::vector<uint8_t>>(data, data + size);
    if (auto media = Media::Create(1, storage, 0, size, &error)) {
        (void)media->CodecName();
        (void)media->DecodeAll();
    }
    return 0;
}
