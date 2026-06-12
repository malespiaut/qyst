#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sexp.h>

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include "log.h"
#include "str.h"
#include "types.h"

#define kScreenWidth 640
#define kScreenHeight 480

bool g_quit = false;

typedef struct screen_manager_s screen_manager_t;
struct screen_manager_s
{
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Texture* texture;
  SDL_Surface* surface;
};

typedef struct clickbox_s clickbox_t;
struct clickbox_s
{
  SDL_Rect bounds;
  i32 scene_id;
  MIX_Audio* sound;
  /* SDL_Rect bounds_original; */
};

typedef struct scene_s scene_t;
struct scene_s
{
  i32 id;
  char* name;
  SDL_Surface* background;
  SDL_Texture* background_texture;
  clickbox_t** clickbox;
  usize clickbox_count;
  MIX_Audio* song;
  char* song_path;
  /* sprite_t sprite; */
  /* i32 spritecount; */
};

typedef struct game_manager_s game_manager_t;
struct game_manager_s
{
  screen_manager_t screen;
  bool quit;
  usize scene_count;
  usize scene_current;
  SDL_Point mouse_position;
  // char* scenes_path;
  scene_t** scene;
};

static void
scene_draw(game_manager_t* gm)
{
  // SDL_Rect r = {.x = 0, .y = 0, .w = kScreenWidth, .h = kScreenHeight};

  // SDL_BlitSurface(gm->scene[gm->scene_current]->background, 0, gm->screen.surface, 0);

  /*
  if (!SDL_UpdateTexture(gm->screen.texture, NULL, gm->scene[gm->scene_current]->background->pixels, sizeof(u32)*kScreenWidth))
  {
    SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Can't update the given texture rectangle with new pixel data: %s\n", SDL_GetError());
    SDL_DestroyWindow(gm->screen.window);
    SDL_Quit();
    exit(SDL_APP_FAILURE);
  }
  */
  SDL_RenderTexture(gm->screen.renderer, gm->scene[gm->scene_current]->background_texture, NULL, NULL);
  // SDL_UpdateWindowSurface(gm->screen.window);
}

static void
game_draw(game_manager_t* gm)
{
  /* Drawing */
  // SDL_SetRenderDrawColor(gm->screen.renderer, 0, 0, 0, 255); /* Black */
  // SDL_RenderClear(gm->screen.renderer);

  /* Example: Draw a white rectangle */
  // SDL_FRect rect = {0.0f, 0.0f, kScreenWidth, kScreenHeight};
  // SDL_SetRenderDrawColor(gm->screen.renderer, 255, 0, 0, 255); /* White */
  // SDL_RenderFillRect(gm->screen.renderer, &rect);

  SDL_RenderClear(gm->screen.renderer);
  scene_draw(gm);
  // SDL_RenderTexture(gm->screen.renderer, gm->screen.texture, NULL, NULL);
  SDL_RenderPresent(gm->screen.renderer);
}

static void
events_process(game_manager_t* gm)
{
  SDL_Event event = {0};

  /* Event handling */
  while (SDL_PollEvent(&event))
  {
    if (event.type == SDL_EVENT_QUIT)
    {
      gm->quit = true;
    }
    /* Add other event handling here (keyboard, mouse, etc.) */
    if (event.type == SDL_EVENT_KEY_DOWN)
    {
      if (event.key.key == SDLK_ESCAPE)
      {
        gm->quit = true;
      }
    }
  }
}

// TODO: REVIEW!!!!!!!
static usize
clickboxes_count(sexp_t* s)
{
  usize result = 0;

  log_debug("Counting the number of clickboxes");

  if (!s)
  {
    log_error("Pointers are NULL!");
    exit(EXIT_FAILURE);
  }

  if (s->ty != SEXP_LIST)
  {
    log_error("Scenes aren't correctly structured!");
    exit(EXIT_FAILURE);
  }

  for (; s; s = s->next)
  {
    ++result;
  }

  log_debug("There are %ld clickboxes", result);

  return result;
}

static void
scene_background_load(scene_t* scene, char* path)
{
  if (!scene || !path)
  {
    log_error("Pointers are NULL!");
    exit(EXIT_FAILURE);
  }

  scene->background = SDL_LoadBMP(path);
  if (!scene->background)
  {
    log_error("SDL_LoadBMP couldn't load %s", path);
    exit(EXIT_FAILURE);
  }
}

