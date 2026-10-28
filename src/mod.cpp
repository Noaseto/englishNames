#include <fmt/format.h>

#include "mods/service.hpp"
#include "mods/svc/log.h"
#include "mods/svc/flow.hpp"
#include "mods/svc/config.h"
#include "mods/svc/message.h"
#include "mods/svc/resource.h"
#include "mods/svc/ui.h"
#include "mods/svc/hooks.hpp"

#include "d/d_meter2_info.h"

#include "mesageFlow.hpp"
#include "config.hpp"

#include <cstdio>

#include "hooks.hpp"

DEFINE_MOD();

IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(UiService, svc_ui);
IMPORT_SERVICE(ConfigService, svc_config);
IMPORT_SERVICE(MessageService, svc_message);
IMPORT_SERVICE(ResourceService, svc_resource);
IMPORT_SERVICE(HookService, svc_hook);

extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError*) {
    englishName::config::initConfigVar();
    englishName::config::initModPanel();

    englishName::hooks::loadTranslations();
    englishName::hooks::initialize();

    englishName::messageFlow::updateFlow();
    return MOD_OK;
}

MOD_EXPORT ModResult mod_update(ModError*) {
    return MOD_OK;
}

MOD_EXPORT ModResult mod_shutdown(ModError*) {
    englishName::messageFlow::clearFlow();

    englishName::hooks::uninstall();

    // there is no panel shutdown
    englishName::config::shutdownConfigVar();
    return MOD_OK;
}
}