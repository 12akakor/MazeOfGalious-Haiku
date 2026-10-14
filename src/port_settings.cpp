#include "port_settings.h"

#include <stdio.h>
#include <string.h>

#include "filehandling.h"
#include "platform/platform.h"


void port_settings_load(void)
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
	} /* while */ 
	fclose(fp);
} /* port_settings_load */ 


void port_settings_save(void)
{
	FILE *fp=f1open("port.cfg","w",USERDATA);

	if (fp==NULL) return;
	fprintf(fp,"scaler=%s\n",plat_scaler_name());
	fclose(fp);
} /* port_settings_save */ 