static void
scene_texture_load(scene_t* scene, SDL_Renderer* renderer)
{
  scene->background_texture = SDL_CreateTextureFromSurface(renderer, scene->background);
  if (!scene->background_texture)
  {
    log_error("Can't create a texture from an existing surface: %s", SDL_GetError());
    exit(EXIT_FAILURE);
  }
}

static void
scene_music_load(scene_t* scene, char* path)
{
  if (!scene || !path)
  {
    log_error("Pointers are NULL!");
    exit(EXIT_FAILURE);
  }

  // TODO: I JUST WANT TO PLAY A DAMN WAV FILE!!!!!!!!!!
  /*
  if (!SDL_LoadWAV(path, 0, 0, 0))
  {
    log_error("SDL_LoadWAV couldn't load %s: %s", path, SDL_GetError());
    exit(EXIT_FAILURE);
  }
  */
  scene->song_path = path;
}

static bool
is_valid_clickbox(clickbox_t* cb)
{
  return (
    (cb->scene_id >= 0) &&
    (cb->bounds.x >= 0) && (cb->bounds.x <= kScreenWidth) &&
    (cb->bounds.y >= 0) && (cb->bounds.y <= kScreenHeight) &&
    ((cb->bounds.x + cb->bounds.w) >= 0) && ((cb->bounds.x + cb->bounds.w) <= kScreenWidth) &&
    ((cb->bounds.y + cb->bounds.h) >= 0) && ((cb->bounds.y + cb->bounds.h) <= kScreenHeight));
}

static bool
is_value(sexp_t* s)
{
  return (s && s->ty == SEXP_VALUE);
}

static i32
scene_id_find(game_manager_t* gm, char* scene_name)
{
  for (usize i = 0; i < gm->scene_count; ++i)
  {
    if (gm->scene[i]->name && !strcmp(gm->scene[i]->name, scene_name))
    {
      return i;
    }
  }
  log_error("Target \"%s\" doesn't exist!", scene_name);
  return -1;
}

static void
clickbox_parse(game_manager_t* gm, sexp_t* s, clickbox_t* cb)
{
  if (!s || !cb)
  {
    log_error("Pointers are NULL!");
    exit(EXIT_FAILURE);
  }
  if (s->ty != SEXP_LIST)
  {
    log_error("Clickbox isn't a list!");
    exit(EXIT_FAILURE);
  }

  log_debug("Processing clickbox content");

  sexp_t* cursor = s->list;
  while (cursor)
  {
    const char* type = cursor->list->val;

    // Bounds
    if (!strncmp(type, "bounds", 6) && is_value(cursor->list->next))
    {
      // background_parse(cursor, scene);
      // scene_background_load(scene, cursor->list->next->val);
      cb->bounds.x = atoi(cursor->list->next->val);
      cb->bounds.y = atoi(cursor->list->next->next->val);
      cb->bounds.w = atoi(cursor->list->next->next->next->val);
      cb->bounds.h = atoi(cursor->list->next->next->next->next->val);
    }

    // Target
    else if (!strncmp(type, "scene", 6) && is_value(cursor->list->next))
    {
      cb->scene_id = scene_id_find(gm, cursor->list->next->val);

      // TODO!!!!!!!!!!!!!!
      // scene_music_load(scene, cursor->list->next->val);
    }

    cursor = cursor->next;
  }
}

static void
clickboxes_init(scene_t* scene)
{
  log_debug("Allocation space for clickboxes");

  scene->clickbox = malloc(sizeof(clickbox_t*) * scene->clickbox_count);
  if (!scene->clickbox)
  {
    perror("ERROR: clickbox_init(): Couldn't allocate clickbox memory!");
    exit(EXIT_FAILURE);
  }

  for (usize i = 0; i < scene->clickbox_count; ++i)
  {
    scene->clickbox[i] = malloc(sizeof(clickbox_t));
    if (!scene->clickbox[i])
    {
      perror("ERROR: clickbox_init(): Couldn't allocate clickbox memory!");
      exit(EXIT_FAILURE);
    }

    // -1 means unset
    memset(scene->clickbox[i], -1, sizeof(clickbox_t));
    // `sound` is a pointer
    scene->clickbox[i]->sound = 0;
  }

  log_debug("%ld clickboxes have been successfuly allocated!", scene->clickbox_count);
}

static bool
is_next_element_value(sexp_t* s)
{
  return (s->list->next && s->list->next->ty == SEXP_VALUE);
}

