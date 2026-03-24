
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "graphics.h"
#include "noise.h"
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
#define NUM_STARS 150

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
	Mesh3D master_asteroid;
	Mesh3D master_ship;
	Mesh3D master_title;
	Model3D lives;			//struct that holds mesh data for the players's lives
	Model3D title;
	Fontmap fontmap;
	Entity star_field[NUM_STARS];
	Entity noise_cloud;
	Sprite red_star_tex;
	Sprite white_star_tex;
	Sprite blue_star_tex;
	Sprite purple_star_tex;
	Sprite noise_tex;
	Sprite space_fog;
	ma_engine audio;
	ma_sound engine_sound;
	float timer;
	float game_end_time;
	float engine_pitch_t;
	bool keys[65536];
	bool space_pressed;
} GameState;

void init_noise_tex(GameState *gs);
void init_star_tex(Sprite *s, int w, int h, float r_exp, float g_exp, float b_exp);
void init_ship(GameState *gs);
void init_asteroids(GameState *gs);
void init_stars(GameState *gs);
void init_game(GameState *gs);
void process_spacebar(GameState *gs);
void update_logic(GameState *game_state, float delta_time);
void update_ship(GameState *gs);
void update_asteroids(GameState *gs);
void update_bullets(Bullet *bullets, double delta_time);
void draw_title(App *app, GameState *game_state);
void draw_ship(App *app, GameState *game_state);
void draw_game_over(App *app, GameState *game_state);
void draw_win(App *app, GameState *game_state);
void draw_asteroids(App *app, Asteroid *asteroids);
void draw_bullets(App *app, Bullet *bullets, int hw, int hh);
void draw_lives(App *app, GameState *gs, int hw, int hh);
void draw_stars(App *app, GameState *gs);
void draw_frame(App *app, GameState *game_state);
void draw_mesh(App *app, Model3D *model);
void check_collisions(GameState *game_state);
void screen_wrapping(Model3D *model);
void process_events(App *app, GameState *gs, XEvent *ev, int *running);
void project(Model3D *model, float hw, float hh);
void setModelDirection(Model3D *model, float amount);
int check_win(Asteroid *asteroids);
float random_float(float min, float max);
double get_time_seconds();

Vector3 star_verts[NUM_STARS];	
Vector3 star_verts_s[NUM_STARS];	

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
	
	double last_time = get_time_seconds();
	double target_time = 1.0 / 60.0;

	while (running) {

		double frame_start = get_time_seconds();
        	float delta_time = (float)(frame_start - last_time);
		game_state.timer += delta_time;
		last_time = frame_start;
		
		//process key and mouse events
		process_events(&app, &game_state, &ev, &running);

		//update logic
		update_logic(&game_state, delta_time);
		
		//render frame
		draw_frame(&app, &game_state);

		//Calculate how much time we spent doing work
		double frame_end = get_time_seconds();
		double time_spent_working = frame_end - frame_start;
		
		//Sleep only if we have time left over
		if (time_spent_working < target_time) {
		
			usleep((unsigned int)((target_time - time_spent_working) * 1000000));
		}

		//// Calculate potential FPS (how fast it could go)
		//float potential_fps = 1.0f / (float)time_spent_working;

		//// Calculate actual FPS (how fast it is actually going)
		//float actual_fps = 1.0f / delta_time;

		//// Print it every 60 frames so it's readable
		//static int frame_count = 0;

		//if (++frame_count >= 60) {
		//	
		//	printf("Potential FPS: %.2f | Actual FPS: %.2f\n", potential_fps, actual_fps);
		//	frame_count = 0;
		//}
	}	

	//free resources use by program		
	close_x(&app);
	model3D_free(&game_state.master_ship);
	model3D_free(&game_state.engine.mesh);
	model3D_free(&game_state.master_title);
	ma_engine_uninit(&game_state.audio);
	
	return 0;
}

void init_noise_tex(GameState *gs) {
	
	//set up Value noise propertys
	Value_noise vn = {0};
	vn.cell_size = 128;
	vn.width = 512;
	vn.height = 512;
	vn.bias = -166;
	
	//create the noise_tex sprite;
	init_vnoise(&vn, &gs->noise_tex);

	//create a new sprite the size of the screen and render the noise_tex into it using affine transforms
	gs->space_fog.width = PBUF_WIDTH;
	gs->space_fog.height = PBUF_HEIGHT;
	gs->space_fog.pixels = malloc(sizeof(uint32_t) * (PBUF_WIDTH * PBUF_HEIGHT));

	//set up the noise entity propertys
	gs->noise_cloud = create_entity(ENT_SPRITE, 0, 0);
	gs->noise_cloud.sprite = &gs->noise_tex;
	gs->noise_cloud.pos = (Vector3) {PBUF_WIDTH / 2, PBUF_HEIGHT / 2, 0.0f};
	gs->noise_cloud.scale = (Vector3) {4.0f, 2.1f, 0.0f};
	gs->noise_cloud.tint = 0xFFFF00FF;
	
	//render the noise_tex into a new sprite
	draw_sprite_affine(gs->space_fog.pixels, gs->space_fog.width, gs->space_fog.height, NORMAL, &gs->noise_cloud);
}

