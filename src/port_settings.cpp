#include "port_settings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "filehandling.h"
#include "platform/platform.h"

extern MOGKey SCREENSHOT_KEY;


void port_settings_load(bool *fullscreen)
{
	FILE *fp=f1open("port.cfg","r",USERDATA);
	char line[128];

	plat_set_scaler("xbr");
	if (fp==NULL) return;
	while(fgets(line,sizeof(line),fp)!=NULL) {
		char *value=strchr(line,'=');
		if (value==NULL) continue;
		*value++=0;
		value[strcspn(value,"\r\n ")]=0;
		if (strcmp(line,"scaler")==0) plat_set_scaler(value);
		if (strcmp(line,"screenshot_key")==0) {
			int key=atoi(value);
			if (key>0 && key<MOGK_LAST) SCREENSHOT_KEY=(MOGKey)key;
		} /* if */ 
		if (strcmp(line,"vsync")==0) plat_set_vsync(atoi(value)!=0);
		if (strcmp(line,"fullscreen")==0 && fullscreen!=NULL) *fullscreen=atoi(value)!=0;
		if (strcmp(line,"window")==0) {
			int x,y,w,h;
			if (sscanf(value,"%d,%d,%d,%d",&x,&y,&w,&h)==4) plat_set_saved_window(x,y,w,h);
		} /* if */ 
	} /* while */ 
	fclose(fp);
} /* port_settings_load */ 


void port_settings_save(void)
{
	FILE *fp=f1open("port.cfg","w",USERDATA);

	if (fp==NULL) return;
	int x,y,w,h;

	fprintf(fp,"scaler=%s\n",plat_scaler_name());
	fprintf(fp,"vsync=%d\n",plat_vsync() ? 1:0);
	fprintf(fp,"screenshot_key=%d\n",(int)SCREENSHOT_KEY);
	fprintf(fp,"fullscreen=%d\n",plat_is_fullscreen() ? 1:0);
	if (plat_get_window(&x,&y,&w,&h)) fprintf(fp,"window=%d,%d,%d,%d\n",x,y,w,h);
	fclose(fp);
} /* port_settings_save */ 
