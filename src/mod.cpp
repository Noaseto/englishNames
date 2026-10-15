#include <charconv>

#include "mods/service.hpp"
#include "mods/svc/log.h"
#include "mods/svc/flow.hpp"
#include "mods/svc/config.h"
#include "mods/svc/message.h"
#include "mods/svc/resource.h"
#include "mods/svc/ui.h"
#include "mods/svc/hook.hpp"

#include "helpers/string.hpp"
#include "d/d_meter2_info.h"

#include "util.hpp"
#include "config.hpp"

#include <nlohmann/json.hpp>
#include <fmt/format.h>

#include <cstdio>
#include <span>
#include <string>
#include <utility>
#include <vector>

DEFINE_MOD();

IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(UiService, svc_ui);
IMPORT_SERVICE(ConfigService, svc_config);
IMPORT_SERVICE(MessageService, svc_message);
IMPORT_SERVICE(ResourceService, svc_resource);
IMPORT_SERVICE(HookService, svc_hook);

using json = nlohmann::json;

namespace {
/* HOOK management */

DEFINE_HOOK(&dMeter2Info_c::getStringKanji, dMeter2Info_c__GetStringKanji);

const u32 fish_name_id[6] = {
    0x59E, 0x59D, 0x59B, 0x599, 0x59A, 0x59C,
};
/*
 * On the journal fish name are retrieved from the bmg file within the game iso, I do not want
 * that, so I let it do its work, and tell it what the return should actually be
 */
void hookPost_GetStringKanji_fish(ModContext*, void* args, void* retval, void* userdata) {
    // first we must check that the id fetched is a fish name's
    const u32 idFetched = mods::arg<u32>(args, 1); // arg 0 is `this`

    bool toReplace = false;
    for (const auto id : fish_name_id) {
        if (idFetched == id) {
            toReplace = true;
        }
    }
    if (!toReplace)
        return;

    // Then we can replace the retrieved fish name with ours
    // TODO, do not read the json file each time a fish call is done, instead fill a map at init
    std::string filePath = "hooks/fish.json";

    ResourceBuffer buffer = RESOURCE_BUFFER_INIT;
    svc_resource->load(mod_ctx, filePath.c_str(), &buffer);

    const char* start = static_cast<const char*>(buffer.data);
    const char* end = start + buffer.size;
    json elements = json::parse(start, end, nullptr, false);
    svc_resource->free(mod_ctx, &buffer);

    for (const auto& element : elements) {
        // id of the message to replace
        std::string fullId = element.at("ID").get<std::string>();
        int leftId, rightId;
        std::sscanf(fullId.c_str(), "%d , %d", &leftId, &rightId);
        uint16_t id = computeId(leftId, rightId);

        if (id == idFetched) {
            std::string text;
            for (const auto& jsonLine : element.at("text")) {
                text += UTF8ToCP1252(jsonLine.get<std::string>());
            }

            const TextSpan o_string = mods::arg<TextSpan>(args, 2);
            if (o_string.buffer == nullptr || o_string.size == 0)
                return;

            const size_t n = std::min(text.size(), o_string.size - 1);
            std::memcpy(o_string.buffer, text.data(), n);
            o_string.buffer[n] = '\0';
        }
    }
    svc_log->info(mod_ctx, "coucou je suis un poisson");
}

const u32 map_name_id[25] = {
    0x589, 0x58A, 0x58B, 0x58C, 0x58D, 0x58E, 0x58F, 0x590, 0x591, 0x592, 0x593, 0x594, 0x595,
    0x596, 0x597, 0x598, 0x599, 0x59A, 0x59B, 0x59C, 0x59D, 0x59E, 0x59F, 0x5A0, 0x5A1,
};

// Work in progress, it does not work for all provinces, despite the ID being correct
// need to look more inot it
void hookPost_GetStringKanji_area(ModContext*, void* args, void* retval, void* userdata) {
    // first we must check that the id fetched is a fish name's
    const u32 idFetched = mods::arg<u32>(args, 1); // arg 0 is `this`

    bool toReplace = false;
    for (const auto id : map_name_id) {
        if (idFetched == id) {
            toReplace = true;
        }
    }
    if (!toReplace)
        return;

    // Then we can replace the retrieved fish name with ours
    // TODO, do not read the json file each time a fish call is done, instead fill a map at init
    std::string filePath = "hooks/area.json";

    ResourceBuffer buffer = RESOURCE_BUFFER_INIT;
    svc_resource->load(mod_ctx, filePath.c_str(), &buffer);

    const char* start = static_cast<const char*>(buffer.data);
    const char* end = start + buffer.size;
    json elements = json::parse(start, end, nullptr, false);
    svc_resource->free(mod_ctx, &buffer);

    for (const auto& element : elements) {
        // id of the message to replace
        std::string fullId = element.at("ID").get<std::string>();
        int leftId, rightId;
        std::sscanf(fullId.c_str(), "%d , %d", &leftId, &rightId);
        uint16_t id = computeId(leftId, rightId);

        if (id == idFetched) {
            std::string text;
            for (const auto& jsonLine : element.at("text")) {
                text += UTF8ToCP1252(jsonLine.get<std::string>());
            }

            const TextSpan o_string = mods::arg<TextSpan>(args, 2);
            if (o_string.buffer == nullptr || o_string.size == 0)
                return;

            const size_t n = std::min(text.size(), o_string.size - 1);
            std::memcpy(o_string.buffer, text.data(), n);
            o_string.buffer[n] = '\0';
        }
    }
    svc_log->info(mod_ctx, "coucou je suis une map");
}

ConfigVarHandle g_config_var_npc = 0;
ConfigVarHandle g_config_var_area = 0;
ConfigVarHandle g_config_var_fish = 0;
ConfigVarHandle g_config_var_item = 0;
ConfigVarHandle g_config_var_cool = 0;

UiElementHandle g_element_handle_npc = 0;
UiElementHandle g_element_handle_area = 0;
UiElementHandle g_element_handle_fish = 0;
UiElementHandle g_element_handle_item = 0;
UiElementHandle g_element_handle_cool = 0;

struct OverrideCategory {
    std::string aFolder;
    std::vector<mods::flow::MessageOverride> anOverride;
    bool isOverriden;
};

#define OVERRIDE_CATEGORY(name, folder) OverrideCategory name{folder, {}, false}

// it's not very useful is it :c
OVERRIDE_CATEGORY(overrideArea, "Area/");
OVERRIDE_CATEGORY(overrideNpc, "NPC/");
OVERRIDE_CATEGORY(overrideAreaAndNpc, "Area_NPC/");
OVERRIDE_CATEGORY(overrideCool, "Cool/");
OVERRIDE_CATEGORY(overrideFish, "Fish/");
OVERRIDE_CATEGORY(overrideFishAndArea, "Fish_Area/");
OVERRIDE_CATEGORY(overrideFishAndNpc, "Fish_NPC/");
OVERRIDE_CATEGORY(overrideItems, "items/");

constexpr std::array kAllCategories{
    &overrideArea,
    &overrideNpc,
    &overrideAreaAndNpc,
    &overrideCool,
    &overrideFish,
    &overrideFishAndArea,
    &overrideFishAndNpc,
    &overrideItems,
};

constexpr std::array kAllLanguages{
    MESSAGE_LANGUAGE_ENGLISH, // Purlo -> Mr. Cool, no other feature for english language
    MESSAGE_LANGUAGE_GERMAN,
    MESSAGE_LANGUAGE_FRENCH,
    MESSAGE_LANGUAGE_SPANISH,
    MESSAGE_LANGUAGE_ITALIAN,
    // MESSAGE_LANGUAGE_JAPANESE, Not sure If I ever want to update the japanese one
};

// groups from 00 up to 08
uint16_t kGroupCount = 9;

ModResult init_settings() {
    ModResult result;
    result = addBoolVar("npcNames", true, g_config_var_npc);
    if (result != MOD_OK)
        return result;
    result = addBoolVar("areaNames", true, g_config_var_area);
    if (result != MOD_OK)
        return result;
    result = addBoolVar("fishNames", true, g_config_var_fish);
    if (result != MOD_OK)
        return result;
    result = addBoolVar("itemNames", false, g_config_var_item);
    if (result != MOD_OK)
        return result;
    result = addBoolVar("cool", true, g_config_var_cool);
    if (result != MOD_OK)
        return result;
    return MOD_OK;
}

ModResult build_main_panel(ModContext*, const UiElementHandle panel, void*, ModError*) {
    UiPredicateFn disabled = [](ModContext*, void*) { return true; };
    UiPredicateFn notDisabled = [](ModContext*, void*) { return false; };

    ModResult result = addToggle(panel, "Replace NPC names", g_config_var_npc, g_element_handle_npc,
        notDisabled);
    if (result != MOD_OK)
        return result;
    result = addToggle(panel, "Replace area names", g_config_var_area, g_element_handle_area,
        notDisabled);
    if (result != MOD_OK)
        return result;
    result = addToggle(panel, "Replace fish names", g_config_var_fish,
        g_element_handle_fish, notDisabled);
    if (result != MOD_OK)
        return result;
    result = addToggle(panel, "Replace item names (NOT YET IMPLEMENTED)", g_config_var_item,
        g_element_handle_item, disabled);
    if (result != MOD_OK)
        return result;
    result = addToggle(panel, "This setting is cool (use Purlo's spanish name)", g_config_var_cool,
        g_element_handle_cool, notDisabled);
    if (result != MOD_OK)
        return result;

    return MOD_OK;
}

// maybe in some world where I'm not exhausted I split this function into several parts
// like, read json, compute ID, extract text, then the call to the flow
void overrideMessages(const MessageLanguage language, OverrideCategory& overrideCategory) {
    int messageOverriden = 0;
    std::string languageFolder = getLanguageFolder(language);
    for (uint16_t group = 0; group < kGroupCount; ++group) {
        std::string filePath = languageFolder +
                               overrideCategory.aFolder +
                               "zel_0" +
                               std::to_string(group) +
                               ".json";
        if (svc_resource->file_exists(mod_ctx, filePath.c_str())) {
            ResourceBuffer buffer = RESOURCE_BUFFER_INIT;
            svc_resource->load(mod_ctx, filePath.c_str(), &buffer);
            const char* start = static_cast<const char*>(buffer.data);
            const char* end = start + buffer.size;
            json elements = json::parse(start, end, nullptr, false);
            svc_resource->free(mod_ctx, &buffer);

            for (const auto& element : elements) {
                // id of the message to replace
                std::string fullId = element.at("ID").get<std::string>();
                int leftId, rightId;
                std::sscanf(fullId.c_str(), "%d , %d", &leftId, &rightId);
                uint16_t id = computeId(leftId, rightId);

                // now we need the bytes to write
                std::vector<uint8_t> text;
                auto lines = element.at("text");

                bool isFirstLine = true;
                for (const auto& jsonLine : lines) {
                    if (!isFirstLine) {
                        text.push_back('\n');
                    }
                    isFirstLine = false;

                    std::string utf8Line = jsonLine.get<std::string>();
                    std::string line = UTF8ToCP1252(utf8Line);

                    for (size_t i = 0; i < line.size(); i++) {
                        char currentChar = line[i];
                        if (currentChar == '{') {
                            // here we have to convert stuff between curly brackets
                            // {1a05000000} is the code for player name
                            // they do not have a fixed size :< but its even
                            // and then we must find the closing one
                            // There is no dialog in game with curly brackets, so it is safe to
                            // assume that there will be a closing one for every open one
                            size_t closingCurlyBracket = line.find('}', i);
                            std::string hexTag = line.substr(i + 1,
                                closingCurlyBracket - i - 1);
                            for (size_t j = 0; j < hexTag.size(); j += 2) {
                                uint8_t byte;
                                std::from_chars(hexTag.data() + j, hexTag.data() + (j + 2),
                                    byte, 16);
                                text.push_back(byte);
                            }

                            // move the cursor to the end of the tag
                            i = closingCurlyBracket;
                        } else {
                            text.push_back(currentChar);
                        }
                    }
                }
                text.push_back(0);

                auto override = mods::flow::override_message(group, id, language,
                    std::span{text});
                overrideCategory.anOverride.push_back(std::move(override));
                messageOverriden++;
            }
        }
    }
    svc_log->debug(mod_ctx, fmt::format("replaced {} messages for {}", messageOverriden,
        languageFolder + overrideCategory.aFolder).c_str());
}

void loadCategory(OverrideCategory& category) {
    for (const MessageLanguage language : kAllLanguages)
        overrideMessages(language, category);
    category.isOverriden = true;
}

void clearCategory(OverrideCategory& category) {
    if (!category.isOverriden)
        return;
    category.anOverride.clear();
    category.isOverriden = false;
    svc_log->debug(mod_ctx,
        fmt::format("cleared all overriden messages for {}", category.aFolder).c_str());
}

void initializeMessages() {
    bool editNpc;
    bool editArea;
    bool editFish;
    bool editItem;
    bool editCool;
    svc_config->get_bool(mod_ctx, g_config_var_npc, &editNpc);
    svc_config->get_bool(mod_ctx, g_config_var_area, &editArea);
    svc_config->get_bool(mod_ctx, g_config_var_fish, &editFish);
    svc_config->get_bool(mod_ctx, g_config_var_item, &editItem);
    svc_config->get_bool(mod_ctx, g_config_var_cool, &editCool);

    // if atleast 1 option is updated, clear all, reload all (I'm having logic issues with the
    // area+NPC so this is easier)
    // at first launch, struct "isOverriden"s are all false, so it can be triggered
    bool upToDate = editNpc == overrideNpc.isOverriden &&
                    editArea == overrideArea.isOverriden &&
                    editCool == overrideCool.isOverriden &&
                    editFish == overrideFish.isOverriden;
    if (upToDate) {
        return;
    }
    clearCategory(overrideCool);
    clearCategory(overrideFishAndArea);
    clearCategory(overrideFishAndNpc);
    clearCategory(overrideFish);
    clearCategory(overrideAreaAndNpc);
    clearCategory(overrideArea);
    clearCategory(overrideNpc);

    if (editNpc)
        loadCategory(overrideNpc);
    if (editArea)
        loadCategory(overrideArea);
    if (editNpc && editArea)
        loadCategory(overrideAreaAndNpc);
    if (editFish)
        loadCategory(overrideFish);
    if (editFish && editNpc)
        // No Fish_NPC for german : Coro is already named Coro
        loadCategory(overrideFishAndNpc);
    if (editFish && editArea)
        // No Fish_Area for german and spanish : Ordon is already named Ordon
        loadCategory(overrideFishAndArea);
    if (editCool)
        loadCategory(overrideCool);

    if (editItem) {
        // todo items
        // some stuff
    }

    //Hooks
    // could return a MOD_UNAVAILABLE, but I ignore it
    mods::hook::uninstall<dMeter2Info_c__GetStringKanji>(svc_hook);


    if (editArea) {
        // For the map, something must be hooked
        // in French : "massif pics blancs" 1424 dec, 0x590
        // still not working
        // mods::hook::add_post<dMeter2Info_c__GetStringKanji>(hookPost_GetStringKanji_area);
    }
    if (editFish) {
        // All the language will end up with the english variant, it is just fishes' names
        // The hook is only used to display the fish journal, not the actual fish dialogs
        mods::hook::add_post<dMeter2Info_c__GetStringKanji>(hookPost_GetStringKanji_fish);
    }
}
} // namespace

extern "C" {
/*
 * Known issue, map area names, they will need some hook
 *
 *  items are a big chunk + there would be grammar, for french, spanish I'm ok, italian, I could
 *  german will be messy if I do it on my own
 *
 */
MOD_EXPORT ModResult mod_initialize(ModError*) {
    // init var
    ModResult result = init_settings();
    if (result != MOD_OK)
        return result;

    // init mod panel menu
    UiModsPanelDesc ui_mods_panel_desc = UI_MODS_PANEL_DESC_INIT;
    ui_mods_panel_desc.build = build_main_panel;

    // this update is called each frame in the mod menu
    ui_mods_panel_desc.update = [](ModContext*, void*, ModError*) {
        initializeMessages();
        return MOD_OK;
    };

    result = svc_ui->register_mods_panel(mod_ctx, &ui_mods_panel_desc);
    if (result != MOD_OK)
        return result;

    initializeMessages();
    return MOD_OK;
}

MOD_EXPORT ModResult mod_update(ModError*) {
    return MOD_OK;
}

MOD_EXPORT ModResult mod_shutdown(ModError*) {
    for (OverrideCategory* category : kAllCategories) {
        category->anOverride.clear();
        category->isOverriden = false;
    }
    return MOD_OK;
}
}