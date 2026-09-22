#ifndef ENGINE_H
#define ENGINE_H

#include <stdbool.h>
#include <SDL3/SDL.h>

#define MAX_WINDOW_COUNT 10

struct Window {
	int width, height;
	SDL_Window *window;
};

struct Engine {
	struct Window windows[MAX_WINDOW_COUNT];
	unsigned window_count;
};

struct Engine *gen_engine(void);
bool add_window(struct Engine *engine, const char *title, int width, int height);
void run_engine(struct Engine *engine);
void free_engine(struct Engine *engine);

#endif