static bool
is_next_element_list(sexp_t* s)
{
  return (s->list->next && s->list->next->ty == SEXP_LIST);
}

static usize
scenes_count(sexp_t* s)
{
  usize result = 0;

  log_debug("Counting the number of scenes");

  if (!s)
  {
    log_error("Pointers are NULL!");
    exit(EXIT_FAILURE);
  }

  if (s->ty != SEXP_LIST)
  {
    log_error("Scenes aren't correctly structured!");
    exit(EXIT_FAILURE);
  }

  for (sexp_t* scene = s->list; scene; scene = scene->next)
  {
    ++result;
  }

  log_debug("There are %ld scenes", result);

  return result;
}

static void
scene_parse(game_manager_t* gm, sexp_t* s, scene_t* scene)
{
  if (!s || !scene)
  {
    log_error("Pointers are NULL!");
    exit(EXIT_FAILURE);
  }
  if (s->ty != SEXP_LIST)
  {
    log_error("Scene isn't a list!");
    exit(EXIT_FAILURE);
  }

  scene->name = s->list->val;

  log_debug("Processing scene \"%s\" content", scene->name);

  sexp_t* cursor = s->list->next;
  while (cursor)
  {
    const char* type = cursor->list->val;

    // Background
    if (!strncmp(type, "image", 5) && is_next_element_value(cursor))
    {
      // background_parse(cursor, scene);
      scene_background_load(scene, cursor->list->next->val);
      scene_texture_load(scene, gm->screen.renderer);
    }

    // Music
    else if (!strncmp(type, "music", 5) && is_next_element_value(cursor))
    {
      scene_music_load(scene, cursor->list->next->val);
    }

    // Click-boxes
    else if (!strncmp(type, "click-boxes", 11) && is_next_element_list(cursor))
    {
      sexp_t* cursor_cb = cursor->list->next;

      scene->clickbox_count = clickboxes_count(cursor_cb);

      clickboxes_init(scene);

      for (usize i = 0; i < scene->clickbox_count; ++i)
      {
        clickbox_parse(gm, cursor_cb, scene->clickbox[i]);

        if (!is_valid_clickbox(scene->clickbox[i]))
        {
          log_error("Clickbox is invalid!");
          exit(EXIT_FAILURE);
        }

        cursor_cb = cursor_cb->next;
      }
    }
    cursor = cursor->next;
  }
}

static sexp_t*
scenes_load(char* path)
{
  if (!path)
  {
    log_error("Scenes path isn't properly initialized!");
    exit(EXIT_FAILURE);
  }

  // Loading sexp file in memory

  log_debug("Loading file %s", path);
  FILE* fp = fopen(path, "r");
  if (!fp)
  {
    char buf[256] = {0};
    snprintf(buf, sizeof(buf), "ERROR: scenes_load(): Couldn't open %s", path);
    perror(buf);
    exit(EXIT_FAILURE);
  }

  if (fseek(fp, 0, SEEK_END))
  {
    char buf[256] = {0};
    snprintf(buf, sizeof(buf), "ERROR: scenes_load(): Couldn't seek %s", path);
    perror(buf);
    exit(EXIT_FAILURE);
  }

  usize fsize = (usize)ftell(fp); // FIXME!!! ftell returns -1 on errors.
  if (fseek(fp, 0, SEEK_SET))
  {
    char buf[256] = {0};
    snprintf(buf, sizeof(buf), "ERROR: scenes_load(): Couldn't seek %s", path);
    perror(buf);
    exit(EXIT_FAILURE);
  }

  char* buffer = (char*)malloc(fsize + 1);
  if (!buffer)
  {
    fclose(fp);
    perror("ERROR: scenes_load(): Couldn't allocate memory!");
    exit(EXIT_FAILURE);
  }

  usize bytes_read = fread(buffer, 1, fsize, fp);
  buffer[bytes_read] = '\0';
  if (fclose(fp))
  {
    char buf[256] = {0};
    snprintf(buf, sizeof(buf), "ERROR: scenes_load(): Couldn't close %s", path);
    perror(buf);
    exit(EXIT_FAILURE);
  }
  log_debug("Loaded %zu bytes", fsize);

  // Processing loaded sexp file

  sexp_t* sexp = parse_sexp(buffer, bytes_read);
  free(buffer);

  if (!sexp)
  {
    log_error("Couldn't parse scenes");
    free(buffer);
    exit(EXIT_FAILURE);
  }
  log_debug("Scenes parsed successfully!");

  log_debug("Checking if scenes are strucrured collectly");
  if (sexp->ty != SEXP_LIST)
  {
    log_error("Scenes aren't structured correctly!");
    exit(EXIT_FAILURE);
  }

  return sexp;
}

