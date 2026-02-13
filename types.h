#ifndef TYPES_H
#define TYPES_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>

typedef enum {

	AST_SMALL,
	AST_MEDIUM,
	AST_LARGE
} asteroid_size_t;

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

//This struct holds sprite data
typedef struct {

	uint32_t *pixels;
	int width;
	int height;
	int channels;
} Sprite;

//this struct holds bitmap font data
typedef struct {
	
	//characters as they appear in the font map
	char *f_map; 
	int char_width;
	int char_height;
	Sprite font_buffer;
} Fontmap;

//this struct discribes a 3D vector
typedef struct {
	
	float x;
	float y;
	float z;

} Vector3;

//Struct for holding the data related to a instance of a 3D model
typedef struct {
	
	Vector3 *local_verts;	//Array of Vector3 structs to store the X,Y,Z positions of a vertex
	Vector3 *screen_verts;	//array holding the 2D screen space position of the 2d projected models vertices
	Vector3 direction;	//vector that holds the direction vector of the model
	Vector3 rotation;	//vector that holds the direction the model is facing
	Vector3 position;	//vector that holds the position of the model in world space
	Vector3 velocity;	//vector that describes the rate of change model will move in relative to its position
	Vector3 acceleration;	//vector that describes the rate of change of the velocity
	Vector3 scale;		//vector that describes scale of each axis
	int *facev;		//Array to store how many vertices each face has (3, 4, 5 ...  N faces)
	int *meshf;		//Array of all the vertex indices for every face in the mesh
	int local_count;	//int to store how many elements are in the vertex_array
	int facev_count;	//int to store how many elements are in the face array	
	int meshf_count;	//int to store how many elements are in the faces_array	
	float scale_s;		//the scale in pixels at which the object is rendered at
} Model3D;

typedef struct {

	Model3D model;
	int lives;
} Ship;

typedef struct {

	Model3D model;
	asteroid_size_t size;
	int spin;
	bool alive;
} Asteroid;

typedef struct {

	Model3D model;
	bool alive;
	float time_alive;
} Bullet;

#endif