void init_star_tex(Sprite *s, int w, int h, float r_exp, float g_exp, float b_exp) {

	s->width = w;
	s->height = h;
	s->pixels = malloc(sizeof(uint32_t) * (w * h));
	
	//center of texture
	float cx = w / 2;
	float cy = h / 2;
	
	//radius of falloff function
	float radius = cx;

	for(int y = 0; y < s->height; y++) {
		
		for(int x = 0; x < s->width; x++) {
			
			//dist to center
			float dx = x - cx;
			float dy = y - cy;

			float dist = sqrt(dx * dx + dy * dy);
			float intensity = 1.0f - (dist / radius);
			
			//clamp to 0 or above 
			intensity = (intensity < 0) ? 0 : intensity;
			
			//falloff curve
			intensity = intensity * intensity;


			//calculate colour channel value
			//uint8_t c = (uint8_t)(intensity * 255);

			uint8_t r = (uint8_t)(pow(intensity, r_exp) * 255);
			uint8_t g = (uint8_t)(pow(intensity, g_exp) * 255);
			uint8_t b = (uint8_t)(pow(intensity, b_exp) * 255); 

			//put into pixel buffer
			s->pixels[y * s->width + x] = (0xFF << 24) | (r << 16) | (g << 8) | b;
		}
	}
}

void init_game (GameState *gs) {
	
	char *f_map = "abcdefghijklmnopqrstuvwxyz      ABCDEFGHIJKLMNOPQRSTUVWXYZ      0123456789.:,;(*!?'/\\\")$%^&-+@~#";
	load_font(&gs->fontmap, "./assets/fontmap.png", f_map, 16, 32, 0xFF00FF00);
	load_ply(&gs->master_ship, "./assets/ship.ply");
	load_ply(&gs->master_asteroid, "./assets/asteroid1.ply");
	load_ply(&gs->engine.mesh, "./assets/engine.ply");
	load_ply(&gs->master_title, "./assets/title.ply");
	init_star_tex(&gs->red_star_tex, 32, 32, 1.0, 1.6, 2.1);
	init_star_tex(&gs->white_star_tex, 32, 32, 1.0, 1.0, 1.0);
	init_star_tex(&gs->blue_star_tex, 32, 32, 2.0, 1.3, 1.0);
	init_star_tex(&gs->purple_star_tex, 32, 32, 1.0, 2.0, 1.0);
	init_noise_tex(gs);
	
	init_ship(gs);
	init_asteroids(gs);
	init_stars(gs);

	gs->engine.model.mesh = &gs->engine.mesh;
	gs->engine.model.screen_verts = malloc(gs->engine.mesh.local_count * sizeof(Vector3));
	
	gs->title.mesh = &gs->master_title;
	gs->title.screen_verts = malloc(gs->master_title.local_count * sizeof(Vector3));
	gs->title.scale = (Vector3) {500.0f, 500.0f, 500.0f};
	
	gs->lives.mesh = &gs->master_ship;
	gs->lives.screen_verts = malloc(gs->master_ship.local_count * sizeof(Vector3));
	gs->lives.scale = (Vector3) {15.0f, 15.0f, 15.0f};
	
	gs->engine.model.scale.x = gs->ship.model.scale.x;
	gs->engine.model.offset.y = -25.0f;
	gs->engine.max_blast_length = 85.0f;
}

void init_ship(GameState *gs) {
	
	gs->ship = (Ship) {0};
	gs->ship.model.mesh = &gs->master_ship;
	gs->ship.model.screen_verts = malloc(gs->master_ship.local_count * sizeof(Vector3));
	gs->ship.model.scale = (Vector3) {30.0f, 30.0f, 30.0f};
	gs->ship.model.direction = (Vector3) {0.0f, 1.0f, 0.0f}; //default forward position
	gs->ship.lives = 3;
}

