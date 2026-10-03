#include <charconv>

#include "mods/service.hpp"
#include "mods/svc/log.h"
#include "mods/svc/flow.hpp"
#include "mods/svc/config.h"
#include "mods/svc/message.h"
#include "mods/svc/resource.h"
#include "mods/svc/ui.h"

#include "util.hpp"
#include "config.hpp"

#include <nlohmann/json.hpp>
#include <fmt/format.h>

#include <cstdint>
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

using json = nlohmann::json;

namespace {
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

std::string areaFolder = "Area/";
std::string areaNpcFolder = "Area_NPC/";
std::string coolFolder = "Cool/";
std::string npcFolder = "NPC/";
std::string fishFolder = "fish/";
std::string itemsFolder = "items/";

constexpr std::array kAllLanguages{
    MESSAGE_LANGUAGE_ENGLISH, // Purlo -> Mr. Cool, no other feature for english language
    MESSAGE_LANGUAGE_GERMAN,
    MESSAGE_LANGUAGE_FRENCH,
    MESSAGE_LANGUAGE_SPANISH,
    MESSAGE_LANGUAGE_ITALIAN,
    // MESSAGE_LANGUAGE_JAPANESE, Not sure If I ever want to update the japanese one
};

// groups from 00 up to 09
uint16_t kGroupCount = 9;

std::vector<mods::flow::MessageOverride> g_overrides;


ModResult init_settings() {
    ModResult result;
    result = addBoolVar("npcNames", true, g_config_var_npc);
    if (result != MOD_OK)
        return result;
    result = addBoolVar("areaNames", true, g_config_var_area);
    if (result != MOD_OK)
        return result;
    result = addBoolVar("fishNames", false, g_config_var_fish);
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

    ModResult result;
    result = addToggle(panel, "Replace NPC names", g_config_var_npc, g_element_handle_npc,
        notDisabled);
    if (result != MOD_OK)
        return result;
    result = addToggle(panel, "Replace area names", g_config_var_area, g_element_handle_area,
        notDisabled);
    if (result != MOD_OK)
        return result;
    result = addToggle(panel, "Replace fish names (NOT YET IMPLEMENTED)", g_config_var_fish,
        g_element_handle_fish, disabled);
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

std::string getLanguageFolder(const MessageLanguage language) {
    std::string languageFolder;
    switch (language) {
    case MESSAGE_LANGUAGE_ENGLISH:
        languageFolder = "english/";
        break;
    case MESSAGE_LANGUAGE_GERMAN:
        languageFolder = "german/";
        break;
    case MESSAGE_LANGUAGE_FRENCH:
        languageFolder = "french/";
        break;
    case MESSAGE_LANGUAGE_SPANISH:
        languageFolder = "spanish/";
        break;
    case MESSAGE_LANGUAGE_ITALIAN:
        languageFolder = "italian/";
        break;
    default:
        // nothing to be done here, even if a language does not exist, file won't be found
        // no segfault or w.e
        break;
    }
    return languageFolder;
}

// maybe in some world where I'm not exhausted I split this function into several parts
// like, read json, compute ID, extract text, then the call to the flow
void overrideMessages(const MessageLanguage language, const std::string& folderOverride) {
    int messageOverriden = 0;
    std::string languageFolder = getLanguageFolder(language);
    for (uint16_t group = 0; group < kGroupCount; ++group) {
        std::string filePath = languageFolder + folderOverride + "zel_0" + std::to_string(group) +
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
                uint16_t id = leftId * 256 + rightId;

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
                g_overrides.push_back(std::move(override));
                messageOverriden++;
            }
        }
    }
    svc_log->debug(mod_ctx, fmt::format("replaced {} messages for {}", messageOverriden,
        languageFolder + folderOverride).c_str());
}

void updateNames() {
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

    for (const MessageLanguage language : kAllLanguages) {
        if (editNpc) {
            overrideMessages(language, npcFolder);
        }
        if (editArea) {
            overrideMessages(language, areaFolder);
        }
        if (editArea && editNpc) {
            overrideMessages(language, areaNpcFolder);
        }
        if (editFish) {
            // todo fish
            // some stuff
        }
        if (editItem) {
            // todo items
            // some stuff
        }
        if (editCool) {
            overrideMessages(language, coolFolder);
        }
    }
}
} // namespace

extern "C" {
/*
 * Known issue, map area names, they will need some hook
 *
 * fishing journal, will need some hook shenanigans, and I'm not so sure yet how to do it
 *  static const u32 name_id[6] = {
 *      0x59E, 0x59D, 0x59B, 0x599, 0x59A, 0x59C,
 *  };
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
    //panel.update = update; no need for this I believe, or maybe?
    result = svc_ui->register_mods_panel(mod_ctx, &ui_mods_panel_desc);
    if (result != MOD_OK)
        return result;

    // overide the languages
    updateNames();

    return MOD_OK;
}

MOD_EXPORT ModResult mod_update(ModError*) {
    // maybe force a reload if settings are updated? not sure how it works right now
    return MOD_OK;
}

MOD_EXPORT ModResult mod_shutdown(ModError*) {
    g_overrides.clear();
    return MOD_OK;
}
}