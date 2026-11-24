// The user's game dump archive (chunk1.psarc): pt::Psarc::Open (header, TOC, block-size table, zlib
// blocks, manifest names) and Read of every named entry. Psarc only opens filesystem paths, so each
// input is written to a fresh file under $TMPDIR and removed afterwards.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

#include "engine/fs/psarc.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    const char* tmp = std::getenv("TMPDIR");
    std::string path = std::string(tmp && *tmp ? tmp : "/tmp") + "/pt_psarc_XXXXXX";
    const int fd = mkstemp(path.data());
    if (fd < 0) return 0;
    size_t done = 0;
    while (done < size) {
        const ssize_t n = write(fd, data + done, size - done);
        if (n <= 0) break;
        done += static_cast<size_t>(n);
    }
    close(fd);
    {
        pt::Psarc archive;
        if (done == size && archive.Open(path)) {
            for (const auto& name : archive.Names()) archive.Read(name);
        }
    }
    unlink(path.c_str());
    return 0;
}
