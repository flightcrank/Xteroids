
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "graphics.h"

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

#define Z_OFFSET 500.0f
#define FOCAL_LENGTH 500.0f

#define NUM_ASTEROIDS 39
#define NUM_BULLETS 4

#define SHIP_SPEED_LIMIT 3.5f
#define SHIP_ACCEL 0.035f
#define BULLET_TIME 0.88f

#define TWO_PI 6.28318530717958647692f

void process_events(App *app, Ship *ship, Ship *lives, Asteroid *asteroids, Bullet *bullets, XEvent *ev, int *running);
void project(Model3D *model, float hw, float hh);
void draw_mesh(App *app, Model3D *model);
void setModelDirection(Model3D *model, float amount);
void update_ship(Ship *ship);
void update_asteroids(Asteroid *asteroids);
void update_bullets(Bullet *bullets, double delta_time);
void init_ship(Ship *ship);
void init_asteroids(Asteroid *asteroids);
void init_bullets(Bullet *bullets);
void draw_asteroids(App *app, Asteroid *asteroids, int hw, int hh);
void draw_bullets(App *app, Bullet *bullets, int hw, int hh);
void draw_lives(App *app, Ship *lives, int num_lives, int hw, int hh);
void check_collisions(Ship *ship, Asteroid *asteroids, Bullet *bullets);
float random_float(float min, float max);

typedef enum {
    TITLE_SCREEN, 
    MAIN_GAME,      
    WIN_SCREEN,      
    GAME_OVER      
} GameState;

GameState current_state = TITLE_SCREEN;
static bool keys[65536];

