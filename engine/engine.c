#include <engine.h>

#include <util.h>

#include <SDL3/SDL.h>

struct Engine *gen_engine(void) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return NULL;
    }

    struct Engine *engine = (struct Engine *)S_malloc(sizeof(*engine));
    engine->window_count = 0;
    return engine;
}

bool add_window(struct Engine *engine, const char *title, int width, int height) {
    if (!engine || !title || width <= 0 || height <= 0 ||
        engine->window_count >= MAX_WINDOW_COUNT) {
        return false;
    }

    SDL_Window *window = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    engine->windows[engine->window_count++] = (struct Window){width, height, window};
    return true;
}

void run_engine(struct Engine *engine) {
    if (!engine || engine->window_count == 0) {
        return;
    }

    SDL_Event event;
    while (SDL_WaitEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT ||
            event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            break;
        }
    }
}

void free_engine(struct Engine *engine) {
    if (!engine) {
        return;
    }

    for (unsigned i = 0; i < engine->window_count; ++i) {
        SDL_DestroyWindow(engine->windows[i].window);
    }
    free(engine);
    SDL_Quit();
}
