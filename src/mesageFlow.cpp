#include "mesageFlow.hpp"

#include <charconv>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "config.hpp"
#include "hooks.hpp"
#include "util.hpp"
#include "fmt/format.h"
#include "mods/svc/config.h"
#include "mods/svc/flow.hpp"
#include "mods/svc/log.h"
#include "mods/svc/resource.h"

namespace englishName::messageFlow {
namespace {
using json = nlohmann::json;

constexpr std::array kAllLanguages{
    MESSAGE_LANGUAGE_ENGLISH, // Purlo -> Mr. Cool, no other feature for english language
    MESSAGE_LANGUAGE_GERMAN,
    MESSAGE_LANGUAGE_FRENCH,
    MESSAGE_LANGUAGE_SPANISH,
    MESSAGE_LANGUAGE_ITALIAN,
    // MESSAGE_LANGUAGE_JAPANESE, Not sure If I ever want to update the japanese one
};

struct OverrideCategory {
    std::string aFolder;
    std::vector<mods::flow::MessageOverride> anOverride;
    bool isOverriden;
};

#define OVERRIDE_CATEGORY(name, folder) OverrideCategory name{folder, {}, false}

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

// maybe in some world where I'm not exhausted I split this function into several parts
// like, read json, compute ID, extract text, then the call to the flow
void overrideMessages(const MessageLanguage language, OverrideCategory& overrideCategory) {
    int overridenMessages = 0;
    std::string languageFolder = util::getLanguageFolder(language);
    for (uint16_t group = 0; group < 9; ++group) {
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
                uint16_t id = util::computeId(leftId, rightId);

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
                    std::string line = util::UTF8ToCP1252(utf8Line);

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

                auto messageOverride = mods::flow::override_message(group, id, language,
                    std::span{text});
                overrideCategory.anOverride.push_back(std::move(messageOverride));
                overridenMessages++;
            }
        }
    }
    svc_log->debug(mod_ctx, fmt::format("replaced {} messages for {}", overridenMessages,
        languageFolder + overrideCategory.aFolder).c_str());
}

void loadCategory(OverrideCategory& category) {
    // TODO : eventually try and see if I can only override the current language
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
}

void updateFlow() {
    bool editNpc;
    bool editArea;
    bool editFish;
    bool editItem;
    bool editCool;
    svc_config->get_bool(mod_ctx, config::configVar_npc, &editNpc);
    svc_config->get_bool(mod_ctx, config::configVar_area, &editArea);
    svc_config->get_bool(mod_ctx, config::configVar_fish, &editFish);
    svc_config->get_bool(mod_ctx, config::configVar_item, &editItem);
    svc_config->get_bool(mod_ctx, config::configVar_cool, &editCool);

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

    for (OverrideCategory* category : kAllCategories) {
        clearCategory(*category);
    }

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
}

void clearFlow() {
    for (OverrideCategory* category : kAllCategories) {
        clearCategory(*category);
    }

    hooks::uninstall();
}
}