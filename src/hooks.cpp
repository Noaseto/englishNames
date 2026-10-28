#include "mods/svc/hooks.hpp"

#include <nlohmann/json.hpp>
#include "fmt/format.h"

#include "hooks.hpp"

#include "config.hpp"
#include "util.hpp"
#include "helpers/string.hpp"
#include "mods/svc/log.h"
#include "mods/svc/resource.h"

#include "d/d_meter2_info.h"
#include "mods/svc/config.h"

namespace englishName::hooks {
namespace {
using json = nlohmann::json;
std::unordered_map<uint32_t, std::string> fishHookMap = {
    {0x59E, ""}, {0x59D, ""}, {0x59B, ""}, {0x599, ""}, {0x59A, ""}, {0x59C, ""}
};
// todo areas
// std::unordered_map<uint32_t, std::string> areaHookMap = {};
// 0x589, 0x58A, 0x58B, 0x58C, 0x58D, 0x58E, 0x58F, 0x590, 0x591, 0x592, 0x593, 0x594, 0x595,
// 0x596, 0x597, 0x598, 0x599, 0x59A, 0x59B, 0x59C, 0x59D, 0x59E, 0x59F, 0x5A0, 0x5A1,

ModResult loadFile(const std::string& filePath, std::unordered_map<uint32_t, std::string>& map) {
    ResourceBuffer buffer = RESOURCE_BUFFER_INIT;
    ModResult result = svc_resource->load(mod_ctx, filePath.c_str(), &buffer);
    if (result != MOD_OK) {
        return result;
    }
    const char* start = static_cast<const char*>(buffer.data);
    const char* end = start + buffer.size;
    json elements = json::parse(start, end, nullptr, false);
    svc_resource->free(mod_ctx, &buffer);

    for (const auto& element : elements) {
        std::string fullId = element.at("ID").get<std::string>();
        int leftId;
        int rightId;
        std::sscanf(fullId.c_str(), "%d , %d", &leftId, &rightId);
        // game uses uint32 despite our ID staying on 16 bits
        uint32_t id = util::computeId(leftId, rightId);

        if (!map.contains(id)) {
            svc_log->error(mod_ctx,
                fmt::format("found unexpected {} ID, inside {} file", id, filePath).c_str());
            return MOD_ERROR;
        }

        std::string text;
        for (const auto& jsonLine : element.at("text")) {
            text += util::UTF8ToCP1252(jsonLine.get<std::string>());
        }
        map[id] = std::move(text);
    }
    svc_log->info(mod_ctx, fmt::format("{} has been loaded", filePath).c_str());
    return MOD_OK;
}


DEFINE_HOOK(&dMeter2Info_c::getStringKanji, dMeter2Info_c__GetStringKanji);

/*
 * On the journal fish name are retrieved from the bmg file within the game iso, I do not want
 * that, so I let it do its work, and tell it what the return should actually be
 */
void hookPost_GetStringKanji_fish(ModContext*, void* args, void* retval, void* userdata) {
    bool editFish;
    svc_config->get_bool(mod_ctx, config::configVar_fish, &editFish);
    if (!editFish) {
        return;
    }

    // first we must check that the id fetched is a fish name to replace
    const uint32_t idFetched = mods::arg<uint32_t>(args, 1); // arg 0 is `this`
    if (!fishHookMap.contains(idFetched))
        return;

    const TextSpan o_string = mods::arg<TextSpan>(args, 2);
    if (o_string.buffer == nullptr || o_string.size == 0)
        return;
    std::string text = fishHookMap[idFetched];

    // For fish names, buffer has a length of 0x20
    // our biggest name is "Ordon Catfish" of size 13 (14?)
    std::memcpy(o_string.buffer, text.data(), text.size());
    o_string.buffer[text.size()] = '\0';
}
}

ModResult loadTranslations() {
    ModResult result = loadFile("hooks/fish.json", fishHookMap);
    if (result != MOD_OK) {
        return result;
    }
    // todo, at a later date loadFile("areaHook", &unorderedArea);
    return MOD_OK;
}

ModResult initialize() {
    // todo
    // this hook should be renamed to work for both the fish and area overide(will be done when I
    // understand how to properly override the area message when hovering the map)
    if (mods::hook::add_post<dMeter2Info_c__GetStringKanji>(hookPost_GetStringKanji_fish) !=
        MOD_OK) {
        svc_log->error(mod_ctx, fmt::format("Failed to add post-hook for {}",
            "dMeter2Info_c__GetStringKanji").c_str());
        return MOD_ERROR;
    }

    return MOD_OK;
}

ModResult uninstall() {
    mods::hook::uninstall<dMeter2Info_c__GetStringKanji>(svc_hook);
    return MOD_OK;
}
}