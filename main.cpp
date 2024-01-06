#define _USE_MATH_DEFINES
#define _CRT_SECURE_NO_WARNINGS
#include<math.h>
#include<stdio.h>
#include<string.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH	800
#define SCREEN_HEIGHT	600
#define PLAYER_SIZE		32
#define SCREEN_MODE		0
#define SPAWN_POSX		64
#define SPAWN_POSY		SCREEN_HEIGHT-(100)
#define PLAYER_SPEED	1
#define AIR				0
#define WALL			1
#define PLATFORM		2
#define LADDER			3
#define NO_COLLISION	0
#define FLOOR_COLLISION 1
#define LEFT_COLLISION  2
#define RIGHT_COLLISION 3
//#define SCREEN_MODE SDL_WINDOW_FULLSCREEN_DESKTOP

struct player_t {
	int posX = SPAWN_POSX;
	int posY = SPAWN_POSY;
	int speedX = 0;
	int speedY = 0;
	int collision = 0;
	int isFalling = 0;
	int onLadder = 0;
	SDL_Surface* sprite;
};

struct game_t {
	int t1, t2, quit, frames, recordScore, score;
	double delta, worldTime, fpsTimer, fps;
	int** colliderMap;
	player_t player;
	SDL_Event event;
	SDL_Surface* screen, * charset;
	SDL_Texture* scrtex;
	SDL_Window* window;
	SDL_Renderer* renderer;
};

void DrawString(SDL_Surface* screen, int x, int y, const char* text, SDL_Surface* charset);
void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y);
void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color);
void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color, game_t* game, int material);
void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k, Uint32 outlineColor, Uint32 fillColor, game_t* game, int material, int fillMaterial = 0);
void initializeSDL(game_t* game);
void loadImages(game_t* game);
void cleanupSDL(game_t* game);
void update(game_t* game);
void render(game_t* game);
void handleEvents(game_t* game);
void movePlayer(game_t* game);






#ifdef __cplusplus
extern "C"
#endif

