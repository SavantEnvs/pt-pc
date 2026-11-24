// Fox2 datasets (.fox2): pt::fox2::DataSetFile::Load, then every typed getter on every property.
#include <cstddef>
#include <cstdint>
#include <span>

#include "engine/data/fox2.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    pt::fox2::DataSetFile file;
    if (!file.Load("fuzz.fox2", std::span<const uint8_t>(data, size))) return 0;
    for (const auto& entity : file.Entities()) {
        for (const auto& prop : entity.properties) {
            const size_t n = prop.Count() < 4 ? prop.Count() : 4;
            for (size_t i = 0; i < n; ++i) {
                file.GetInt(entity, prop.name, i);
                file.GetUInt(entity, prop.name, i);
                file.GetFloat(entity, prop.name, i);
                file.GetBool(entity, prop.name, i);
                file.GetString(entity, prop.name, i);
                file.GetVec4(entity, prop.name, i);
                file.GetQuat(entity, prop.name, i);
                file.GetEntity(entity, prop.name, i);
                file.GetLink(entity, prop.name, i);
            }
            file.GetEntityMap(entity, prop.name);
        }
    }
    return 0;
}