static void
scenes_init(game_manager_t* gm, sexp_t* scene)
{
  log_debug("Allocation space for scenes");

  gm->scene = malloc(sizeof(scene_t*) * gm->scene_count);
  if (!gm->scene)
  {
    perror("ERROR: scenes_init(): Couldn't allocate scene memory!");
    exit(EXIT_FAILURE);
  }

  for (usize i = 0; i < gm->scene_count; ++i)
  {
    gm->scene[i] = malloc(sizeof(scene_t));
    if (!gm->scene[i])
    {
      perror("ERROR: scenes_init(): Couldn't allocate scene memory!");
      exit(EXIT_FAILURE);
    }
    memset(gm->scene[i], 0, sizeof(scene_t));
  }

  log_debug("Setting scenes names");
  for (usize i = 0; i < gm->scene_count; ++i)
  {
    gm->scene[i]->name = scene->list->val;
    scene = scene->next;
  }

  log_debug("%ld scenes have been successfuly allocated!", gm->scene_count);
}

static void
game_update(game_manager_t* gm)
{
  SDL_FRect mouse_fposition = {0};
  SDL_GetMouseState(&mouse_fposition.x, &mouse_fposition.y);
  gm->mouse_position.x = (i32)mouse_fposition.x;
  gm->mouse_position.y = (i32)mouse_fposition.y;

  char buffer[256] = {0};
  sprintf(buffer, "X:%d, Y:%d", gm->mouse_position.x, gm->mouse_position.y);
  SDL_SetWindowTitle(gm->screen.window, buffer);
}

static void
game_init(game_manager_t* gm)
{
  // gm->scenes_path = "scenes.sexp";
  sexp_t* scenes = scenes_load("scenes.sexp");
  gm->scene_count = scenes_count(scenes);

  sexp_t* scene = scenes->list;
  scenes_init(gm, scene);
  for (usize i = 0; i < gm->scene_count; ++i)
  {
    scene_parse(gm, scene, gm->scene[i]);
    scene = scene->next;
  }
}

int
main(void)
{
  // Init game manager
  game_manager_t gm = {0};

  if (!SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO))
  {
    SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Can't initialize the SDL library: %s\n", SDL_GetError());
    SDL_Quit();
    return SDL_APP_FAILURE;
  }

  gm.screen.window = SDL_CreateWindow("Myst-like engine", kScreenWidth, kScreenHeight, 0);
  if (!gm.screen.window)
  {
    SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Can't create a window with the specified dimensions and flags: %s\n", SDL_GetError());
    SDL_Quit();
    return SDL_APP_FAILURE;
  }

  gm.screen.renderer = SDL_CreateRenderer(gm.screen.window, NULL);
  if (!gm.screen.renderer)
  {
    SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Can't create a 2D rendering context for a window: %s\n", SDL_GetError());
    SDL_DestroyWindow(gm.screen.window);
    SDL_Quit();
    return SDL_APP_FAILURE;
  }

  gm.screen.texture = SDL_CreateTexture(gm.screen.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, kScreenWidth, kScreenHeight);
  if (!gm.screen.texture)
  {
    SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Can't create a 2D rendering context for a window: %s\n", SDL_GetError());
    SDL_DestroyWindow(gm.screen.window);
    SDL_Quit();
    return SDL_APP_FAILURE;
  }

  if (!SDL_SetRenderLogicalPresentation(gm.screen.renderer, kScreenWidth, kScreenHeight, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE))
  {
    SDL_LogWarn(SDL_LOG_CATEGORY_RENDER, "Can't set a device-independent resolution and presentation mode for rendering: %s\n", SDL_GetError());
  }

  // TODO: HARDEN!
  gm.screen.surface = SDL_GetWindowSurface(gm.screen.window);

  SDL_Log("SDL3 Application Started!");

  game_init(&gm);
  while (!gm.quit)
  {
    events_process(&gm);
    game_update(&gm);
    game_draw(&gm);
  }

  SDL_Log("SDL3 Application Quitting...");
  SDL_DestroyRenderer(gm.screen.renderer);
  SDL_DestroyWindow(gm.screen.window);
  SDL_QuitSubSystem(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  SDL_Quit();

  return SDL_APP_SUCCESS;
}
