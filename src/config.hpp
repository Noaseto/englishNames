#pragma once
#include "mods/svc/config.h"

namespace englishName::config {
inline ConfigVarHandle configVar_npc = 0;
inline ConfigVarHandle configVar_area = 0;
inline ConfigVarHandle configVar_fish = 0;
inline ConfigVarHandle configVar_item = 0;
inline ConfigVarHandle configVar_cool = 0;

/**
 * Configure all the variables the mod uses
 *
 * @return `MOD_OK` or a specific `ModResult` value if failing
 */
ModResult initConfigVar();

/**
 * Initializes the panel when selecting the mod in the mods list
 *
 * @return `MOD_OK` or a specific `ModResult` value if failing
 */
ModResult initModPanel();

/**
 * Remove all the variables the mod uses
 *
 * @return `MOD_OK` or a specific `ModResult` value if failing
 */
ModResult shutdownConfigVar();
}