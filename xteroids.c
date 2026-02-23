
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "graphics.h"
#define MA_NO_GENERATION
#define MA_NO_ENCODING
#define MA_NO_MP3
#define MA_NO_FLAC
#define MA_NO_VORBIS
#define MA_NO_PULSEAUDIO
#define MA_NO_JACK
#define MA_NO_COREAUDIO
#define MA_NO_WASAPI
#define MA_NO_DSOUND
#define MA_NO_WINMM
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

#define Z_OFFSET 500.0f
#define FOCAL_LENGTH 500.0f

#define NUM_ASTEROIDS 39
#define NUM_BULLETS 4
#define NUM_STARS 200

#define SHIP_SPEED_LIMIT 3.5f
#define SHIP_ACCEL 0.035f
#define BULLET_TIME 0.88f

#define TWO_PI 6.28318530717958647692f

typedef enum {
    TITLE_SCREEN, 
    MAIN_GAME,      
    WIN_SCREEN,      
    GAME_OVER      
} GameScreen;

typedef struct {
	GameScreen current_state;	//enum that keeps track of what screen the game should be rendering
	Ship ship;			//struct that hold data for the player's ship
	Engine engine;
	Asteroid asteroids[NUM_ASTEROIDS];
	Bullet bullets[NUM_BULLETS];
	Model3D lives;			//struct that holds mesh data for the players's lives
	Model3D stars;
	Model3D title;
	Fontmap fontmap;
	ma_engine audio;
	ma_sound engine_sound;
	float timer;
	float game_end_time;
	float engine_pitch_t;
	bool keys[65536];
	bool space_pressed;
} GameState;

void process_events(App *app, GameState *gs, XEvent *ev, int *running);
void project_vs(Model3D *model, float hw, float hh);
void project(Model3D *model, float hw, float hh);
void draw_mesh(App *app, Model3D *model);
void setModelDirection(Model3D *model, float amount);
void update_ship(GameState *gs);
void update_asteroids(GameState *gs);
void update_bullets(Bullet *bullets, double delta_time);
void init_ship(Ship *ship);
void init_asteroids(Asteroid *asteroids);
void init_bullets(Bullet *bullets);
void init_stars(Model3D *stars);
void draw_asteroids(App *app, Asteroid *asteroids, int hw, int hh);
void draw_bullets(App *app, Bullet *bullets, int hw, int hh);
void draw_lives(App *app, GameState *gs, int hw, int hh);
void draw_stars(App *app, Model3D *model);
void check_collisions(GameState *game_state);
float random_float(float min, float max);

Vector3 star_verts[NUM_STARS];	
Vector3 star_verts_s[NUM_STARS];	

