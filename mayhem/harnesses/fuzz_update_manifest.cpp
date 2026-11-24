// Update check: the release JSON fetched over HTTP (pt::update::ParseManifest), the signed update
// manifest (pt::update::VerifyManifest), version comparison and the asset URL allow-list.
// Input layout: <release json or manifest> NUL <signature text> NUL <version>.
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "engine/platform/update_check.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    std::string_view rest(reinterpret_cast<const char*>(data), size);
    std::string_view parts[3];
    for (int i = 0; i < 3; ++i) {
        const size_t nul = i < 2 ? rest.find('\0') : std::string_view::npos;
        parts[i] = rest.substr(0, nul);
        rest = nul == std::string_view::npos ? std::string_view() : rest.substr(nul + 1);
    }
    for (std::string_view platform : {"linux", "windows", "macos"}) {
        if (auto release = pt::update::ParseManifest(parts[0], platform)) {
            pt::update::AllowedAssetUrl(release->url);
            pt::update::CompareVersions(release->version, parts[2]);
        }
    }
    std::string error;
    pt::update::VerifyManifest(parts[0], parts[1], pt::update::ReleasePublicKey(), &error);
    pt::update::AllowedAssetUrl(parts[1]);
    pt::update::AllowedMetadataUrl(parts[1]);
    pt::update::CompareVersions(parts[1], parts[2]);
    return 0;
}
