// Fox packages (.fpk / .fpkd, optionally fox_crypt-encrypted): pt::FoxPackage::Load + Read of every entry.
// The first byte picks the package file name (.fpk or .fpkd), the rest is the package.
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "engine/fs/fpk.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 1) return -1;
    const std::string name = (data[0] & 1) ? "/Assets/fuzz/pack.fpkd" : "/Assets/fuzz/pack.fpk";
    pt::FoxPackage package;
    if (!package.Load(name, std::vector<uint8_t>(data + 1, data + size))) return 0;
    for (const auto& entry : package.Entries()) {
        package.Read(entry);
    }
    return 0;
}
