#pragma once
#include "mods/svc/message.h"
//#include <nlohmann/json.hpp>

namespace englishName::util {
// todo
// should have a json reader/parser used by both the messageFlow and the hook
// Is it really useful though?
/**
 * merge high byte and low byte into a full id
 *
 * @param leftId high byte
 * @param rightId low byte
 * @return the concatenated id
 */
uint16_t computeId(uint8_t leftId, uint8_t rightId);

/**
 * Retrieve the relative path for the folder of a given language
 *
 * @param language
 * @return language specific folder name, empty string for japanese
 */
std::string getLanguageFolder(MessageLanguage language);

/**
 * Convert an utf8 string into a cp1252 one
 * parameter is left untouched
 *
 * @param utf8String some utf8 encoded string
 * @return a cp1252 encoded string
 */
std::string UTF8ToCP1252(const std::string& utf8String);
}