/*
 * Settings added by the port (not in MoG.cfg, which keeps the upstream
 * format). Stored as "key=value" lines in port.cfg in the settings directory.
 *   scaler=xbr|sharp|nearest
 *   vsync=1|0
 *   fullscreen=1|0
 *   window=x,y,width,height
 */
#ifndef MOG_PORT_SETTINGS_H
#define MOG_PORT_SETTINGS_H

/* Call before plat_init(). Sets *fullscreen when port.cfg has it. */
void port_settings_load(bool *fullscreen);
void port_settings_save(void);

#endif