int main(int argc, char** argv) {
	game_t game;

	initializeSDL(&game);
	loadImages(&game);

	game.t1 = SDL_GetTicks();
	game.frames = 0;
	game.fpsTimer = 0;
	game.fps = 0;
	game.quit = 0;
	game.worldTime = 0;
	game.recordScore = 123456;
	game.score = 0;


	while (!game.quit) {
		game.t2 = SDL_GetTicks();
		game.delta = (game.t2 - game.t1) * 0.001;
		game.t1 = game.t2;

		handleEvents(&game);
		update(&game);
		render(&game);

		game.frames++;
	}

	cleanupSDL(&game);

	return 0;
}


	// aktualizacja stanu gry
	void update(game_t* game) {
		game->worldTime += game->delta;

		movePlayer(game);

		game->fpsTimer += game->delta;
		if (game->fpsTimer > 0.5) {
			game->fps = game->frames * 2.0;
			game->frames = 0;
			game->fpsTimer -= 0.5;
		}
	}


	// obsluguje zdarzenia
	void handleEvents(game_t* game) {
		while (SDL_PollEvent(&game->event)) {
			switch (game->event.type) {
			case SDL_KEYDOWN:
				if (game->event.key.keysym.sym == SDLK_ESCAPE) game->quit = 1;
				else if (game->event.key.keysym.sym == SDLK_n) {
					game->worldTime = 0;
					game->player.posX = SPAWN_POSX;
					game->player.posY = SPAWN_POSY;
					game->score = 0;
				}
				else {
					switch (game->event.key.keysym.sym) {
					case SDLK_UP:
						if(game->player.onLadder)
							game->player.speedY = -PLAYER_SPEED;
						break;
					case SDLK_DOWN:
						if (game->player.onLadder)
							game->player.speedY = PLAYER_SPEED;
						break;
					case SDLK_LEFT:
						game->player.speedX = -PLAYER_SPEED;
						break;
					case SDLK_RIGHT:
						game->player.speedX = PLAYER_SPEED;
						break;
					}
				}
				break;
			case SDL_KEYUP:
				if (game->event.key.keysym.sym == SDLK_UP || game->event.key.keysym.sym == SDLK_DOWN)
					game->player.speedY = 0;
				else if (game->event.key.keysym.sym == SDLK_LEFT || game->event.key.keysym.sym == SDLK_RIGHT)
					game->player.speedX = 0;
				break;
			case SDL_QUIT:
				game->quit = 1;
				break;
			}
		}
		game->frames++;
	}

	void checkCollision(game_t* game, char axis) {
		game->player.collision = NO_COLLISION;
		game->player.onLadder = 0;
		game->player.isFalling = 0;
		if (axis == 'x') {
			if (game->colliderMap[game->player.posX - (PLAYER_SIZE/2 + 1)][game->player.posY + (PLAYER_SIZE/2 - 1)] == WALL) {
				printf("Kolizja ze scianka po lewej\n");
				game->player.collision = LEFT_COLLISION;
			}
			else if (game->colliderMap[game->player.posX + (PLAYER_SIZE/2 + 1)][game->player.posY + (PLAYER_SIZE/2 - 1)] == WALL) {
				printf("Kolizja ze scianka po prawej\n");
				game->player.collision = RIGHT_COLLISION;
			}
		}
		else if (axis == 'y') {
			if (game->colliderMap[game->player.posX - PLAYER_SIZE/2][game->player.posY + PLAYER_SIZE/2] == PLATFORM) {
				printf("Kolizja z podloga po lewej\n");
				game->player.collision = FLOOR_COLLISION;
			}
			else if (game->colliderMap[game->player.posX + PLAYER_SIZE/2][game->player.posY + PLAYER_SIZE/2] == PLATFORM) {
				printf("Kolizja z podloga po prawej\n");
				game->player.collision = FLOOR_COLLISION;
			}
		}
		if (game->colliderMap[game->player.posX][game->player.posY] == LADDER) {
			printf("Gracz na drabinie\n");
			game->player.onLadder = 1;
		}
	}

	void movePlayer(game_t* game) {
		checkCollision(game, 'x');
		if ((game->player.speedX < 0 && game->player.collision == LEFT_COLLISION) ||
			(game->player.speedX > 0 && game->player.collision == RIGHT_COLLISION)) {
			//kolizja ze sciana
		}
		else {
			game->player.posX += game->player.speedX;
		}

		checkCollision(game, 'y');
		if (!game->player.onLadder) {
			if (game->player.collision != FLOOR_COLLISION) {
				game->player.isFalling = 1;
				game->player.posY += PLAYER_SPEED;
			}
		}
		else
		{
			if (game->player.collision == FLOOR_COLLISION) {
				game->player.posY -= PLAYER_SPEED;
			}
			game->player.posY += (game->player.speedY);
		}

	}

	// utworzenie okna i ekranu
	void initializeSDL(game_t* game) {
		if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
			printf("SDL_Init error: %s\n", SDL_GetError());
			exit(1);
		}

		int rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_MODE, &game->window, &game->renderer);
		if (rc != 0) {
			SDL_Quit();
			printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
			exit(1);
		}

		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
		SDL_RenderSetLogicalSize(game->renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
		SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 255);

		SDL_SetWindowTitle(game->window, "King Donkey - Mateusz Grzonka s198023");

		game->screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
			0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

		game->scrtex = SDL_CreateTexture(game->renderer, SDL_PIXELFORMAT_ARGB8888,
			SDL_TEXTUREACCESS_STREAMING,
			SCREEN_WIDTH, SCREEN_HEIGHT);

		game->colliderMap = (int**)malloc(SCREEN_WIDTH * sizeof(int*));
		for (int i = 0; i < SCREEN_WIDTH; i++) {
			game->colliderMap[i] = (int*)malloc(SCREEN_HEIGHT * sizeof(int));
		}
		for (int i = 0; i < SCREEN_HEIGHT; i++) {
			for (int j = 0; j < SCREEN_WIDTH; j++) {
				game->colliderMap[j][i] = 0;
			}
		}
	};


	// wczytanie tekstur
	void loadImages(game_t* game) {
		game->charset = SDL_LoadBMP("./cs8x8.bmp");
		if (game->charset == NULL) {
			printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
			cleanupSDL(game);
			exit(1);
		}

		game->player.sprite = SDL_LoadBMP("./gracz.bmp");
		if (game->player.sprite == NULL) {
			printf("SDL_LoadBMP(gracz.bmp) error: %s\n", SDL_GetError());
			cleanupSDL(game);
			exit(1);
		}
	};


	// zwolnienie pamieci
	void cleanupSDL(game_t* game) {
		for (int i = 0; i < SCREEN_WIDTH; i++) {
			free(game->colliderMap[i]);
		}
		free(game->colliderMap);

		SDL_FreeSurface(game->charset);
		SDL_FreeSurface(game->screen);
		SDL_DestroyTexture(game->scrtex);
		SDL_DestroyRenderer(game->renderer);
		SDL_DestroyWindow(game->window);
		SDL_Quit();
	}

	// rysuje wszystko na ekran
	void render(game_t* game) {
		char text[128];
		int czarny = SDL_MapRGB(game->screen->format, 0, 0, 0);
		int czerwony = SDL_MapRGB(game->screen->format, 255, 0, 0);
		int zielony = SDL_MapRGB(game->screen->format, 0, 255, 0);
		int niebieski = SDL_MapRGB(game->screen->format, 0, 0, 255);

		SDL_FillRect(game->screen, NULL, czarny);

		// interfejs
		DrawRectangle(game->screen, 4, 4, SCREEN_WIDTH - 8, 4 + 16 * 4, czerwony, czarny, game, WALL);
		sprintf(text, "- KING DONKEY -");
		DrawString(game->screen, game->screen->w / 2 - strlen(text) * 8 / 2, 10, text, game->charset);
		sprintf(text, "%.6g FPS", game->fps);
		DrawString(game->screen, 8, 8, text, game->charset);
		sprintf(text, "Mateusz Grzonka s198023");
		DrawString(game->screen, game->screen->w - strlen(text) * 8 - 8, 10, text, game->charset);

		sprintf(text, "CZAS: %.1lf s", game->worldTime);
		DrawString(game->screen, game->screen->w / 4 - strlen(text) * 8 / 2, 10 + 16 * 2, text, game->charset);
		sprintf(text, "REKORD: %d", game->recordScore);
		DrawString(game->screen, game->screen->w / 2 - strlen(text) * 8 / 2, 10 + 16 * 2, text, game->charset);
		sprintf(text, "PUNKTY: %d", game->score);
		DrawString(game->screen, game->screen->w * 3 / 4 - strlen(text) * 8 / 2, 10 + 16 * 2, text, game->charset);

		sprintf(text, "ESC - Wyjscie, N - Nowa gra");
		DrawString(game->screen, game->screen->w / 2 - strlen(text) * 8 / 2, 10 + 16 * 3, text, game->charset);

		// gra
		DrawRectangle(game->screen, 4, 12 + 16 * 4, SCREEN_WIDTH - 8, SCREEN_HEIGHT - 16 * 5, czerwony, czarny, game, WALL);


		DrawLine(game->screen, 5, SCREEN_HEIGHT - 50, SCREEN_WIDTH - 10, 1, 0, zielony, game, PLATFORM);
		DrawLine(game->screen, 93, 149, 110, 1, 0, zielony, game, PLATFORM);

		DrawLine(game->screen, 93, 249, 220, 1, 0, zielony, game, PLATFORM);
		DrawLine(game->screen, 420, 249, 220, 1, 0, zielony, game, PLATFORM);

		DrawLine(game->screen, 24, 349, 140, 1, 0, zielony, game, PLATFORM);
		DrawLine(game->screen, 256, 349, 190, 1, 0, zielony, game, PLATFORM);
		DrawLine(game->screen, 530, 349, 160, 1, 0, zielony, game, PLATFORM);

		DrawLine(game->screen, 205, 449, 420, 1, 0, zielony, game, PLATFORM);

		DrawRectangle(game->screen, 155, 149-16, 44, 110, niebieski, niebieski, game, LADDER, LADDER);

		DrawRectangle(game->screen, 97, 249 - 16, 44, 110, niebieski, niebieski, game, LADDER, LADDER);
		DrawRectangle(game->screen, 265, 249 - 16, 44, 110, niebieski, niebieski, game, LADDER, LADDER);
		DrawRectangle(game->screen, 592, 249 - 16, 44, 110, niebieski, niebieski, game, LADDER, LADDER);

		DrawRectangle(game->screen, 533, 349 - 16, 44, 110, niebieski, niebieski, game, LADDER, LADDER);

		DrawRectangle(game->screen, 209, 449 - 16, 44, 110, niebieski, niebieski, game, LADDER, LADDER);

		DrawSurface(game->screen, game->player.sprite, game->player.posX, game->player.posY);

		SDL_UpdateTexture(game->scrtex, NULL, game->screen->pixels, game->screen->pitch);
		// SDL_RenderClear(renderer);
		SDL_RenderCopy(game->renderer, game->scrtex, NULL, NULL);
		SDL_RenderPresent(game->renderer);
	}


	// narysowanie napisu txt na powierzchni screen, zaczynajac od punktu (x, y)
	// charset to bitmapa 128x128 zawierajaca znaki
	void DrawString(SDL_Surface* screen, int x, int y, const char* text, SDL_Surface* charset) {
		int px, py, c;
		SDL_Rect s, d;
		s.w = 8;
		s.h = 8;
		d.w = 8;
		d.h = 8;
		while (*text) {
			c = *text & 255;
			px = (c % 16) * 8;
			py = (c / 16) * 8;
			s.x = px;
			s.y = py;
			d.x = x;
			d.y = y;
			SDL_BlitSurface(charset, &s, screen, &d);
			x += 8;
			text++;
		};
	};


	// narysowanie na ekranie screen powierzchni sprite w punkcie (x, y)
	// (x, y) to punkt srodka obrazka sprite na ekranie
	void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
		SDL_Rect dest;
		dest.x = x - sprite->w / 2;
		dest.y = y - sprite->h / 2;
		dest.w = sprite->w;
		dest.h = sprite->h;
		SDL_BlitSurface(sprite, NULL, screen, &dest);
	};


	// rysowanie pojedynczego pixela
	void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
		int bpp = surface->format->BytesPerPixel;
		Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
		*(Uint32*)p = color;
	};


	// rysowanie linii o dlugosci l 
	// pionowa (dx = 0, dy = 1)
	// pozioma (dx = 1, dy = 0)
	void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color, game_t* game, int material) {
		for (int i = 0; i < l; i++) {
			DrawPixel(screen, x, y, color);
			if (material == LADDER)
				game->colliderMap[x][y] = material;
			else
				game->colliderMap[x][y] = material;
			x += dx;
			y += dy; 
		};
	};


	// rysowanie prostokata o dlugosci boków l i k
	void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k, Uint32 outlineColor, Uint32 fillColor, game_t* game, int material, int fillMaterial) {
		int i;
		DrawLine(screen, x, y, k, 0, 1, outlineColor, game, material);
		DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor, game, material);
		DrawLine(screen, x, y, l, 1, 0, outlineColor, game, material);
		DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor, game, material);
		for (i = y + 1; i < y + k - 1; i++)
			DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor, game, fillMaterial);
	};
