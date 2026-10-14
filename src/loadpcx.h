#ifndef __LOADPCX_HEADER
#define __LOADPCX_HEADER

#include "MOGtypes.h"

/* Port: reads the 256 colour palette (768 bytes RGB, 8 bit per component)
   at the end of a PCX file. */
bool pcx_read_palette(const char *file, unsigned char *rgb);

/* Port: 2 for 320x200 art stored with doubled pixels, else 1. */
int pcx_source_factor(const char *file);


class PCX_IMAGE {
public:
	PCX_IMAGE() {image=NULL;ancho=0,alto=0;x=0.0F;y=0.0F;dim=0.5F;transp=1.0F;angle=0.0F;state=0;
				 x_inc=y_inc=dim_inc=transp_inc=angle_inc=0.0F;};
	~PCX_IMAGE() {delete image;};

	void set_pos(float nx,float ny) {x=nx;y=ny;};
	void set_dim(float ndim) {dim=ndim;};
	void set_transp(float ntransp) {transp=ntransp;};
	void set_angle(float nangle) {angle=nangle;};
	
	bool done() {return state==0;};

	bool B_loadpcx(char *nombre);
	void set_keypoint(float nx,float ny,float ndim,float ntransp,float nangle,int nframes)
	{
		state=nframes;
		x_inc=(nx-x)/nframes;
		y_inc=(ny-y)/nframes;
		dim_inc=(ndim-dim)/nframes;
		transp_inc=(ntransp-transp)/nframes;
		angle_inc=(nangle-angle)/nframes;
	} /* set_keypoint */ 
	void step() {
		if (state>0) {
			state--;
			x+=x_inc;
			y+=y_inc;
			dim+=dim_inc;
			transp+=transp_inc;
			angle+=angle_inc;
		} else {
			x_inc=y_inc=dim_inc=transp_inc=angle_inc=0.0F;
		} /* if */ 
		} /* step */ 
	void draw(int x,int y,int sx,int sy,BYTE *screen,int screen_x,int screen_y,int pitch);
	void draw_sprite(int x1,int y1,int spx,int spy,int x,int y,int sx,int sy,BYTE *screen,int screen_x,int screen_y,int pitch);

	char *get_image() {return image;};
	char *get_palete() {return pal;};
	int get_ancho() {return ancho;};
	int get_alto() {return alto;};

	float x,y,dim,transp,angle;
private:
	char *image,pal[768];
	int ancho,alto;
	float x_inc,y_inc,dim_inc,transp_inc,angle_inc;
	int state;
};


#endif

