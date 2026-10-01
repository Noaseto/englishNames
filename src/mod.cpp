#include <charconv>

#include "mods/service.hpp"
#include "mods/svc/log.h"
#include "mods/svc/flow.hpp"
#include "mods/svc/config.h"
#include "mods/svc/message.h"
#include "mods/svc/resource.h"
#include "mods/svc/ui.h"

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
ConfigVarHandle g_config_var_items = 0;
ConfigVarHandle g_config_var_cool = 0;

UiElementHandle g_element_handle_npc = 0;
UiElementHandle g_element_handle_area = 0;
UiElementHandle g_element_handle_fish = 0;
UiElementHandle g_element_handle_items = 0;
UiElementHandle g_element_handle_cool = 0;

std::string areaFolder = "Area/";
std::string areaNpcFolder = "Area_NPC/";
std::string coolFolder = "Cool/";
std::string npcFolder = "NPC/";
std::string fishFolder = "fish/";
std::string itemsFolder = "items/";

// only french is done for now
constexpr std::array kAllLanguages{
    MESSAGE_LANGUAGE_ENGLISH, // Purlo -> Mr. Cool, no other feature for english language
    // MESSAGE_LANGUAGE_GERMAN,
    MESSAGE_LANGUAGE_FRENCH,
    // MESSAGE_LANGUAGE_SPANISH,
    // MESSAGE_LANGUAGE_ITALIAN,
    // MESSAGE_LANGUAGE_JAPANESE, Not sure If I ever want to update the japanese one
};

// groups from 00 up to 09
uint16_t kGroupCount = 9;

std::vector<mods::flow::MessageOverride> g_overrides;

/*
 * Fully copied from the randomizer
 */
