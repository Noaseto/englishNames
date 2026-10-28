#pragma once
#include "mods/api.h"

namespace englishName::hooks {
/**
 * Read the translation files used by the hooks to store their value
 *
 * @return `MOD_OK` or a specific `ModResult` value if failing
 */
ModResult loadTranslations();

/**
 * Initialize the hooks that this mod uses
 *
 * @return `MOD_OK` or a specific `ModResult` value if failing
 */
ModResult initialize();

/**
 * Uninstall the hooks used by this mod
 *
 * @return `MOD_OK` or a specific `ModResult` value if failing
 */
ModResult uninstall();
}