// Mod models: a mod's .glb/.gltf replacement or added model through pt::LoadGltfModel (GLB container,
// glTF JSON, accessors/bufferViews, skins, embedded images), the loader the 1.0.3 mod support added.
// The first byte picks the asset name (.glb or .gltf), the rest is the model.
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include "engine/assets/fmdl.h"
#include "engine/assets/gltf_model.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 1) return -1;
    const std::string path = (data[0] & 1) ? "/Assets/mods/fuzz/model.gltf" : "/Assets/mods/fuzz/model.glb";
    pt::FmdlModel model;
    std::string error;
    pt::LoadGltfModel(std::span<const uint8_t>(data + 1, size - 1), path, nullptr, model, &error);
    return 0;
}