// Helper to get time in seconds
double get_time_seconds() {

	struct timespec ts;
	// CLOCK_MONOTONIC is immune to system clock jumps
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

void init_game (GameState *gs) {
	
	init_ship(&gs->ship);
	init_asteroids(gs->asteroids);
	init_bullets(gs->bullets);
	init_stars(&gs->stars);

	char *f_map = "abcdefghijklmnopqrstuvwxyz      ABCDEFGHIJKLMNOPQRSTUVWXYZ      0123456789.:,;(*!?'/\\\")$%^&-+@~#";
	load_font(&gs->fontmap, "./assets/fontmap.png", f_map, 8, 16);
	load_ply(&gs->engine.model, "./assets/engine.ply");
	load_ply(&gs->title, "./assets/title.ply");
	load_ply(&gs->lives, "./assets/ship.ply");
	gs->title.scale_s = 500.0f;
	gs->lives.scale_s = 15.0f;
	gs->engine.model.scale.x = gs->ship.model.scale_s;
	gs->engine.max_blast_length = 65.0f;
	gs->engine.current_blast_t;
}

void update_game_state(GameState *gs) {
    
	if (!gs->space_pressed)  {

		return;
	}

	if (gs->current_state == TITLE_SCREEN) {
	
		gs->current_state = MAIN_GAME;
		gs->ship.spawn_time = gs->timer;

	} else if (gs->current_state == GAME_OVER || gs->current_state == WIN_SCREEN) {
		
		//reset game after short delay
		if (gs->timer > gs->game_end_time + 2.0f) {

			gs->current_state = TITLE_SCREEN;
			init_ship(&gs->ship);
			init_asteroids(gs->asteroids);
			init_bullets(gs->bullets);
			gs->engine.model.rotation = (Vector3){0};
		}

	} else if (gs->current_state == MAIN_GAME) {

		// Bullet firing loop lives here now
		for (int i = 0; i < NUM_BULLETS; i++) {

			if (!gs->bullets[i].alive) {
				
				Vector3 b_vel = (Vector3){10.0f, -10.0f, 0.0f};
				Vector3 b_offset = v3_multi_s(gs->ship.model.direction, gs->ship.model.scale_s);

				gs->bullets[i].alive = true;
				gs->bullets[i].model.position = v3_add(gs->ship.model.position, b_offset);
				gs->bullets[i].model.position.y = -gs->bullets[i].model.position.y;
				gs->bullets[i].model.velocity = v3_multi(gs->ship.model.direction, b_vel);
				ma_engine_play_sound(&gs->audio, "./assets/Shoot.wav", NULL);
				break;
			}
		}
	}
	
	gs->space_pressed = false; // Reset the flag so we don't fire every frame
}

void screen_wrapping(Model3D *model) {

	int hw = SCREEN_WIDTH / 2;
	int hh = SCREEN_HEIGHT / 2;

	if (model->position.x >  hw) model->position.x = -hw;
	if (model->position.x < -hw) model->position.x =  hw;
	if (model->position.y >  hh) model->position.y = -hh;
	if (model->position.y < -hh) model->position.y =  hh;
}

int main () {

	App app = {0};
	GameState game_state = {0};
	init_game(&game_state);
	
	ma_result result = ma_engine_init(NULL, &game_state.audio);
	
	if (result != MA_SUCCESS) {
		
		printf("Failed to initialize audio engine.\n");
		return -1;
	}

	ma_sound_init_from_file(&game_state.audio, "./assets/engine.wav", 0, NULL, NULL, &game_state.engine_sound);
	ma_sound_set_looping(&game_state.engine_sound, MA_TRUE);

	if (init_x(&app, SCREEN_WIDTH, SCREEN_HEIGHT) != 0) {
		
		puts("init_x function failed");

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
		game_state.timer += delta_time;
		last_time = frame_start;
		
		//process key and mouse events
		process_events(&app, &game_state, &ev, &running);
		
		//drawing operations
		clear_screen(&app, 0x000000);

		switch (game_state.current_state) {

			case TITLE_SCREEN: {
				
				//copy pixel_buffer to the xlib pixmap for display
				char *play = "Press SPACE to Play";
				int len = (strlen(play) * 8) / 2;

				update_game_state(&game_state);

				draw_string(&app, &game_state.fontmap, play, x - len, PBUF_HEIGHT - 100);
				update_ximage(&app);
				
				project(&game_state.stars, hw, hh);
				draw_stars(&app, &game_state.stars);
				
				update_asteroids(&game_state);
				draw_asteroids(&app, game_state.asteroids, hw, hh);
				
				project(&game_state.title, hw, hh);
				draw_mesh(&app, &game_state.title);
				break;
			}
			
			case MAIN_GAME: {
				
				bool should_draw = true;
				bool invincible = false;

				float elapsed = game_state.timer - game_state.ship.spawn_time;

				if (elapsed < 3.0f) {
					
					invincible = true;
					should_draw = (fmodf(elapsed, 0.2f) > 0.1f);
				}

				if (!invincible) {
					
					//update ship and asteroids position, velocity etc
					check_collisions(&game_state);
				}

				update_game_state(&game_state);
				update_ship(&game_state);
				update_asteroids(&game_state);
				update_bullets(game_state.bullets, delta_time);
				
				draw_string(&app, &game_state.fontmap, "Lives", 0, 0);
				update_ximage(&app);
				
				project(&game_state.stars, hw, hh);
				draw_stars(&app, &game_state.stars);

				if (should_draw) {
					
					project(&game_state.ship.model, hw, hh);
					draw_mesh(&app, &game_state.ship.model);
				}

				if (game_state.engine.current_blast_t > 0.0f) {
					
					project_vs(&game_state.engine.model, hw, hh);
					draw_mesh(&app, &game_state.engine.model);
				}
				
				draw_lives(&app, &game_state, hw, hh);
				draw_asteroids(&app, game_state.asteroids, hw, hh);
				draw_bullets(&app, game_state.bullets, hw, hh);
				break;
			}

			case GAME_OVER: {
				
				char *over = "GAME OVER";
				char *replay = "Press SPACE to play again!";
				int leng = (strlen(over) * 8) / 2;
				int len2 = (strlen(replay) * 8) / 2;

				update_game_state(&game_state);
				update_asteroids(&game_state);
				
				draw_string(&app, &game_state.fontmap, over, x - leng, y);
				draw_string(&app, &game_state.fontmap, replay, x - len2, PBUF_HEIGHT - 100);
				update_ximage(&app);
				
				project(&game_state.stars, hw, hh);
				draw_stars(&app, &game_state.stars);
				
				draw_asteroids(&app, game_state.asteroids, hw, hh);
				
				break;
			}

			case WIN_SCREEN: {
				
				char *win = "YOU WIN !!!";
				char *replay2 = "Press SPACE to play again!";
				int len3 = (strlen(win) * 8) / 2;
				int len4 = (strlen(replay2) * 8) / 2;
				
				update_game_state(&game_state);
				update_ship(&game_state);

				draw_string(&app, &game_state.fontmap, win, x - len3, y);
				draw_string(&app, &game_state.fontmap, replay2, x - len4, PBUF_HEIGHT - 100);
				update_ximage(&app);
				
				project(&game_state.stars, hw, hh);
				draw_stars(&app, &game_state.stars);
				
				if (game_state.engine.current_blast_t > 0.0f) {
					
					project_vs(&game_state.engine.model, hw, hh);
					draw_mesh(&app, &game_state.engine.model);
				}
				
				project(&game_state.ship.model, hw, hh);
				draw_mesh(&app, &game_state.ship.model);
				break;
			}

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
	model3D_free(&game_state.ship.model);
	
	ma_engine_uninit(&game_state.audio);
	
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

void key_press(GameState *gs) {
}

void process_events(App *app, GameState *gs, XEvent *ev, int *running) {

	while (XPending(app->d)) {

		XNextEvent(app->d, ev);

		if (ev->type == ClientMessage) {
			
			if (ev->xclient.data.l[0] == app->wmDeleteMessage) {

				*running = 0;
				continue;
			}
		}

		if (ev->type == ConfigureNotify) {

			app->width = ev->xconfigure.width;
			app->height = ev->xconfigure.height;
			continue;
		}

		if (ev->type == KeyRelease) {

			KeySym k = XLookupKeysym(&ev->xkey, 0);
			
			if (k >= 65536) {

				continue;
			}

			if (XPending(app->d)) {

				XEvent nev;
				XPeekEvent(app->d, &nev);

				if (nev.type == KeyPress && nev.xkey.time == ev->xkey.time && nev.xkey.keycode == ev->xkey.keycode) {

					XNextEvent(app->d, &nev);
					continue;
				}
			}

			gs->keys[k] = false;
			continue;
		}

		if (ev->type == KeyPress) {

			KeySym k = XLookupKeysym(&ev->xkey, 0);
			
			if (k >= 65536) {

				continue;
			}

			if (gs->keys[k] == false) {
				
				if (k == XK_space) {

					gs->space_pressed = true;
				}

				if (k == XK_f) {
					
					toggle_fullscreen(app);
				}
				
				if (k == XK_Escape) {
					
					*running = 0;
				}
			}

			gs->keys[k] = true;
		}
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

void check_collisions(GameState *gs) {

	int hw = SCREEN_WIDTH / 2;
	int hh = SCREEN_HEIGHT / 2;
	
	for(int i = 0; i < NUM_ASTEROIDS; i++) {

		Vector3 ship_pos = gs->ship.model.position;
		float ship_r = gs->ship.model.scale_s;
		Vector3 ast_pos = gs->asteroids[i].model.position;
		float ast_r = gs->asteroids[i].model.scale_s;

		int r = circles_touching(ship_pos, ship_r, ast_pos , ast_r);

		//ship collision
		if (r && gs->asteroids[i].alive) { 
			
			gs->ship.lives--;
			gs->ship.model.position = (Vector3) {0};
			gs->ship.model.velocity = (Vector3) {0};
			gs->ship.spawn_time = gs->timer;
			ma_engine_play_sound(&gs->audio, "./assets/Boom39.wav", NULL);
			continue;
		}

		for (int j = 0; j < NUM_BULLETS; j++) {
		
			Vector3 b_pos = v3_multi(gs->bullets[j].model.position, (Vector3) {1.0f, -1.0f, 1.0f});
			r = circles_touching(b_pos, 0, ast_pos , ast_r);
			
			//bullet collision
			if (r && gs->asteroids[i].alive && gs->bullets[j].alive) { 
				
				gs->bullets[j] = (Bullet) {0};
				ma_engine_play_sound(&gs->audio, "./assets/Boom3.wav", NULL);

				if (gs->asteroids[i].size == AST_LARGE) {
					
					gs->asteroids[i].alive = false;
					spawn_asteroids(gs->asteroids, ast_pos, AST_MEDIUM);
					continue;		
				
				} else if (gs->asteroids[i].size == AST_MEDIUM) {
					
					gs->asteroids[i].alive = false;
					spawn_asteroids(gs->asteroids, ast_pos, AST_SMALL);
					continue;
				
				}else {
					
					gs->asteroids[i].alive = false;					
				}
			}
		}
	}
}

float random_float(float min, float max) {
	
	float scale = (float)rand() / (float)RAND_MAX; /* [0, 1.0] */
 
	return min + scale * (max - min);      /* [min, max] */
}

void update_ship(GameState *gs) {

	//STATE MANAGEMENT
	if (gs->current_state == MAIN_GAME && gs->ship.lives < 0) {
	
		gs->current_state = GAME_OVER;
		gs->game_end_time = gs->timer;
	}

	//INPUT PROCESSING
	// Rotation
	if (gs->keys[XK_a] || gs->keys[XK_Left]) {
		
		setModelDirection(&gs->ship.model, .04f);
		setModelDirection(&gs->engine.model, .04f);
	}
	if (gs->keys[XK_d] || gs->keys[XK_Right]) {
	
		setModelDirection(&gs->ship.model, -.04f);
		setModelDirection(&gs->engine.model, -.04f);
	}

	// Thrust & Engine Audio
	bool is_pressing_thrust = gs->keys[XK_w] || gs->keys[XK_Up];
	bool can_thrust = (gs->current_state == MAIN_GAME || gs->current_state == WIN_SCREEN);

	if (is_pressing_thrust && can_thrust) {
	
		gs->ship.model.acceleration = v3_multi_s(gs->ship.model.direction, SHIP_ACCEL);

		if (gs->engine_pitch_t < 1.0f) {
		
			gs->engine_pitch_t += 0.002f;
		}

		if (gs->engine.current_blast_t < 1.0f) {

			gs->engine.current_blast_t += 0.01f;
		}

		if (!ma_sound_is_playing(&gs->engine_sound)) {

			ma_sound_start(&gs->engine_sound);
		}

		ma_sound_set_pitch(&gs->engine_sound, gs->engine_pitch_t);

	} else {

		gs->ship.model.acceleration = (Vector3){0, 0, 0};
		ma_sound_stop(&gs->engine_sound);
		gs->engine_pitch_t = 0.0f;

		if (gs->engine.current_blast_t > 0.0f) {
			
			gs->engine.current_blast_t -= 0.05f;
		}
	}

	//PHYSICS INTEGRATION
	gs->ship.model.velocity = v3_add(gs->ship.model.velocity, gs->ship.model.acceleration);
	gs->ship.model.velocity = v3_limit_mag(gs->ship.model.velocity, SHIP_SPEED_LIMIT);
	gs->ship.model.position = v3_add(gs->ship.model.position, gs->ship.model.velocity);

	// Keep engine attached to ship
	gs->engine.model.position = gs->ship.model.position;

	// SCREEN WRAPPING
	screen_wrapping(&gs->ship.model);

	//VISUAL EFFECTS (Flicker)
	float flicker = 1.0f + (sinf(gs->timer * 50.0f) * 0.05f); 
	gs->engine.model.scale.y = (gs->ship.model.scale_s + (gs->engine.current_blast_t * gs->engine.max_blast_length)) * flicker;

}

void update_asteroids(GameState *gs) {

	if (check_win(gs->asteroids)) {
		
		gs->current_state = WIN_SCREEN;
		gs->game_end_time = gs->timer;
	}

	for (int i = 0; i < NUM_ASTEROIDS; i++) {
	
		//update position
		gs->asteroids[i].model.position = v3_add(gs->asteroids[i].model.position, gs->asteroids[i].model.velocity);

		float n = (gs->asteroids[i].spin) ? 0.01f : -0.01f;

		//update rotation
		gs->asteroids[i].model.rotation = v3_add(gs->asteroids[i].model.rotation, (Vector3){0.0f, 0.0f, n});
		
		screen_wrapping(&gs->asteroids[i].model);
	}
}

void update_bullets(Bullet *bullets, double delta_time) {

	for (int i = 0; i < NUM_BULLETS; i++) {
		
		if (bullets[i].alive == true) {
		
			bullets[i].model.position = v3_add(bullets[i].model.position, bullets[i].model.velocity);
			bullets[i].time_alive += delta_time;

			if (bullets[i].time_alive >= BULLET_TIME) {

				bullets[i] = (Bullet) {0};
				continue;
			}

			screen_wrapping(&bullets[i].model);
		}
	}
}

void init_ship(Ship *ship) {
	
	*ship = (Ship) {0};
	load_ply(&ship->model, "./assets/ship.ply");
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
		load_ply(&asteroids[i].model, "./assets/asteroid1.ply");
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

void init_stars(Model3D *stars) {
	
	int hw = SCREEN_WIDTH / 2;
	int hh = SCREEN_HEIGHT / 2;

	stars->local_verts = &star_verts[0];
	stars->screen_verts = &star_verts_s[0];
	stars->scale_s = 1.0f;
	stars->local_count = NUM_STARS;

	for (int i = 0; i < NUM_STARS; i++) {

		stars->local_verts[i].x = (rand() % SCREEN_WIDTH) - hw;
		stars->local_verts[i].y = (rand() % SCREEN_HEIGHT) - hh;
		stars->local_verts[i].z = rand() % 100;
	}
}

void setModelDirection(Model3D *model, float amount) {

	model->direction = (Vector3) {0.0f, 1.0f, 0.0f}; //default forward position
	model->rotation.z += amount;
	model->direction = v3_rotate(model->direction, model->rotation);
}

void project_vs(Model3D *model, float hw, float hh) {

	float NOZZLE_OFFSET = -25.0f; 

	for (int i = 0; i < model->local_count; i++) {

		Vector3 scaled_model = v3_multi(model->local_verts[i], model->scale);


		scaled_model.y += NOZZLE_OFFSET;

		Vector3 rot_model = v3_rotate(scaled_model, model->rotation);
		Vector3 translation = v3_add(rot_model, model->position);
		
		//push vert to focal length
		translation.z += Z_OFFSET;

		model->screen_verts[i].x = hw + ((translation.x / translation.z) * FOCAL_LENGTH);
		model->screen_verts[i].y = hh - ((translation.y / translation.z) * FOCAL_LENGTH);
	}
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

void draw_stars(App *app, Model3D *model) {
	
	float max_size = 5;
	float min_size = 1;
	float minZ = 500;
	float maxZ = 600;

	for(int i = 0; i < NUM_STARS; i++) {
		
		float z = FOCAL_LENGTH + model->local_verts[i].z;

		float size = max_size - ((z - minZ) / (maxZ - minZ)) * (max_size - min_size);
		float t = (z - minZ) / (maxZ - minZ);
		int bright = (int)(255.0f - (t * (255.0f - 50.0f)));

		unsigned int colour = 0xFF000000 | (bright << 16) | (bright << 8) | bright;

		draw_arc(app, model->screen_verts[i].x, model->screen_verts[i].y, size, size, 0, 64 * 360, colour);
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

void draw_lives(App *app, GameState *gs, int hw, int hh) {
	
	float x_offset = 0;
	int num_lives = gs->ship.lives;

	for(int i =0; i < num_lives; i++) {

		gs->lives.position = (Vector3) {0};
		Vector3 trans = {-hw + 95 + x_offset, +hh - 15, 0.0f};
		gs->lives.position = v3_add(gs->lives.position, trans);

		project(&gs->lives, hw, hh);
		draw_mesh(app, &gs->lives);
		x_offset += gs->lives.scale_s * 2;
	}
}

