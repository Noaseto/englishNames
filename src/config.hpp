#pragma once

/*
 * Generic method to add a bool config var
 */
inline ModResult addBoolVar(const std::string& configName, bool defaultValue,
    ConfigVarHandle& configVarHandle) {
    ConfigVarDesc desc = CONFIG_VAR_DESC_INIT;
    desc.name = configName.c_str();
    desc.type = CONFIG_VAR_BOOL;
    desc.default_bool = defaultValue;
    return svc_config->register_var(mod_ctx, &desc, &configVarHandle);
}

/*
 * Generic method to add a ON/OFF button linked to a bool config var
 */
inline ModResult addToggle(const UiElementHandle panel, const std::string& label,
    ConfigVarHandle configVarHandle, UiElementHandle& uiElementHandle, UiPredicateFn isDisabled) {
    UiControlDesc desc = UI_CONTROL_DESC_INIT;
    desc.kind = UI_CONTROL_TOGGLE;
    desc.label = label.c_str();
    desc.binding = UI_BINDING_CONFIG_VAR;
    desc.config_var = configVarHandle;
    desc.is_disabled = isDisabled;
    return svc_ui->pane_add_control(mod_ctx, panel, &desc, &uiElementHandle);
}