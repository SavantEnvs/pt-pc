// Fox sound data that mods and the game dump supply: the .sbp sound bank package (pt::audio::ReadSbp,
// then each "bnk" entry through SoundBankSet::LoadBankBytes -> Wwise bank + Media::Create of every
// embedded wem, and each "sab" table kept the way SoundBankSet::Load keeps it), the .sal sound-asset list
// (ReadSal), cutscene demo-stream audio (ExtractDemoStreamAudio) and the .subp subtitle table
// (SubtitleTable::Parse, given the package's sab tables). Every parser sees the whole input.
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "engine/audio/sound_package.h"
#include "engine/audio/subtitles.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    using namespace pt::audio;
    const std::span<const uint8_t> in(data, size);
    std::string error;

    auto storage = std::make_shared<const std::vector<uint8_t>>(data, data + size);
    std::vector<SbpEntry> entries;
    SoundBankSet banks;
    std::vector<std::vector<uint8_t>> sab_tables;
    if (ReadSbp(*storage, entries, &error)) {
        // ReadSbp guarantees offset + size <= data.size() for every entry it returns.
        for (const auto& entry : entries) {
            if (entry.kind == "bnk") {
                banks.LoadBankBytes("fuzz", storage, entry.offset, entry.size, &error);
            } else if (entry.kind == "sab") {
                sab_tables.emplace_back(storage->begin() + entry.offset, storage->begin() + entry.offset + entry.size);
            }
        }
        for (const auto& [id, media] : banks.AllMedia()) {
            (void)banks.Find(id);
            (void)media->Frames();
        }
    }

    std::vector<SalRecord> records;
    ReadSal(in, records, &error);

    if (auto audio = ExtractDemoStreamAudio(in)) {
        WemInfo info;
        ParseWem(audio->wem, info, &error);
    }

    SubtitleTable subtitles;
    if (subtitles.Parse(in, sab_tables, &error)) {
        for (const auto& entry : subtitles.Entries()) {
            (void)subtitles.FindById(entry.id);
            (void)subtitles.FindByKey(entry.key);
        }
    }
    return 0;
}
