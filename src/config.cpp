#include "config.hpp"
#include "mods/svc/config.h"

#include <string>
#include <vector>

#include "mods/api.h"
#include "mods/svc/ui.h"
#include "mesageFlow.hpp"
#include "fmt/format.h"
#include "mods/svc/log.h"

namespace englishName::config {
namespace {
struct RegisteredVar {
    std::string varName;
    ConfigVarHandle* varHandle;
};

std::vector<RegisteredVar> registeredVars;

UiElementHandle elementHandle_npc = 0;
UiElementHandle elementHandle_area = 0;
UiElementHandle elementHandle_fish = 0;
UiElementHandle elementHandle_item = 0;
UiElementHandle elementHandle_cool = 0;

ModResult addBoolVar(const std::string& configName, const bool defaultValue,
    ConfigVarHandle& configVarHandle) {
    ConfigVarDesc desc = CONFIG_VAR_DESC_INIT;
    desc.name = configName.c_str();
    desc.type = CONFIG_VAR_BOOL;
    desc.default_bool = defaultValue;
    ModResult result = svc_config->register_var(mod_ctx, &desc, &configVarHandle);
    if (result == MOD_OK) {
        registeredVars.push_back({configName, &configVarHandle});
        svc_log->trace(mod_ctx, fmt::format("Registered {}", configName).c_str());
    } else {
        svc_log->error(mod_ctx, fmt::format("Failed to register {}", configName).c_str());
    }
    return result;
}

ModResult removeVar(const RegisteredVar& configVarHandle) {
    ModResult result = svc_config->unregister_var(mod_ctx, *configVarHandle.varHandle);
    if (result == MOD_OK) {
        svc_log->trace(mod_ctx, fmt::format("Unregistered {}", configVarHandle.varName).c_str());
    } else {
        svc_log->error(mod_ctx,
            fmt::format("Failed to unregister {}", configVarHandle.varName).c_str());
    }
    return result;
}

ModResult addToggle(const UiElementHandle panel, const std::string& label,
    ConfigVarHandle configVarHandle, UiElementHandle& uiElementHandle, UiPredicateFn isDisabled) {
    UiControlDesc desc = UI_CONTROL_DESC_INIT;
    desc.kind = UI_CONTROL_TOGGLE;
    desc.label = label.c_str();
    desc.binding = UI_BINDING_CONFIG_VAR;
    desc.config_var = configVarHandle;
    desc.is_disabled = isDisabled;
    return svc_ui->pane_add_control(mod_ctx, panel, &desc, &uiElementHandle);
}

ModResult build_main_panel(ModContext*, const UiElementHandle panel, void*, ModError*) {
    UiPredicateFn disabled = [](ModContext*, void*) { return true; };
    UiPredicateFn notDisabled = [](ModContext*, void*) { return false; };

    ModResult result = addToggle(panel, "Replace NPC names", configVar_npc,
        elementHandle_npc, notDisabled);
    if (result != MOD_OK)
        return result;
    result = addToggle(panel, "Replace area names", configVar_area,
        elementHandle_area, notDisabled);
    if (result != MOD_OK)
        return result;
    result = addToggle(panel, "Replace fish names", configVar_fish,
        elementHandle_fish, notDisabled);
    if (result != MOD_OK)
        return result;
    result = addToggle(panel, "Replace item names (NOT YET IMPLEMENTED)", configVar_item,
        elementHandle_item, disabled);
    if (result != MOD_OK)
        return result;
    result = addToggle(panel, "This setting is cool (use Purlo's spanish name)",
        configVar_cool, elementHandle_cool, notDisabled);
    if (result != MOD_OK)
        return result;

    return MOD_OK;
}
}

ModResult initConfigVar() {
    // should not be needed to clear at start of init, better safe than sorry
    registeredVars.clear();

    ModResult result = addBoolVar("npcNames", true, configVar_npc);
    if (result != MOD_OK)
        return result;
    result = addBoolVar("areaNames", true, configVar_area);
    if (result != MOD_OK)
        return result;
    result = addBoolVar("fishNames", true, configVar_fish);
    if (result != MOD_OK)
        return result;
    result = addBoolVar("itemNames", false, configVar_item);
    if (result != MOD_OK)
        return result;
    result = addBoolVar("cool", true, configVar_cool);
    if (result != MOD_OK)
        return result;
    return result;
}

ModResult initModPanel() {
    UiModsPanelDesc ui_mods_panel_desc = UI_MODS_PANEL_DESC_INIT;
    ui_mods_panel_desc.build = build_main_panel;

    // this update is called each frame when in the mod menu
    ui_mods_panel_desc.update = [](ModContext*, void*, ModError*) {
        messageFlow::updateFlow();
        return MOD_OK;
    };

    auto result = svc_ui->register_mods_panel(mod_ctx, &ui_mods_panel_desc);
    if (result != MOD_OK)
        return result;
    return MOD_OK;
}

ModResult shutdownConfigVar() {
    ModResult toReturn = MOD_OK;
    for (const RegisteredVar& registeredVar : registeredVars) {
        ModResult result = removeVar(registeredVar);
        if (toReturn == MOD_OK && result != MOD_OK) {
            toReturn = result;
        }
    }
    registeredVars.clear();
    return toReturn;
}
}