std::string UTF8ToCP1252(const std::string& utf8Str) {
    std::string cp1252Str;
    cp1252Str.reserve(utf8Str.length());

    size_t readPos = 0;
    size_t len = utf8Str.length();

    while (readPos < len) {
        unsigned char c = utf8Str[readPos];

        if (c < 0x80) {
            // Standard ASCII (0x00 - 0x7F)
            cp1252Str.push_back(c);
            ++readPos;
        } else if ((c & 0xE0) == 0xC0 && (readPos + 1 < len)) {
            // 2-byte UTF-8 sequence (0xC2 - 0xDF)
            unsigned char nextByte = utf8Str[readPos + 1];

            // Reconstruct code point for U+0080 to U+07FF
            uint32_t codePoint = ((c & 0x1F) << 6) | (nextByte & 0x3F);

            static std::unordered_map<uint32_t, char> twoByteMap = {
                {0x0152, 0x8C}, // Œ
                {0x0153, 0x9C}, // œ
                {0x0160, 0x8A}, // Š
                {0x0161, 0x9A}, // š
                {0x0178, 0x9F}, // Ÿ
                {0x017D, 0x8E}, // Ž
                {0x017E, 0x9E}, // ž
            };

            if (twoByteMap.contains(codePoint)) {
                cp1252Str.push_back(twoByteMap.at(codePoint));
            } else if (codePoint <= 0xFF) {
                cp1252Str.push_back(static_cast<char>(codePoint));
            } else {
                throw std::runtime_error(fmt::format(
                    "Invalid character U+{:04X} when converting to CP1252 in \"{}\"", codePoint,
                    utf8Str));
            }

            readPos += 2;
        } else if ((c & 0xF0) == 0xE0 && (readPos + 2 < len)) {
            // 3-byte UTF-8 sequence
            unsigned char b2 = utf8Str[readPos + 1];
            unsigned char b3 = utf8Str[readPos + 2];

            uint32_t codePoint = ((c & 0x0F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);

            static std::unordered_map<uint32_t, char> threeByteMap = {
                {0x20AC, 0x80}, // €
                {0x201A, 0x82}, // ‚
                {0x0192, 0x83}, // ƒ
                {0x201E, 0x84}, // „
                {0x2026, 0x85}, // …
                {0x2020, 0x86}, // †
                {0x2021, 0x87}, // ‡
                {0x02C6, 0x88}, // ˆ
                {0x2030, 0x89}, // ‰
                {0x2039, 0x8B}, // ‹
                {0x2018, 0x91}, // ‘
                {0x2019, 0x92}, // ’
                {0x201C, 0x93}, // “
                {0x201D, 0x94}, // ”
                {0x2022, 0x95}, // •
                {0x2013, 0x96}, // –
                {0x2014, 0x97}, // —
                {0x02DC, 0x98}, // ˜
                {0x2122, 0x99}, // ™
                {0x203A, 0x9B}, // ›
            };

            if (threeByteMap.contains(codePoint)) {
                cp1252Str.push_back(threeByteMap.at(codePoint));
            } else {
                throw std::runtime_error(fmt::format(
                    "Invalid character U+{:04X} when converting to CP1252 in \"{}\"", codePoint,
                    utf8Str));
            }
            readPos += 3;
        } else {
            // Unsupported sequence, out of CP1252 range, or malformed UTF-8
            throw std::runtime_error(
                fmt::format("Invalid bytes when converting to CP1252 with \"{}\"", utf8Str));
        }
    }

    return cp1252Str;
}

ModResult init_settings() {
    ConfigVarDesc config_var_desc_npc = CONFIG_VAR_DESC_INIT;
    config_var_desc_npc.name = "npcNames";
    config_var_desc_npc.type = CONFIG_VAR_BOOL;
    config_var_desc_npc.default_bool = true;
    ModResult result = svc_config->register_var(mod_ctx, &config_var_desc_npc, &g_config_var_npc);
    if (result != MOD_OK)
        return result;

    ConfigVarDesc config_var_desc_area = CONFIG_VAR_DESC_INIT;
    config_var_desc_area.name = "areaNames";
    config_var_desc_area.type = CONFIG_VAR_BOOL;
    config_var_desc_area.default_bool = true;
    result = svc_config->register_var(mod_ctx, &config_var_desc_area, &g_config_var_area);
    if (result != MOD_OK)
        return result;

    ConfigVarDesc config_var_desc_fish = CONFIG_VAR_DESC_INIT;
    config_var_desc_fish.name = "fishNames";
    config_var_desc_fish.type = CONFIG_VAR_BOOL;
    config_var_desc_fish.default_bool = true;
    result = svc_config->register_var(mod_ctx, &config_var_desc_fish, &g_config_var_fish);
    if (result != MOD_OK)
        return result;

    ConfigVarDesc config_var_desc_items = CONFIG_VAR_DESC_INIT;
    config_var_desc_items.name = "itemNames";
    config_var_desc_items.type = CONFIG_VAR_BOOL;
    config_var_desc_items.default_bool = true;
    result = svc_config->register_var(mod_ctx, &config_var_desc_items, &g_config_var_items);
    if (result != MOD_OK)
        return result;

    ConfigVarDesc config_var_desc_cool = CONFIG_VAR_DESC_INIT;
    config_var_desc_cool.name = "cool";
    config_var_desc_cool.type = CONFIG_VAR_BOOL;
    config_var_desc_cool.default_bool = true;
    result = svc_config->register_var(mod_ctx, &config_var_desc_cool, &g_config_var_cool);
    if (result != MOD_OK)
        return result;

    return MOD_OK;
}

ModResult build_main_panel(ModContext*, const UiElementHandle panel, void*, ModError*) {
    UiControlDesc ui_control_desc_1 = UI_CONTROL_DESC_INIT;
    ui_control_desc_1.kind = UI_CONTROL_TOGGLE;
    ui_control_desc_1.label = "Replace NPC names";
    //ui_control_desc_1.help_rml = ""; Unseen in this menu
    ui_control_desc_1.binding = UI_BINDING_CONFIG_VAR;
    ui_control_desc_1.config_var = g_config_var_npc;
    svc_ui->pane_add_control(mod_ctx, panel, &ui_control_desc_1, &g_element_handle_npc);

    UiControlDesc ui_control_desc_2 = UI_CONTROL_DESC_INIT;
    ui_control_desc_2.kind = UI_CONTROL_TOGGLE;
    ui_control_desc_2.label = "Replace area names";
    //ui_control_desc_2.help_rml = ""; Unseen in this menu
    ui_control_desc_2.binding = UI_BINDING_CONFIG_VAR;
    ui_control_desc_2.config_var = g_config_var_area;
    svc_ui->pane_add_control(mod_ctx, panel, &ui_control_desc_2, &g_element_handle_area);

    UiControlDesc ui_control_desc_3 = UI_CONTROL_DESC_INIT;
    ui_control_desc_3.is_disabled = [](ModContext*, void*){return true;};
    ui_control_desc_3.kind = UI_CONTROL_TOGGLE;
    ui_control_desc_3.label = "Replace fish names (NOT YET IMPLEMENTED)";
    //ui_control_desc_3.help_rml = ""; Unseen in this menu
    ui_control_desc_3.binding = UI_BINDING_CONFIG_VAR;
    ui_control_desc_3.config_var = g_config_var_area;
    svc_ui->pane_add_control(mod_ctx, panel, &ui_control_desc_3, &g_element_handle_fish);

    UiControlDesc ui_control_desc_5 = UI_CONTROL_DESC_INIT;
    ui_control_desc_5.is_disabled = [](ModContext*, void*){return true;};
    ui_control_desc_5.kind = UI_CONTROL_TOGGLE;
    ui_control_desc_5.label = "Replace item names (NOT YET IMPLEMENTED)";
    //ui_control_desc_5.help_rml = ""; Unseen in this menu
    ui_control_desc_5.binding = UI_BINDING_CONFIG_VAR;
    ui_control_desc_5.config_var = g_config_var_area;
    svc_ui->pane_add_control(mod_ctx, panel, &ui_control_desc_5, &g_element_handle_items);

    UiControlDesc ui_control_desc_4 = UI_CONTROL_DESC_INIT;
    ui_control_desc_4.kind = UI_CONTROL_TOGGLE;
    ui_control_desc_4.label = "This setting is cool (use Purlo's spanish name)";
    //ui_control_desc_4.help_rml = ""; Unseen in this menu
    ui_control_desc_4.binding = UI_BINDING_CONFIG_VAR;
    ui_control_desc_4.config_var = g_config_var_cool;
    svc_ui->pane_add_control(mod_ctx, panel, &ui_control_desc_4, &g_element_handle_cool);

    return MOD_OK;
}

void overrideMessages(const MessageLanguage language, std::string languageFolder, std::string folderOverride) {
    int messageOverriden = 0;
    for (uint16_t group = 0; group < kGroupCount; ++group) {
        std::string filePath = languageFolder + folderOverride + "zel_0" + std::to_string(group)
                               + ".json";
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

                    // here we have to convert stuff between curly brackets
                    // {1a05000000} is the code for player name
                    // they do not have a fixed size :< but its even
                    std::string utf8Line = jsonLine.get<std::string>();
                    std::string line = UTF8ToCP1252(utf8Line);

                    for (size_t i = 0; i < line.size(); i++) {
                        char currentChar = line[i];
                        if (currentChar == '{') {
                            // then we must find the closing one
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
    bool editCool;
    svc_config->get_bool(mod_ctx, g_config_var_npc, &editNpc);
    svc_config->get_bool(mod_ctx, g_config_var_area, &editArea);
    svc_config->get_bool(mod_ctx, g_config_var_cool, &editCool);

    for (const MessageLanguage language : kAllLanguages) {
        std::string languageFolder = "";

        // this switch is meh :c
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

        if (editNpc) {
            overrideMessages(language, languageFolder, npcFolder);
        }
        if (editArea) {
            overrideMessages(language, languageFolder, areaFolder);
        }
        if (editArea && editNpc) {
            overrideMessages(language, languageFolder, areaNpcFolder);
        }
        if (editCool) {
            overrideMessages(language, languageFolder, coolFolder);
        }
        // todo fish and items
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
    svc_ui->register_mods_panel(mod_ctx, &ui_mods_panel_desc);

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