void init_asteroids(GameState *gs) {
	
	Asteroid *asteroids = gs->asteroids;

	for (int i = 0; i < NUM_ASTEROIDS; i++) {
		
		float angle = random_float(0, TWO_PI);
		float speed = 0.3f;
		float vx = cosf(angle) * speed;
		float vy = sinf(angle) * speed;
		int hw = PBUF_WIDTH / 2;
		int hh = PBUF_HEIGHT / 2;
		
		asteroids[i].model = (Model3D) {0};
		asteroids[i].model.mesh = &gs->master_asteroid;
		asteroids[i].model.screen_verts = malloc(gs->master_asteroid.local_count * sizeof(Vector3));
		asteroids[i].spin = rand() % 2;
		asteroids[i].model.position = (Vector3) {(rand() % PBUF_WIDTH) - hw, (rand() % PBUF_HEIGHT) - hh, 0.0f};
		asteroids[i].model.velocity = (Vector3) {vx, vy, 0.0f};
		asteroids[i].model.rotation = (Vector3) {0.0f, 0.0f, 0.01f};

		if (i < 3) {

			asteroids[i].size = AST_LARGE;
			asteroids[i].model.scale = (Vector3) {60.0f, 60.0f, 60.0f};
			asteroids[i].alive = true;

		} else if (i > 2 && i < 12) {

			asteroids[i].size = AST_MEDIUM;
			asteroids[i].model.scale = (Vector3) {30.0f, 30.0f, 30.0f};
			asteroids[i].alive = false;

		} else {

			asteroids[i].size = AST_SMALL;
			asteroids[i].model.scale = (Vector3) {15.0f, 15.0f, 15.0f};
			asteroids[i].alive = false;
		}
	}
}

void init_stars(GameState *gs) {
	
	for (int i =0; i < NUM_STARS; i++) {
	
		//generate random x, y values for the entity sprite
		int x = rand() % PBUF_WIDTH;
		int y = rand() % PBUF_HEIGHT;
		int n = rand() % 100;
		float s = (rand() % 100) / 99.0f;
		s = 0.2f + (s * 0.25f);

		//create the entity sprite with default values
		gs->star_field[i] = create_entity(ENT_SPRITE, x, y);

		//assign a random scale value to the entity
		gs->star_field[i].scale = (Vector3) {s, s, 0}; 
		
		//assign a random sprite texture to the entity
		if (n < 70) {
			
			gs->star_field[i].sprite = &gs->white_star_tex;

		} else if (n >= 70 && n <= 90) {
			
			gs->star_field[i].sprite = &gs->red_star_tex;

		} else if (n >= 91 && n <= 98){
			
			gs->star_field[i].sprite = &gs->blue_star_tex;
		
		} else {
			
			gs->star_field[i].sprite = &gs->purple_star_tex;		
		}
	}
}

