/*
 * Settings added by the port (not in MoG.cfg, which keeps the upstream
 * format). Stored as "key=value" lines in port.cfg in the settings directory.
 *   scaler=xbr|sharp|nearest
 */
#ifndef MOG_PORT_SETTINGS_H
#define MOG_PORT_SETTINGS_H

void port_settings_load(void);
void port_settings_save(void);

#endif
