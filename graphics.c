
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <stdlib.h>
#include <stdio.h>
#include "graphics.h"
#define STBI_NO_JPEG
#define STBI_NO_GIF
#define STBI_NO_PSD
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_HDR
#define STBI_NO_TGA
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define PLY_IMPLEMENTATION
#include "ply.h"

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
	XSelectInput(app->d, app->w, ExposureMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask | PointerMotionMask);
	
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

void draw_line(App *app, int x1, int y1, int x2, int y2, unsigned long colour) {

	XSetForeground(app->d, app->gc, colour);
	
	// LineSolid = 0, CapButt = 1, JoinMiter = 0
	//XSetLineAttributes(app->d, app->gc, 4, LineSolid, CapButt, JoinMiter);
	XSetLineAttributes(app->d, app->gc, 2, LineSolid, CapRound, JoinRound);

	// This draws directly to your back-buffer Pixmap
	XDrawLine(app->d, app->buffer, app->gc, x1, y1, x2, y2);
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
void load_font(Fontmap *fontmap, char *filename, char *f_map, int char_width, int char_height) {

	load_sprite(&fontmap->font_buffer, filename);
	fontmap->f_map = f_map;
	fontmap->char_width = char_width;
	fontmap->char_height = char_height;
}

//draw sprite to screen buffer
void draw_sprite(App *app, Sprite *s, int start_x, int start_y) {

	for (int y = 0; y < s->height; y++) {

		for (int x = 0; x < s->width; x++) {

			//determine where we are on the screen
			int screen_x = start_x + x;
			int screen_y = start_y + y;

			//safety Check (Clipping) don't draw if we are off-screen!
			if (screen_x >= 0 && screen_x < app->pixel_buffer_w && screen_y >= 0 && screen_y < app->pixel_buffer_h) {

				//Get the color from the sprite
				uint32_t color = s->pixels[y * s->width + x];

				//Alpha Check (Transparency) Only draw if the Alpha byte isn't 0
				if ((color & 0xFF000000) != 0) {
			
					// Calculate index in the big screen buffer
					int screen_index = screen_y * app->pixel_buffer_w + screen_x;
					app->pixel_buffer[screen_index] = color;
				}
			}
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

			//Where to draw on the screen (Destination)
			int screen_x = start_x + x;
			int screen_y = start_y + y;

			//Safety Check (Clipping)
			//Never draw off the screen buffer!
			if (screen_x >= 0 && screen_x < app->pixel_buffer_w && screen_y >= 0 && screen_y < app->pixel_buffer_h) {

				int screen_index = screen_y * app->pixel_buffer_w + screen_x;

				if (color == 0xffffffff) {
				
					app->pixel_buffer[screen_index] = 0xff00ff00;
				}
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

   // 1. Calculate ratios using your struct variables
    // Source: pixel_buffer_w/h (960x540)
    // Destination: width/height (1920x1080)
    double step_x = (double)app->pixel_buffer_w / app->width;
    double step_y = (double)app->pixel_buffer_h / app->height;

    for (int y = 0; y < app->height; y++) {
        // Map window row to buffer row
        int src_y = (int)(y * step_y);
        
        // OFFSET CALCULATION:
        // This is where the 'center' bug lives. We MUST use pixel_buffer_w here.
        // If app->pixel_buffer_w has been changed to 1920, this pointer 
        // will jump way past the start of your data.
        uint32_t *src_row = app->pixel_buffer + (src_y * app->pixel_buffer_w);

        // Map window row to XImage memory
        uint32_t *dest_row = (uint32_t *)(app->ximage->data + (y * app->ximage->bytes_per_line));

        for (int x = 0; x < app->width; x++) {
            int src_x = (int)(x * step_x);
            
            // Copy the pixel
            dest_row[x] = src_row[src_x];
        }
    }

    XPutImage(app->d, app->buffer, app->gc, app->ximage, 0, 0, 0, 0, app->width, app->height);

}


void draw_pixel_buffer(App *app) {
	
	//fill the pixel_buffer with random data
	for (int i = 0; i < (app->pixel_buffer_w * app->pixel_buffer_h); i++) {
	
		// rand() gives a big number; we just want a 32-bit color
		app->pixel_buffer[i] = (uint32_t)rand();
	}
}

static inline void put_pixel(App *app, int x, int y, uint32_t color) {

	//only draw pixel if it is within the bounds of the pixelbuffer
	if (x >= 0 && x < PBUF_WIDTH && y >= 0 && y < PBUF_HEIGHT) {
		
		app->pixel_buffer[y * PBUF_WIDTH + x] = color;
	}
}

void draw_filled_circle(App *app, int xc, int yc, int r, uint32_t color) {
    int x = 0, y = r;
    int d = 3 - 2 * r;

    while (y >= x) {
        // Just use a simple loop and the safe helper
        for (int i = xc - x; i <= xc + x; i++) {
            put_pixel(app, i, yc + y, color);
            put_pixel(app, i, yc - y, color);
        }
        for (int i = xc - y; i <= xc + y; i++) {
            put_pixel(app, i, yc + x, color);
            put_pixel(app, i, yc - x, color);
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