// Helper to get time in seconds
double get_time_seconds() {

	struct timespec ts;
	// CLOCK_MONOTONIC is immune to system clock jumps
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

int main () {

	App app;
	Ship ship;
	Ship lives;
	Model3D title = {0};
	Asteroid asteroids[NUM_ASTEROIDS];
	Bullet bullets[NUM_BULLETS];
	Fontmap fontmap;
	char *f_map = "abcdefghijklmnopqrstuvwxyz      ABCDEFGHIJKLMNOPQRSTUVWXYZ      0123456789.:,;(*!?'/\\\")$%^&-+@~#";
	
	load_ply(&title, "title.ply");
	title.scale_s = 500.0f;

	init_ship(&ship);
	init_ship(&lives);
	init_asteroids(asteroids);
	init_bullets(bullets);
	load_font(&fontmap, "fontmap.png", f_map, 8, 16);
	lives.model.scale_s = 15.0f;

	if (init_x(&app, SCREEN_WIDTH, SCREEN_HEIGHT) != 0) {
		
		puts("init function failed");

		return 1; 
	}

	int running = 1;
	XEvent ev;
	
	float hw = (float) app.width / 2.0f;	//half the window width
	float hh = (float) app.height / 2.0f;	//half the window height
	float angle = 0.0f;
	double last_time = get_time_seconds();
	double target_time = 1.0 / 60.0;
	int x = PBUF_WIDTH / 2;
	int y = PBUF_HEIGHT / 2;
	
	while (running) {

		double frame_start = get_time_seconds();
        	float delta_time = (float)(frame_start - last_time);	
		last_time = frame_start;
		
		//process key and mouse events
		process_events(&app, &ship, &lives, asteroids, bullets, &ev, &running);
		
		//drawing operations
		clear_screen(&app, 0x000000);

		switch (current_state) {

			case TITLE_SCREEN:
				
				//copy pixel_buffer to the xlib pixmap for display
				char *play = "Press Space to Play";
				int len = (strlen(play) * 8) / 2;

				draw_string(&app, &fontmap, play, x - len, PBUF_HEIGHT - 100);
				update_ximage(&app);
				
				update_asteroids(asteroids);
				draw_asteroids(&app, asteroids, hw, hh);
				
				project(&title, hw, hh);
				draw_mesh(&app, &title);
				break;
			
			case MAIN_GAME:
		
				//update ship and asteroids position, velocity etc
				check_collisions(&ship, asteroids, bullets);
				update_ship(&ship);
				update_asteroids(asteroids);
				update_bullets(bullets, delta_time);
				
				draw_string(&app, &fontmap, "Lives", 0, 0);
				update_ximage(&app);
				
				project(&ship.model, hw, hh);
				draw_mesh(&app, &ship.model);
				draw_lives(&app, &lives, ship.lives, hw, hh);
				draw_asteroids(&app, asteroids, hw, hh);
				draw_bullets(&app, bullets, hw, hh);
				break;

			case GAME_OVER:
				
				char *over = "GAME OVER";
				char *replay = "Press SPACE to play again!";
				int leng = (strlen(over) * 8) / 2;
				int len2 = (strlen(replay) * 8) / 2;

				draw_string(&app, &fontmap, over, x - leng, y);
				draw_string(&app, &fontmap, replay, x - len2, PBUF_HEIGHT - 100);
				update_ximage(&app);
				break;

			case WIN_SCREEN:
				
				char *win = "YOU WIN !!!";
				char *replay2 = "Press SPACE to play again!";
				int len3 = (strlen(win) * 8) / 2;
				int len4 = (strlen(replay2) * 8) / 2;

				draw_string(&app, &fontmap, win, x - len3, y);
				draw_string(&app, &fontmap, replay2, x - len4, PBUF_HEIGHT - 100);
				update_ximage(&app);
				break;

			default:
				puts("unknown state");
				return 1;
		}
		
		flip_buffer(&app);

		//Calculate how much time we spent doing work
		double frame_end = get_time_seconds();
		double time_spent_working = frame_end - frame_start;

		//Sleep only if we have time left over
		if (time_spent_working < target_time) {
		
			usleep((unsigned int)((target_time - time_spent_working) * 1000000));
		}
	}	

	//free resources use by program		
	close_x(&app);
	model3D_free(&ship.model);
	
	return 0;
}

int check_win(Asteroid *asteroids) {

	for (int i = 0; i < NUM_ASTEROIDS; i ++) {

		if (asteroids[i].alive == true) {
			
			return 0;
		}
	}

	return 1;
}

void process_events(App *app, Ship *ship, Ship *lives, Asteroid *asteroids, Bullet *bullets, XEvent *ev, int *running) {
	
	while (XPending(app->d)) {
	
		XNextEvent(app->d, ev);
		
		//window manager event
		if (ev->type == ClientMessage) {
			
			if (ev->xclient.data.l[0] == app->wmDeleteMessage) {
			
				*running = 0;
			}
		}

		// resize event
		if (ev->type == ConfigureNotify) {
		
			app->width = ev->xconfigure.width;
			app->height = ev->xconfigure.height;
		}

		// Only do key logic if it's actually a key event
		if (ev->type == KeyPress || ev->type == KeyRelease) {

			KeySym k = XLookupKeysym(&ev->xkey, 0);

			if (k < 65536) {
			
				if (ev->type == KeyPress) {
					
					keys[k] = true;
				} else {

					keys[k] = false; // Forced off on KeyRelease
				}
			}

			// Handle one-shots
			if (ev->type == KeyPress && k == XK_Escape) {

				*running = 0;
			}
			
			if (ev->type == KeyPress && k == XK_f) {
				
				toggle_fullscreen(app);
			}
			
			if (ev->type == KeyPress && k == XK_space) {
			
				if (current_state == TITLE_SCREEN) {

					current_state = MAIN_GAME;

				} else if (current_state == GAME_OVER || current_state == WIN_SCREEN) {
					
					current_state = TITLE_SCREEN;
					init_ship(ship);
					init_ship(lives);
					init_asteroids(asteroids);
					init_bullets(bullets);
					lives->model.scale_s = 15.0f;

				} else if (current_state == MAIN_GAME) {
				
					for (int i = 0; i < NUM_BULLETS; i++) {
						
						if (bullets[i].alive == false) {
							
							Vector3 b_vel = (Vector3) {10.0f, -10.0f, 0.0f};
							Vector3 b_offset = v3_multi_s(ship->model.direction, ship->model.scale_s);

							bullets[i].alive = true;
							//bullets[i].time_alive = 0;
							bullets[i].model.position = v3_add(ship->model.position, b_offset);
							bullets[i].model.position.y = -bullets[i].model.position.y;
							bullets[i].model.velocity = v3_multi(ship->model.direction, b_vel);
							break;
						}
					}
				}
			}
		}
	}
	
	//rotate ship to the left
	if (keys[XK_a] || keys[XK_Left]) {
		
		setModelDirection(&ship->model, .04f);
	}
	
	//rotate ship to the right
	if (keys[XK_d] || keys[XK_Right]) {
		
		setModelDirection(&ship->model, -.04f);
	}

	if (keys[XK_w] || keys[XK_Up]) {

		// direction is a unit vector, so we scale it by our 'thrust' amount
		ship->model.acceleration = v3_multi_s(ship->model.direction, SHIP_ACCEL);

	} else {

		// If no thrust key is held, acceleration is zero
		// ship.velocity stays the same, so it drifts
		ship->model.acceleration = (Vector3){0, 0, 0};
	}
}

int circles_touching(Vector3 p1, float r1, Vector3 p2, float r2) {

	Vector3 d = v3_sub(p1, p2);
	float limit = r1 + r2;
	
	return (d.x * d.x + d.y * d.y) < (limit * limit);
}

void spawn_asteroids(Asteroid *asteroids, Vector3 pos, asteroid_size_t a_size) {

	int count = 0;

	for(int i = 0; i < NUM_ASTEROIDS; i++) {
		
		if (a_size == AST_MEDIUM && asteroids[i].size == a_size && asteroids[i].alive == false) {
			
			asteroids[i].alive = true;
			asteroids[i].model.position = pos;
			count++;
		}
		
		if (a_size == AST_SMALL && asteroids[i].size == a_size && asteroids[i].alive == false) {
			
			asteroids[i].alive = true;
			asteroids[i].model.position = pos;
			count++;
		}

		if (count == 3) {

			break;
		}
	}
}

void check_collisions(Ship *ship, Asteroid *asteroids, Bullet *bullets) {

	int hw = SCREEN_WIDTH / 2;
	int hh = SCREEN_HEIGHT / 2;
	
	for(int i = 0; i < NUM_ASTEROIDS; i++) {

		Vector3 ship_pos = ship->model.position;
		float ship_r = ship->model.scale_s;
		Vector3 ast_pos = asteroids[i].model.position;
		float ast_r = asteroids[i].model.scale_s;

		int r = circles_touching(ship_pos, ship_r, ast_pos , ast_r);

		//ship collision
		if (r && asteroids[i].alive) { 
			
			ship->lives--;
			ship->model.position = (Vector3) {0};
			ship->model.velocity = (Vector3) {0};
			continue;
		}

		for (int j = 0; j < NUM_BULLETS; j++) {
		
			Vector3 b_pos = v3_multi(bullets[j].model.position, (Vector3) {1.0f, -1.0f, 1.0f});
			r = circles_touching(b_pos, 0, ast_pos , ast_r);
			
			//bullet collision
			if (r && asteroids[i].alive && bullets[j].alive) { 
				
				bullets[j] = (Bullet) {0};

				if (asteroids[i].size == AST_LARGE) {
					
					asteroids[i].alive = false;
					spawn_asteroids(asteroids, ast_pos, AST_MEDIUM);
					continue;		
				
				} else if (asteroids[i].size == AST_MEDIUM) {
					
					asteroids[i].alive = false;
					spawn_asteroids(asteroids, ast_pos, AST_SMALL);
					continue;
				
				}else {
					
					asteroids[i].alive = false;					
				}
			}
		}
	}
}

float random_float(float min, float max) {
	
	float scale = (float)rand() / (float)RAND_MAX; /* [0, 1.0] */
 
	return min + scale * (max - min);      /* [min, max] */
}

void update_ship(Ship *ship) {

	if (ship->lives < 0) {
		
		current_state = GAME_OVER;
	}

	ship->model.velocity = v3_add(ship->model.velocity, ship->model.acceleration);
	ship->model.velocity = v3_limit_mag(ship->model.velocity, SHIP_SPEED_LIMIT);
	ship->model.position = v3_add(ship->model.position, ship->model.velocity);

	int hw = SCREEN_WIDTH / 2;
	int hh = SCREEN_HEIGHT / 2;
	
	//right
	if (ship->model.position.x > hw) {

		 ship->model.position = (Vector3) {-hw, ship->model.position.y, ship->model.position.z};
	}

	//top
	if (ship->model.position.y > hh) {
		 
		ship->model.position = (Vector3) {ship->model.position.x, -hh, ship->model.position.z};
	}
	
	//left
	if (ship->model.position.x < -hw) {
	
		ship->model.position = (Vector3) {hw, ship->model.position.y, ship->model.position.z};
	}
	
	//bottom
	if (ship->model.position.y < -hh ) {
		 
		ship->model.position = (Vector3) {ship->model.position.x, hh, ship->model.position.z};
	}
}

void update_asteroids(Asteroid *asteroids) {
	
	int hw = SCREEN_WIDTH / 2;
	int hh = SCREEN_HEIGHT / 2;
	
	if (check_win(asteroids)) {
		
		current_state = WIN_SCREEN;
	}

	for (int i = 0; i < NUM_ASTEROIDS; i++) {
	
		//update position
		asteroids[i].model.position = v3_add(asteroids[i].model.position, asteroids[i].model.velocity);

		float n = (asteroids[i].spin) ? 0.01f : -0.01f;

		//update rotation
		asteroids[i].model.rotation = v3_add(asteroids[i].model.rotation, (Vector3){0.0f, 0.0f, n});

		//right
		if (asteroids[i].model.position.x > hw) {

			 asteroids[i].model.position = (Vector3) {-hw, asteroids[i].model.position.y, asteroids[i].model.position.z};
		}

		//top
		if (asteroids[i].model.position.y > hh) {
			 
			asteroids[i].model.position = (Vector3) {asteroids[i].model.position.x, -hh, asteroids[i].model.position.z};
		}
		
		//left
		if (asteroids[i].model.position.x < -hw) {
		
			asteroids[i].model.position = (Vector3) {hw, asteroids[i].model.position.y, asteroids[i].model.position.z};
		}
		
		//bottom
		if (asteroids[i].model.position.y < -hh ) {
			 
			asteroids[i].model.position = (Vector3) {asteroids[i].model.position.x, hh, asteroids[i].model.position.z};
		}
	}
}

void update_bullets(Bullet *bullets, double delta_time) {

	int hw = SCREEN_WIDTH / 2;
	int hh = SCREEN_HEIGHT / 2;

	for (int i = 0; i < NUM_BULLETS; i++) {
		
		if (bullets[i].alive == true) {
		
			bullets[i].model.position = v3_add(bullets[i].model.position, bullets[i].model.velocity);
			bullets[i].time_alive += delta_time;

			if (bullets[i].time_alive >= BULLET_TIME) {

				bullets[i] = (Bullet) {0};
				continue;
			}

			//right
			if (bullets[i].model.position.x > hw) {

				 bullets[i].model.position = (Vector3) {-hw, bullets[i].model.position.y, bullets[i].model.position.z};
			}

			//top
			if (bullets[i].model.position.y > hh) {
				 
				bullets[i].model.position = (Vector3) {bullets[i].model.position.x, -hh, bullets[i].model.position.z};
			}
			
			//left
			if (bullets[i].model.position.x < -hw) {
			
				bullets[i].model.position = (Vector3) {hw, bullets[i].model.position.y, bullets[i].model.position.z};
			}
			
			//bottom
			if (bullets[i].model.position.y < -hh ) {
				 
				bullets[i].model.position = (Vector3) {bullets[i].model.position.x, hh, bullets[i].model.position.z};
			}
		}
	}
}

void init_ship(Ship *ship) {
	
	*ship = (Ship) {0};
	load_ply(&ship->model, "ship.ply");
	ship->model.scale_s = 30.0f;
	ship->model.direction = (Vector3) {0.0f, 1.0f, 0.0f}; //default forward position
	ship->lives = 3;
}

void init_bullets(Bullet *bullets) {

	for (int i = 0; i < NUM_BULLETS; i++) {
	
		bullets[i] = (Bullet) {0};
	}
}

void init_asteroids(Asteroid *asteroids) {

	for (int i = 0; i < NUM_ASTEROIDS; i++) {
		
		float angle = random_float(0, TWO_PI);
		float speed = 0.3f;
		float vx = cosf(angle) * speed;
		float vy = sinf(angle) * speed;
		int hw = SCREEN_WIDTH / 2;
		int hh = SCREEN_HEIGHT / 2;
		
		asteroids[i].model = (Model3D) {0};			
		load_ply(&asteroids[i].model, "asteroid1.ply");
		asteroids[i].spin = rand() % 2;
		asteroids[i].model.position = (Vector3) {(rand() % SCREEN_WIDTH) - hw, (rand() % SCREEN_HEIGHT) - hh, 0.0f};
		asteroids[i].model.velocity = (Vector3) {vx, vy, 0.0f};
		asteroids[i].model.rotation = (Vector3) {0.0f, 0.0f, 0.01f};

		if (i < 3) {

			asteroids[i].size = AST_LARGE;
			asteroids[i].model.scale_s = 60.0f;
			asteroids[i].alive = true;

		} else if (i > 2 && i < 12) {

			asteroids[i].size = AST_MEDIUM;
			asteroids[i].model.scale_s = 30.0f;
			asteroids[i].alive = false;

		} else {

			asteroids[i].size = AST_SMALL;
			asteroids[i].model.scale_s = 15.0f;
			asteroids[i].alive = false;
		}
	}
}

void setModelDirection(Model3D *model, float amount) {

	model->direction = (Vector3) {0.0f, 1.0f, 0.0f}; //default forward position
	model->rotation.z += amount;
	model->direction = v3_rotate(model->direction, model->rotation);
}

void project(Model3D *model, float hw, float hh) {
	
	for (int i = 0; i < model->local_count; i++) {
		
		Vector3 scaled_model = v3_multi_s(model->local_verts[i], model->scale_s); 
		Vector3 rot_model = v3_rotate(scaled_model, model->rotation);
		Vector3 translation = v3_add(rot_model, model->position);
		
		//push vert to focal length
		translation.z += Z_OFFSET;

		model->screen_verts[i].x = hw + ((translation.x / translation.z) * FOCAL_LENGTH);
		model->screen_verts[i].y = hh - ((translation.y / translation.z) * FOCAL_LENGTH);
	}
}

void draw_mesh(App *app, Model3D *model) {

	int offset = 0;

	//draw each face of the mesh
	for (int i = 0; i < model->facev_count; i++) {
	
		//verts per face
		int n = model->facev[i];

		for (int j = 0; j < n; j++) {

			//Get the first 3 vertex indices of this face
			int i0 = model->meshf[offset];
			int i1 = model->meshf[offset + 1];
			int i2 = model->meshf[offset + 2];

			//Get their 2D Screen coordinates (already projected)
			Vector3 p0 = model->screen_verts[i0];
			Vector3 p1 = model->screen_verts[i1];
			Vector3 p2 = model->screen_verts[i2];

			//Calculate the "Side" (2D Cross Product)
			//This tells us if the points are winding CCW or CW
			float area = (p1.x - p0.x) * (p2.y - p0.y) - (p1.y - p0.y) * (p2.x - p0.x);

			//Only draw the lines if the face is pointing at us
			//If it's facing away, the area sign will flip.
			//(Try < 0 first; if the sphere disappears, change it to > 0)

			//if (area > 0) {
			
				int index_1 = model->meshf[offset + j];
				int index_2 = model->meshf[offset + (j + 1) % n];

				Vector3 v1 = model->screen_verts[index_1];
				Vector3 v2 = model->screen_verts[index_2];

				//draw line
				draw_line(app, v1.x, v1.y, v2.x, v2.y, 0x0000ff00);
			//}
		}

		offset += n;
	}
}

void draw_asteroids(App *app, Asteroid *asteroids, int hw, int hh) {

	//project asteroids to screen space and draw to screen
	for (int i = 0; i < NUM_ASTEROIDS; i++) {
		
		project(&asteroids[i].model, hw, hh);

		if (asteroids[i].alive) {

			draw_mesh(app, &asteroids[i].model);
		}
	}
}

void draw_bullets(App *app, Bullet *bullets, int hw, int hh) {

	//draw to screen
	for (int i = 0; i < NUM_BULLETS; i++) {
		
		if (bullets[i].alive) {
			
			//translate to center screen co ords
			Vector3 trans = (Vector3) {0.0f + hw, 0.0f + hh, 0.0f};
			Vector3 new_pos = v3_add(bullets[i].model.position, trans);
			
			float x = new_pos.x;
			float y = new_pos.y;

			draw_arc(app, (int) x, (int) y, 10, 10, 0, 64 * 360, 0xFFFFFFFF);
		}
	}
}

void draw_lives(App *app, Ship *lives, int num_lives, int hw, int hh) {
	
	float x_offset = 0;

	for(int i =0; i < num_lives; i++) {

		lives->model.position = (Vector3) {0};
		Vector3 trans = {-hw + 95 + x_offset, +hh - 15, 0.0f};
		lives->model.position = v3_add(lives->model.position, trans);

		project(&lives->model, hw, hh);
		draw_mesh(app, &lives->model);
		x_offset += lives->model.scale_s * 2;
	}
}




