#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdint.h>
#include "types.h"
#include "vector.h"
#include "ply.h"

//size of pixel buffer
#define PBUF_WIDTH 1920 
#define PBUF_HEIGHT 1080

typedef enum {

	NORMAL,
	ADDITIVE
} Render_mode;

//This struct bundles everything Xlib needs 
typedef struct {
	
	Display *d;
	Window w;
	GC gc;
	Pixmap buffer;		//back buffer for double buffering
	XImage *ximage;		//Xlibs wrapper for the pixel buffer
	uint32_t *pixel_buffer;	//raw pixel buffer to draw to for pixel effects
	int screen;		//which monitor the window will be on
	int width;		//width of window
	int height;		//height of window
	int pixel_buffer_w;	//width of pixel buffer
	int pixel_buffer_h;	//height of pixel buffer
	Atom wmDeleteMessage;
} App;

typedef struct {

	uint32_t *pixels;
	int width;
	int height;
	int channels;
} Sprite;

typedef struct {

	Vector3 *local_verts;
	Vector3 *screen_verts;
	int num_verts;
} Mesh;

typedef struct {
	
	Sprite font_buffer;
	char *f_map; 		//characters as they appear in the font map
	int char_width;
	int char_height;
	uint32_t colour; 
} Fontmap;

typedef enum {

	ENT_SPRITE, 
	ENT_MESH
} Entity_type;

typedef struct {
	
	union {    
		Sprite *sprite; // Pointer to sprite data
		Mesh *mesh;	// Pointer to hold mesh data
	};

	Entity_type type;
	Vector3 pos;	//position
	Vector3 vel;	//velocity
	Vector3 accel;	//acceleration
	Vector3 scale;	//scale
	Vector3 rot;	//rotation
	uint32_t tint;  // The modulation color (Brightness + Color)
} Entity;

//function Prototypes
int init_x(App *app, int w, int h);
void clear_screen(App *app, unsigned long color);
void flip_buffer(App *app);
void close_x(App *app);
void toggle_fullscreen(App *app);
int load_sprite(Sprite *s, char *filename);
Entity create_entity(Entity_type type, int x, int y);
void draw_sprite_affine(uint32_t *dest_buff, int w, int h, Render_mode rm, Entity *ent);
void draw_sprite(App *app, Sprite *s, int start_x, int start_y);
void draw_char(App *app, Fontmap *f, int start_x, int start_y, char c);
void draw_string(App *app, Fontmap *fm, char *str, int x, int y);
void draw_pixel_buffer(App *app);
void draw_line(App *app, int x1, int y1, int x2, int y2, unsigned long colour);
void draw_line_b(App *app, int x1, int y1, int x2, int y2, uint32_t colour);
void draw_arc(App *app, int x, int y, unsigned int width, unsigned int height, int angle1, int angle2, unsigned long colour);
void update_ximage(App *app);
void load_font(Fontmap *fontmap, char *filename, char *f_map, int char_width, int char_height, uint32_t colour);
static inline void put_pixel(uint32_t *pix_buff, int w, int h, Render_mode rm, int x, int y, uint32_t colour);
void draw_filled_circle(App *app, int xc, int yc, int r, uint32_t color);


#endif

