
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <stdlib.h>
#include <stdio.h>
#include "graphics.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


/* Function definitions */
int init_x(App *app, int w, int h) {
	
	app->d = XOpenDisplay(NULL);
	
	if (!app->d) {
		
		return 1;
	}
	
	//primary display ID
	app->screen = DefaultScreen(app->d);

	// Get the actual hardware window resolution
	app->width = w;
	app->height = h;

	//setup a width and height for a pixel buffer to manually draw into
	app->pixel_buffer_w = PBUF_WIDTH;
	app->pixel_buffer_h = PBUF_HEIGHT;
	app->pixel_buffer = (uint32_t *) malloc((PBUF_WIDTH * PBUF_HEIGHT) * sizeof(uint32_t));

	if (app->pixel_buffer == NULL) {

		puts("error allocating pixel buffer");
		return 1;
	}

	// Create the XImage structure at the full screen resolution
	app->ximage = XCreateImage(app->d, DefaultVisual(app->d, app->screen), DefaultDepth(app->d, app->screen), ZPixmap, 0, NULL, app->width, app->height, 32, 0);

	// Allocate the big memory block for the 1080p image
	app->ximage->data = (char *)malloc(app->ximage->bytes_per_line * app->height);

	if (app->ximage->data == NULL) {
		
		puts("Error allocating XImage data");
		return 1;
	}	

	//Create Window
	app->w = XCreateSimpleWindow(app->d, RootWindow(app->d, app->screen), 10, 10, app->width, app->height, 1, BlackPixel(app->d, app->screen), BlackPixel(app->d, app->screen));

	//Setup Close Button
	app->wmDeleteMessage = XInternAtom(app->d, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(app->d, app->w, &app->wmDeleteMessage, 1);

	Atom wm_state = XInternAtom(app->d, "_NET_WM_STATE", False);
	Atom fullscreen = XInternAtom(app->d, "_NET_WM_STATE_FULLSCREEN", False);

	// This tells the Window Manager: "When you show this, make it fullscreen"
	XChangeProperty(app->d, app->w, wm_state, XA_ATOM, 32, PropModeReplace, (unsigned char *)&fullscreen, 1);

	//Graphics Context
	app->gc = XCreateGC(app->d, app->w, 0, NULL);

	//Setup Pixmap (Back Buffer)
	XWindowAttributes wa;
	XGetWindowAttributes(app->d, app->w, &wa);
	
	//store width and height in the App struct
	app->width = wa.width;
	app->height = wa.height;
	
	app->buffer = XCreatePixmap(app->d, app->w, app->width, app->height, wa.depth);

	//listen for events
	XSelectInput(app->d, app->w, ExposureMask | KeyPressMask | KeyReleaseMask |StructureNotifyMask | PointerMotionMask);
	
	//put the window on the screen
	XMapWindow(app->d, app->w);

	return 0;
}

void clear_screen(App *app, unsigned long colour) {
	
	//if the pixel buffer has been set, clear it when this function is called (every frame)
	if (app->pixel_buffer != NULL) {
		
		memset(app->pixel_buffer, 0, app->pixel_buffer_w * app->pixel_buffer_h * sizeof(uint32_t));
	}

	XSetForeground(app->d, app->gc, colour);
	XFillRectangle(app->d, app->buffer, app->gc, 0, 0, app->width, app->height);
}

void flip_buffer(App *app) {

	//Copy the Pixmap to the Window (The actual "Flip")
	XCopyArea(app->d, app->buffer, app->w, app->gc, 0, 0, app->width, app->height, 0, 0);
	XFlush(app->d);
}

void close_x(App *app) {
	
	//Free the pixel buffer
	if (app->pixel_buffer) {
	
		free(app->pixel_buffer);
	}
	
	//Free the XImage
	if (app->ximage) {
	
		XDestroyImage(app->ximage);
	}


	//Free the back buffer
	if (app->buffer) {
	
		XFreePixmap(app->d, app->buffer);
	}

	//Free the Graphics Context
	if (app->gc) {
	
		XFreeGC(app->d, app->gc);
	}

	//Destroy the window
	if (app->w) {
		
		XDestroyWindow(app->d, app->w);
	}

	//Close the display connection
	if (app->d) {
		
		XCloseDisplay(app->d);
	}
}

void draw_line_b(App *app, int x1, int y1, int x2, int y2, uint32_t colour){

	//distance in pixels x/y are away from each other
	int dx = abs(x1 - x2);
	int dy = abs(y1 - y2);
	
	//direction x/y are from each other
	int sx = (x1 < x2) ? 1 : -1;
	int sy = (y1 < y2) ? 1 : -1;

	int error = dx - dy;

	while(1) {
	
		//bounds check
		if (x1 >= 0 && x1 < app->pixel_buffer_w && y1 >= 0 && y1 < app->pixel_buffer_h) {
			
			//plot pixel
			app->pixel_buffer[y1 * app->pixel_buffer_w + x1] = colour;
		}

		//line finished
		if (x1 == x2 && y1 == y2) {
			
			break;
		}
		
		int temp_error = 2 * error;
		
		//check x
		if (temp_error > -dy) {

			x1 += sx;
			error -= dy;
		}

		//check y
		if (temp_error < dx) {

			y1 += sy;
			error += dx;
		}
	}
}

void draw_line(App *app, int x1, int y1, int x2, int y2, unsigned long colour) {

	XSetForeground(app->d, app->gc, colour);
	// This draws directly to your back-buffer Pixmap
	XDrawLine(app->d, app->buffer, app->gc, x1, y1, x2, y2);
}

void draw_arc(App *app, int x, int y, unsigned int width, unsigned int height, int angle1, int angle2, unsigned long colour) {

	XSetForeground(app->d, app->gc, colour);
	XFillArc(app->d, app->buffer, app->gc, x, y, width, height, angle1, angle2);
}

void toggle_fullscreen(App *app) {
    
	Atom wm_state = XInternAtom(app->d, "_NET_WM_STATE", False);
	Atom fullscreen = XInternAtom(app->d, "_NET_WM_STATE_FULLSCREEN", False);
	XEvent xev = {0};
	xev.type = ClientMessage;
	xev.xclient.window = app->w;
	xev.xclient.message_type = wm_state;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = 2; // Toggle
	xev.xclient.data.l[1] = fullscreen;
	XSendEvent(app->d, DefaultRootWindow(app->d), False, SubstructureNotifyMask | SubstructureRedirectMask, &xev);
}

//open image file an store in a ARGB buffer that can be read by xlib
int load_sprite(Sprite *s, char *filename) {

	//force 4 channels (RGBA) even if the source is RGB
	unsigned char *data = stbi_load(filename, &s->width, &s->height, &s->channels, 4);

	if (data == NULL) {
		
		printf("could not load image: %s\n", filename);
		
		return 1;
	}

	//allocate a pixel buffer to store the sprite data in ARGB which is expected by xlib and the screenbuffer
	int total_pixels = s->width * s->height;
	s->pixels = (uint32_t *) malloc(total_pixels * sizeof(uint32_t));
	
	if (s->pixels == NULL) {
		
		printf("could not allocate memory for sprite: %s\n", filename);
		
		return 1;
	}

	//do the conversion from RGBA from the image data to ARGB and store it in the sprites pixel buffer
	for(int i = 0; i < total_pixels; i++) {
		
		uint8_t r = data[i * 4 + 0];
		uint8_t g = data[i * 4 + 1];
		uint8_t b = data[i * 4 + 2];
		uint8_t a = data[i * 4 + 3];

		// Packing it for your ARGB X11 setup
		s->pixels[i] = (a << 24) | (r << 16) | (g << 8) | b;
	}

	//free the un-needed image data loaded by stb_image
	stbi_image_free(data);

	return 0;
}

//loads a bitmapped font into a sprite and defines each chars width and height
void load_font(Fontmap *fontmap, char *filename, char *f_map, int char_width, int char_height, uint32_t colour) {

	load_sprite(&fontmap->font_buffer, filename);
	fontmap->f_map = f_map;
	fontmap->char_width = char_width;
	fontmap->char_height = char_height;
	fontmap->colour = colour;
}

Entity create_entity(Entity_type type, int x, int y) {
	
	//default entitiy values
	Entity ent = {0};
	ent.type = type;
	ent.tint = 0xFFFFFFFF;
	ent.scale = (Vector3) {1.0f, 1.0f, 1.0f};
	ent.pos = (Vector3) {x, y, 1.0f};

	return ent;
}

uint32_t apply_tint(uint32_t color, uint32_t tint) {

	//Extract Color Channels
	int cr = (color >> 16) & 0xFF;
	int cg = (color >> 8)  & 0xFF;
	int cb =  color        & 0xFF;

	//Extract Tint Channels
	int tr = (tint >> 16) & 0xFF;
	int tg = (tint >> 8)  & 0xFF;
	int tb =  tint        & 0xFF;

	//Multiply and Normalize (Scale 0-65025 back to 0-255)
	int r = (cr * tr) >> 8;
	int g = (cg * tg) >> 8;
	int b = (cb * tb) >> 8;

	//Re-pack (Assuming Alpha is 0xFF)
	return (0xFF << 24) | (r << 16) | (g << 8) | b;
}

void draw_sprite_affine(uint32_t *dest_buff, int w, int h, Render_mode rm, Entity *ent) {
    
	if (ent->type != ENT_SPRITE) return;
	if (ent->tint == 0) return;

	// Basis vectors
	Vector3 i = {1.0f, 0.0f, 0.0f};
	Vector3 j = {0.0f, 1.0f, 0.0f};

	// Scale vector
	i = v3_multi_s(i, ent->scale.x);
	j = v3_multi_s(j, ent->scale.y);

	// Rotate
	i = v3_rotate(i, ent->rot);
	j = v3_rotate(j, ent->rot);

	// Pivot Adjustment: Shift 'pos' from Top-Left to Center
	Vector3 half_w  = v3_multi_s(i, ent->sprite->width * 0.5f);
	Vector3 half_h = v3_multi_s(j, ent->sprite->height * 0.5f);
	Vector3 origin = v3_sub(ent->pos, v3_add(half_w, half_h));

	// Calculate the 4 corners of the destination quadrilateral
	Vector3 c0 = origin;                                           // Top-Left
	Vector3 c1 = v3_add(origin, v3_multi_s(i, (float)ent->sprite->width));   // Top-Right
	Vector3 c2 = v3_add(origin, v3_multi_s(j, (float)ent->sprite->height));  // Bottom-Left
	Vector3 c3 = v3_add(c1, v3_multi_s(j, (float)ent->sprite->height));       // Bottom-Right

	// Find the Bounding Box (Scan Area) on the screen
	float min_x = fminf(fminf(c0.x, c1.x), fminf(c2.x, c3.x));
	float max_x = fmaxf(fmaxf(c0.x, c1.x), fmaxf(c2.x, c3.x));
	float min_y = fminf(fminf(c0.y, c1.y), fminf(c2.y, c3.y));
	float max_y = fmaxf(fmaxf(c0.y, c1.y), fmaxf(c2.y, c3.y));

	// Pre-calculate squared lengths of basis vectors
	float i_mag_sq = (i.x * i.x + i.y * i.y + i.z * i.z);
	float j_mag_sq = (j.x * j.x + j.y * j.y + j.z * j.z);

	// Optimization: Calculate reciprocals once to avoid division in loops
	float inv_i_mag_sq = 1.0f / i_mag_sq;
	float inv_j_mag_sq = 1.0f / j_mag_sq;

	// Step values: how much u and v change per pixel in screen X and Y
	float du_x = i.x * inv_i_mag_sq;
	float dv_x = i.y * inv_j_mag_sq;
	float du_y = j.x * inv_i_mag_sq;
	float dv_y = j.y * inv_j_mag_sq;

	// Starting u,v for the first pixel (top-left of bounding box)
	int start_x = (int)min_x;
	int start_y = (int)min_y;
	Vector3 start_p = {(float)start_x - origin.x, (float)start_y - origin.y, 0.0f};

	float row_u = (start_p.x * i.x + start_p.y * i.y) * inv_i_mag_sq;
	float row_v = (start_p.x * j.x + start_p.y * j.y) * inv_j_mag_sq;

	// Loop through every pixel in the bounding box
	for (int y = start_y; y < (int)max_y; y++) {
	
		float u = row_u;
		float v = row_v;

		for (int x = start_x; x < (int)max_x; x++) {

			// Check if uv coordinates are within the texture bounds
			if (u >= 0 && u < (float)ent->sprite->width && v >= 0 && v < (float)ent->sprite->height) {

				int tex_x = (int)u;
				int tex_y = (int)v;

				uint32_t color = ent->sprite->pixels[tex_y * ent->sprite->width + tex_x];

				// Alpha check
				if ((color >> 24) & 0xFF) { 

					color = apply_tint(color, ent->tint);
					put_pixel(dest_buff, w, h, rm, x, y, color); 
				}
			}
			
			// Move to next pixel in the row
			u += du_x;
			v += dv_x;
		}

		// Move to next row
		row_u += du_y;
		row_v += dv_y;
	}
}

//draw sprite to screen buffer
void draw_sprite(App *app, Sprite *s, int start_x, int start_y) {

	int offset_x = start_x % s->width;
	if (offset_x < 0) offset_x += s->width;

	for (int y = 0; y < app->pixel_buffer_h; y++) {
	
		uint32_t *dest_row = &app->pixel_buffer[y * app->pixel_buffer_w];
		uint32_t *src_row = &s->pixels[y * s->width];

		for (int x = 0; x < app->pixel_buffer_w; x++) {

			int tex_x = x + offset_x;
			if (tex_x >= s->width) tex_x -= s->width;

			uint32_t color = src_row[tex_x];
			dest_row[x] = color;
		}
	}
}

//this function takes a Fontmap struct and draws a single char from a sprite sheet contained within the font map
void draw_char(App *app, Fontmap *f, int start_x, int start_y, char c) {

	char *cp = strchr(f->f_map, c);	//get pointer to first occurrence of char
	int index = 0;			//default to 0 if not found

	if (cp != NULL) {
	
		index = (int) (cp - f->f_map);
	}

	int col = f->font_buffer.width / f->char_width;	//number of columns in spritesheet
	int row = index / col;				//row the char was found on
	int px = (index % col) * f->char_width;		//X pixel position of the char in the spritesheet
	int py = row * f->char_height;			//Y pixel position of the char in the spritesheet

	for (int y = 0; y < f->char_height; y++) {
		
		for (int x = 0; x < f->char_width; x++) {
		
			//Where to read from the sheet (Source)
			//We start at the char's corner (px, py) and add our loop offsets
			int src_x = px + x;
			int src_y = py + y;
			uint32_t color = f->font_buffer.pixels[src_y * f->font_buffer.width + src_x];
			
			//if font pixel is white change it to color stored in the fontmap struct
			color = (color == 0xFFFFFFFF) ? f->colour : color;

			//Where to draw on the screen (Destination)
			int screen_x = start_x + x;
			int screen_y = start_y + y;

			//Safety Check (Clipping)
			//Never draw off the screen buffer!
			if (screen_x >= 0 && screen_x < app->pixel_buffer_w && screen_y >= 0 && screen_y < app->pixel_buffer_h) {

				int screen_index = screen_y * app->pixel_buffer_w + screen_x;
				app->pixel_buffer[screen_index] = color;
			}
		}
	}
}

//this function takes a string and draws it to the screen buffer at a X,Y coordinate
void draw_string(App *app, Fontmap *fm, char *str, int x, int y) {

	int len = strlen(str);

	for (int i = 0; i < len; i++) {
		
		int xoff = i * fm->char_width + x;

		draw_char(app, fm, xoff, y, str[i]);
	}
}

//copy the buffer to a Ximage and scale it to the screen size
void update_ximage(App *app) {

	// Calculate how many screen pixels one buffer pixel occupies
	float scale_x = (float)app->width / app->pixel_buffer_w;
	float scale_y = (float)app->height / app->pixel_buffer_h;

	for (int y = 0; y < app->height; y++) {

		// Map current screen row back to the source buffer row
		int src_y = (int)(y / scale_y);
		uint32_t *src_row = &app->pixel_buffer[src_y * app->pixel_buffer_w];

		// Get the destination row in the XImage
		uint32_t *dest_row = (uint32_t *)(app->ximage->data + (y * app->ximage->bytes_per_line));

		for (int x = 0; x < app->width; x++) {
			// Map current screen column back to the source buffer column
			int src_x = (int)(x / scale_x);
	
			// "Sample" the color from the buffer and drop it into the screen
			dest_row[x] = src_row[src_x];
		}
	}
	
	// Upload the XImage (CPU RAM) to the Pixmap (X Server/VRAM) This takes whatever is in ximage->data and puts it in the Pixmap 
	XPutImage(app->d, app->buffer, app->gc, app->ximage, 0, 0, 0, 0, app->width, app->height);	
}

void draw_pixel_buffer(App *app) {
	
	//fill the pixel_buffer with random data
	for (int i = 0; i < (app->pixel_buffer_w * app->pixel_buffer_h); i++) {
	
		// rand() gives a big number; we just want a 32-bit color
		app->pixel_buffer[i] = (uint32_t)rand();
	}
}

static inline void put_pixel(uint32_t *pix_buff, int w, int h, Render_mode rm, int x, int y, uint32_t colour) {

	//only draw pixel if it is within the bounds of the pixelbuffer
	if (x >= 0 && x < w && y >= 0 && y < h) {
		
		if (rm == NORMAL) {
			
			pix_buff[y * PBUF_WIDTH + x] = colour;
		}

		if (rm == ADDITIVE) {

			//pre exisitng pixel were adding to
			uint32_t original = pix_buff[y * w + x];

			//seperate the colours into ints so there room to add to them;
			int or = (original >> 16) & 0xFF;
			int og = (original >> 8)  & 0xFF;
			int ob =  original        & 0xFF;

			int r = (colour >> 16) & 0xFF;
			int g = (colour >> 8)  & 0xFF;
			int b =  colour        & 0xFF;

			uint8_t nr = (or + r > 255) ? 255 : or + r;
			uint8_t ng = (og + g > 255) ? 255 : og + g;
			uint8_t nb = (ob + b > 255) ? 255 : ob + b;
			
			//add colour to pixel buffer
			pix_buff[y * w + x] = (0xFF << 24) | (nr << 16) | (ng << 8) | nb;
		}
	}
}

void draw_filled_circle(App *app, int xc, int yc, int r, uint32_t color) {

	int x = 0, y = r;
	int d = 3 - 2 * r;

	while (y >= x) {
	
		// Just use a simple loop and the safe helper
		for (int i = xc - x; i <= xc + x; i++) {
			
			put_pixel(app->pixel_buffer, app->pixel_buffer_w, app->pixel_buffer_h, NORMAL, i, yc + y, color);
			put_pixel(app->pixel_buffer, app->pixel_buffer_w, app->pixel_buffer_h, NORMAL, i, yc - y, color);
		}
		
		for (int i = xc - y; i <= xc + y; i++) {
		
			put_pixel(app->pixel_buffer, app->pixel_buffer_w, app->pixel_buffer_h, NORMAL, i, yc + x, color);
			put_pixel(app->pixel_buffer, app->pixel_buffer_w, app->pixel_buffer_h, NORMAL, i, yc - x, color);
		}

		if (d < 0) {
			
			d = d + 4 * x + 6;
		
		} else {
			
			d = d + 4 * (x - y) + 10;
			y--;
		}
		
		x++;
	}
}