void update_ship(GameState *gs) {

	gs->ship.should_draw = true;
	gs->ship.invincible = false;

	//time elapsed since ship was spawned
	float elapsed = gs->timer - gs->ship.spawn_time;
	
	//set ship in invincible if it has just spawned
	if (elapsed < 3.0f) {
		
		gs->ship.invincible = true;
		gs->ship.should_draw = (fmodf(elapsed, 0.2f) > 0.1f);
	}

	//set game to game over if ship has no lives left
	if (gs->current_state == MAIN_GAME && gs->ship.lives < 0) {
	
		gs->current_state = GAME_OVER;
		gs->game_end_time = gs->timer;
	}

	//check if ship has collided with anything
	if (!gs->ship.invincible) {
		
		check_collisions(gs);
	}

	//rotate ship
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

	//update ship movemnt physics
	gs->ship.model.velocity = v3_add(gs->ship.model.velocity, gs->ship.model.acceleration);
	gs->ship.model.velocity = v3_limit_mag(gs->ship.model.velocity, SHIP_SPEED_LIMIT);
	gs->ship.model.position = v3_add(gs->ship.model.position, gs->ship.model.velocity);

	//Keep engine attached to ship
	gs->engine.model.position = gs->ship.model.position;

	//VISUAL EFFECTS (Flicker)
	float flicker = 1.0f + (sinf(gs->timer * 50.0f) * 0.05f); 
	gs->engine.model.scale.y = (gs->ship.model.scale_s + (gs->engine.current_blast_t * gs->engine.max_blast_length)) * flicker;
	
	// SCREEN WRAPPING
	screen_wrapping(&gs->ship.model);
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

void update_logic(GameState *game_state, float delta_time) {

	process_spacebar(game_state);

	switch (game_state->current_state) {

		case TITLE_SCREEN: {
			
			update_asteroids(game_state);
			break;
		}
		
		case MAIN_GAME: {
			
			update_ship(game_state);
			update_asteroids(game_state);
			update_bullets(game_state->bullets, delta_time);
			break;
		}

		case GAME_OVER: {
			
			update_asteroids(game_state);
			break;
		}

		case WIN_SCREEN: {
			
			update_ship(game_state);
			break;
		}

		default: {
			puts("unknown state");
			return;
		}
	}
}

void process_events(App *app, GameState *gs, XEvent *ev, int *running) {

	while (XPending(app->d)) {

		XNextEvent(app->d, ev);

		if (ev->type == ClientMessage) {
			
			if ((Atom)ev->xclient.data.l[0] == app->wmDeleteMessage) {

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

void process_spacebar(GameState *gs) {

	// If space isn't pressed, we have nothing to do here.
	if (!gs->space_pressed) {

		return;
	}

	// Handle the input based on where we are in the game
	switch (gs->current_state) {

		case TITLE_SCREEN:
			
			gs->current_state = MAIN_GAME;
			gs->ship.spawn_time = gs->timer;
			break;


		case MAIN_GAME:
		
			// Bullet firing logic
			for (int i = 0; i < NUM_BULLETS; i++) {

				if (!gs->bullets[i].alive) {
				
					Vector3 b_vel = {10.0f, -10.0f, 0.0f};
					Vector3 b_offset = v3_multi_s(gs->ship.model.direction, gs->ship.model.scale.x);

					gs->bullets[i].alive = true;
					gs->bullets[i].model.position = v3_add(gs->ship.model.position, b_offset);
					gs->bullets[i].model.position.y = -gs->bullets[i].model.position.y;
					gs->bullets[i].model.velocity = v3_multi(gs->ship.model.direction, b_vel);

					ma_engine_play_sound(&gs->audio, "./assets/Shoot.wav", NULL);
					break; // Exit loop after firing one bullet
				}
			}

			break;
		
		case WIN_SCREEN:

		case GAME_OVER:
			
			// Only allow restart after a 2-second buffer
			if (gs->timer > gs->game_end_time + 2.0f) {

				gs->current_state = TITLE_SCREEN;
				init_ship(gs);
				init_asteroids(gs);
				gs->engine.model.rotation = (Vector3){0};
			}

			break;
	}

	// Reset the flag at the very end so the player must press again
	gs->space_pressed = false; 
}

void draw_stars(App *app, GameState *gs) {
	
	//draw stars
	for (int i =0; i < NUM_STARS; i++) {
		
		draw_sprite_affine(app->pixel_buffer, app->pixel_buffer_w, app->pixel_buffer_h, ADDITIVE, &gs->star_field[i]);
	}
}

void draw_mesh(App *app, Model3D *model) {

	project(model, app->pixel_buffer_w / 2, app->pixel_buffer_h / 2);

	int offset = 0;

	//draw each face of the mesh
	for (int i = 0; i < model->mesh->facev_count; i++) {
	
		//verts per face
		int n = model->mesh->facev[i];

		for (int j = 0; j < n; j++) {

			int index_1 = model->mesh->meshf[offset + j];
			int index_2 = model->mesh->meshf[offset + (j + 1) % n];

			Vector3 v1 = model->screen_verts[index_1];
			Vector3 v2 = model->screen_verts[index_2];

			//draw line
			draw_line_b(app, v1.x, v1.y, v2.x, v2.y, 0x0000ff00);
		}

		offset += n;
	}
}

void draw_asteroids(App *app, Asteroid *asteroids) {

	//project asteroids to screen space and draw to screen
	for (int i = 0; i < NUM_ASTEROIDS; i++) {
		

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
			
			draw_filled_circle(app, new_pos.x, new_pos.y, 4, 0xFFFFFFFF);
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

		draw_mesh(app, &gs->lives);
		x_offset += gs->lives.scale.x * 2;
	}
}

void draw_title(App *app, GameState *game_state) {

	float hw = (float) app->pixel_buffer_w / 2.0f;	
	int chrw = game_state->fontmap.char_width;
	
	//copy pixel_buffer to the xlib pixmap for display
	char *play = "Press SPACE to Play";
	int len = (strlen(play) * chrw) / 2;

	draw_string(app, &game_state->fontmap, play, (int)hw - len, PBUF_HEIGHT - 100);
	
	draw_mesh(app, &game_state->title);
}

void draw_ship(App *app, GameState *game_state) {

	if (game_state->ship.should_draw) {
		
		draw_mesh(app, &game_state->ship.model);
	}

	if (game_state->engine.current_blast_t > 0.0f) {
		
		draw_mesh(app, &game_state->engine.model);
	}
}

void draw_game_over(App *app, GameState *game_state) {

	float hw = (float) app->pixel_buffer_w / 2.0f;	
	float hh = (float) app->pixel_buffer_h / 2.0f;
	char *over = "GAME OVER";
	char *replay = "Press SPACE to play again!";
	int chrw = game_state->fontmap.char_width;
	int leng = (strlen(over) * chrw) / 2;
	int len2 = (strlen(replay) * chrw) / 2;

	draw_string(app, &game_state->fontmap, over, (int)hw - leng, (int)hh);
	draw_string(app, &game_state->fontmap, replay, (int)hw - len2, PBUF_HEIGHT - 100);
	draw_asteroids(app, game_state->asteroids);
}

void draw_win(App *app, GameState *game_state) {

	float hw = (float) app->pixel_buffer_w / 2.0f;	
	float hh = (float) app->pixel_buffer_h / 2.0f;
	char *win = "YOU WIN !!!";
	char *replay2 = "Press SPACE to play again!";
	int chrw = game_state->fontmap.char_width;
	int len3 = (strlen(win) * chrw) / 2;
	int len4 = (strlen(replay2) * chrw) / 2;
	
	draw_string(app, &game_state->fontmap, win, (int)hw - len3,(int)hh);
	draw_string(app, &game_state->fontmap, replay2, (int)hw - len4, PBUF_HEIGHT - 100);
}

void draw_frame(App *app, GameState *game_state) {

	float hw = (float) app->pixel_buffer_w / 2.0f;	
	float hh = (float) app->pixel_buffer_h / 2.0f;
	
	//drawing operations every frame
	clear_screen(app, 0x000000);
	draw_sprite(app, &game_state->space_fog, 0, 0);
	draw_stars(app, game_state);
	
	//draw elements depending on the current game state 
	switch (game_state->current_state) {

		case TITLE_SCREEN: {
			
			draw_title(app, game_state);
			break;
		}
		
		case MAIN_GAME: {
			
			draw_string(app, &game_state->fontmap, "Lives", 0, 0);
			draw_ship(app, game_state);
			draw_lives(app, game_state, hw, hh);
			draw_asteroids(app, game_state->asteroids);
			draw_bullets(app, game_state->bullets, hw, hh);
			break;
		}

		case GAME_OVER: {
			
			draw_game_over(app, game_state);
			break;
		}

		case WIN_SCREEN: {
			
			draw_win(app, game_state);
			draw_ship(app, game_state);
			break;
		}

		default: {
			puts("unknown state");
			return;
		}
	}
	
	update_ximage(app);
	flip_buffer(app);
}

void project(Model3D *model, float hw, float hh) {
	
	for (int i = 0; i < model->mesh->local_count; i++) {

		Vector3 scaled_model = v3_multi(model->mesh->local_verts[i], model->scale); 
		Vector3 offset_model = v3_add(scaled_model, model->offset);
		Vector3 rot_model = v3_rotate(offset_model, model->rotation);
		Vector3 translation = v3_add(rot_model, model->position);
		
		//push vert to focal length
		translation.z += Z_OFFSET;

		model->screen_verts[i].x = hw + ((translation.x / translation.z) * FOCAL_LENGTH);
		model->screen_verts[i].y = hh - ((translation.y / translation.z) * FOCAL_LENGTH);
	}
}

void screen_wrapping(Model3D *model) {

	int hw = PBUF_WIDTH / 2;
	int hh = PBUF_HEIGHT / 2;

	if (model->position.x >  hw) model->position.x = -hw;
	if (model->position.x < -hw) model->position.x =  hw;
	if (model->position.y >  hh) model->position.y = -hh;
	if (model->position.y < -hh) model->position.y =  hh;
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

int check_win(Asteroid *asteroids) {

	for (int i = 0; i < NUM_ASTEROIDS; i ++) {

		if (asteroids[i].alive == true) {
			
			return 0;
		}
	}

	return 1;
}

void check_collisions(GameState *gs) {

	for(int i = 0; i < NUM_ASTEROIDS; i++) {

		Vector3 ship_pos = gs->ship.model.position;
		float ship_r = gs->ship.model.scale.x;
		Vector3 ast_pos = gs->asteroids[i].model.position;
		float ast_r = gs->asteroids[i].model.scale.x;

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

void setModelDirection(Model3D *model, float amount) {

	model->direction = (Vector3) {0.0f, 1.0f, 0.0f}; //default forward position
	model->rotation.z += amount;
	model->direction = v3_rotate(model->direction, model->rotation);
}

double get_time_seconds() {

	struct timespec ts;
	// CLOCK_MONOTONIC is immune to system clock jumps
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

