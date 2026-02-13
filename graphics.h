#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdint.h>
#include "types.h"
#include "vector.h"
#include "ply.h"

//size of pixel buffer
#define PBUF_WIDTH 960
#define PBUF_HEIGHT 540


//function Prototypes
int init_x(App *app, int w, int h);
void clear_screen(App *app, unsigned long color);
void flip_buffer(App *app);
void close_x(App *app);
void draw_line(App *app, int x1, int y1, int x2, int y2, unsigned long colour);
void draw_arc(App *app, int x, int y, unsigned int width, unsigned int height, int angle1, int angle2, unsigned long colour);
void toggle_fullscreen(App *app);
int load_sprite(Sprite *s, char *filename);
void draw_sprite(App *app, Sprite *s, int start_x, int start_y);
void draw_char(App *app, Fontmap *f, int start_x, int start_y, char c);
void draw_string(App *app, Fontmap *fm, char *str, int x, int y);
void draw_pixel_buffer(App *app);
void update_ximage(App *app);
void load_font(Fontmap *fontmap, char *filename, char *f_map, int char_width, int char_height);
void load_model3D(Model3D *model);


#endif

