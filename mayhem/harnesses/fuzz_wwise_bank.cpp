// Wwise sound banks (.bnk): pt::audio::Bank::Load (BKHD/HIRC/DIDX/STMG chunk and object parsing).
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "engine/audio/wwise_bank.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    auto storage = std::make_shared<const std::vector<uint8_t>>(data, data + size);
    std::string error;
    pt::audio::Bank bank;
    bank.Load("fuzz.bnk", storage, 0, storage->size(), &error);
    return 0;
}
