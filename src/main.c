#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-prototypes"
#include <sexp.h>
#pragma GCC diagnostic pop

#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include "log.h"
#include "str.h"
#include "types.h"

#define kScreenWidth 640
#define kScreenHeight 480

bool g_quit = false;

enum gamestate_e
{
  eGamestateVideo = 1u << 0,
  eGamestateAudio = 1u << 1,
  eGamestatePlay = 1u << 2
};
typedef enum gamestate_e gamestate_t;

enum todo_e
{
  eTodoPlayVideo = 1u << 0,
  eTodoPlayAudio = 1u << 1,
  eTodoSetScene = 1u << 2
};
typedef enum todo_e todo_t;

typedef struct stack_s stack_t;
struct stack_s
{
  plm_t* video;
  MIX_Audio* audio;
  i32 scene_id;
};

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
  plm_t* video;
  MIX_Audio* audio;
  // SDL_Rect bounds_original;
};

typedef struct scene_s scene_t;
struct scene_s
{
  i32 id;
  char* name;
  SDL_Surface* background;
  SDL_Texture* background_texture;
  clickbox_t** clickbox;
  i32 clickbox_count;
  MIX_Audio* music;
  char* music_path;
};

typedef struct video_s video_t;
struct video_s
{
  bool playing;
  plm_t* player;
  SDL_Texture* texture;
  SDL_FRect rectangle;
};

typedef struct game_manager_s game_manager_t;
struct game_manager_s
{
  screen_manager_t screen;
  bool quit;
  double last_time;
  i32 scene_count;
  i32 scene_current;

  SDL_Point mouse_position;
  // char* scenes_path;
  scene_t** scene;
  video_t video;
  todo_t todo;
  stack_t stack;

  gamestate_t gamestate;
};

u32 palette_websafe[216];

static void
palette_websafe_init_pro(void)
{
  static const u8 levels[6] = {0x00, 0x33, 0x66, 0x99, 0xcc, 0xff};

  usize i = 0;

  for (usize g = 0; g < 6; ++g)
  {
    for (usize b = 0; b < 6; ++b)
    {
      for (usize r = 0; r < 6; ++r)
      {
        u32 rr = (u32)levels[r];
        u32 gg = (u32)levels[g];
        u32 bb = (u32)levels[b];

        palette_websafe[i++] =
          0xff000000u |
          (rr << 16) |
          (gg << 8) |
          (bb);
      }
    }
  }
}

static void
on_video(plm_t* player, plm_frame_t* frame, void* user)
{
  game_manager_t* gm = (game_manager_t*)user;

  SDL_UpdateYUVTexture(gm->video.texture, NULL, frame->y.data, frame->y.width, frame->cb.data, frame->cb.width, frame->cr.data, frame->cr.width);
}

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
  // Drawing
  // SDL_SetRenderDrawColor(gm->screen.renderer, 0, 0, 0, 255); //Black
  // SDL_RenderClear(gm->screen.renderer);

  // Example: Draw a white rectangle
  // SDL_FRect rect = {0.0f, 0.0f, kScreenWidth, kScreenHeight};
  // SDL_SetRenderDrawColor(gm->screen.renderer, 255, 0, 0, 255); // White
  // SDL_RenderFillRect(gm->screen.renderer, &rect);

  SDL_RenderClear(gm->screen.renderer);

  // Always render the scene
  scene_draw(gm);
  if (gm->video.playing)
  {
    i32 video_width = plm_get_width(gm->video.player);
    i32 video_height = plm_get_height(gm->video.player);
    // If the video is not of the same resolution of the screen
    if ((video_width < kScreenWidth) && (video_height < kScreenHeight))
    {
      // Render the video centered
      SDL_RenderTexture(gm->screen.renderer, gm->video.texture, &gm->video.rectangle, &(SDL_FRect){.x = (kScreenWidth - video_width) / 2, .y = (kScreenHeight - video_height) / 2, .w = video_width, .h = video_height});
    }
    else
    {
      // Render the video fullscreen
      SDL_RenderTexture(gm->screen.renderer, gm->video.texture, &gm->video.rectangle, &gm->video.rectangle);
    }
  }
  else
  {
    // TODO: Draw cursor and font only when the video isn't playing
  }
  // SDL_RenderTexture(gm->screen.renderer, gm->screen.texture, NULL, NULL);
  SDL_RenderPresent(gm->screen.renderer);
}

static void
play_video(game_manager_t* gm, plm_t* video)
{
  gm->video.playing = true;
  gm->video.player = video;
  gm->video.texture = SDL_CreateTexture(
    gm->screen.renderer,
    SDL_PIXELFORMAT_IYUV,
    SDL_TEXTUREACCESS_STREAMING,
    plm_get_width(video),
    plm_get_height(video)); // TODO: free, lol

  gm->video.rectangle.w = plm_get_width(video);
  gm->video.rectangle.h = plm_get_height(video);

  gm->last_time = (double)SDL_GetTicks() / 1000.0;

  log_debug("Playing video");
}

void
update_video(game_manager_t* gm)
{
  if (plm_has_ended(gm->video.player))
  {
    plm_rewind(gm->video.player);
    gm->video.playing = false;
    log_debug("Video ended");
  }

  // From pl_mpeg example:
  // Compute the delta time since the last app_update(), limit max step to
  // 1/30th of a second
  double current_time = (double)SDL_GetTicks() / 1000.0;
  double elapsed_time = current_time - gm->last_time;
  if (elapsed_time > 1.0 / 30.0)
  {
    elapsed_time = 1.0 / 30.0;
  }
  gm->last_time = current_time;

  plm_decode(gm->video.player, elapsed_time);
  // log_debug("vid time : %lf", plm_get_time(gm->video.player));
}

static void
todo_process(game_manager_t* gm)
{
  if (gm->todo)
  {
    if ((gm->todo & eTodoPlayVideo) && (gm->gamestate != eGamestateVideo))
    {
      gm->gamestate = eGamestateVideo;

      play_video(gm, gm->stack.video);
    }

    if (!(gm->todo & eTodoPlayVideo) && (gm->todo & eTodoPlayAudio) && (gm->gamestate != eGamestateAudio))
    {
      gm->gamestate = eGamestateAudio;

      // Clearing out the *play audio* flag
      gm->todo &= ~eTodoPlayAudio;
    }

    if (!(gm->todo & eTodoPlayVideo) && !(gm->todo & eTodoPlayAudio) && (gm->todo & eTodoSetScene))
    {
      gm->scene_current = gm->stack.scene_id;
      gm->gamestate = eGamestatePlay;

      // Clearing out everything
      memset(&gm->stack, 0, sizeof(stack_t));
      gm->todo = 0;
    }
  }
}

static void
handle_click(game_manager_t* gm, i32 x, i32 y)
{
  for (usize i = 0; i < gm->scene[gm->scene_current]->clickbox_count; ++i)
  {
    clickbox_t* cb = gm->scene[gm->scene_current]->clickbox[i];
    SDL_Rect r = cb->bounds;
    if ((x >= r.x) && (x <= (r.x + r.w)) &&
        (y >= r.y) && (y <= (r.y + r.h)))
    {
      log_debug("Click inside of clickbox %d, switching to scene \"%s\"", i, gm->scene[cb->scene_id]->name);

      if (cb->video)
      {
        gm->todo |= eTodoPlayVideo;
        gm->stack.video = cb->video;
      }
      if (cb->audio)
      {
        gm->todo |= eTodoPlayAudio;
        gm->stack.audio = cb->audio;
      }
      if (cb->scene_id)
      {
        gm->todo |= eTodoSetScene;
        gm->stack.scene_id = cb->scene_id;
      }

      return;
    }
  }

  log_debug("Click outside of any clickbox");
}

static void
events_process(game_manager_t* gm)
{
  SDL_Event event = {0};

  // Event handling
  while (SDL_PollEvent(&event))
  {
    switch (event.type)
    {

      case SDL_EVENT_QUIT:
        gm->quit = true;
        break;
      // Add other event handling here (keyboard, mouse, etc.)
      case SDL_EVENT_KEY_DOWN:
        if (event.key.key == SDLK_ESCAPE)
        {
          gm->quit = true;
        }
        break;
      case SDL_EVENT_MOUSE_BUTTON_UP:
        if (!gm->video.playing)
        {
          handle_click(gm, event.button.x, event.button.y);
        }
        break;
    }
  }
}

// TODO: REVIEW!!!!!!!
static i32
clickboxes_count(sexp_t* s)
{
  i32 result = 0;

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
  scene->music_path = path;
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

    // Clip
    else if (!strncmp(type, "video", 5) && is_value(cursor->list->next))
    {
      char* path = cursor->list->next->val;
      cb->video = plm_create_with_filename(path);
      if (!cb->video)
      {
        log_error("Couldn't open '%s'", path);
        exit(EXIT_FAILURE);
      }
      plm_set_audio_enabled(cb->video, false);
      plm_set_video_decode_callback(cb->video, on_video, gm);
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
    // `audio` is a pointer
    scene->clickbox[i]->audio = 0;
    scene->clickbox[i]->video = NULL;
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

static i32
scenes_count(sexp_t* s)
{
  i32 result = 0;

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

  {
    char buffer[256] = {0};
    sprintf(buffer, "X:%d, Y:%d", gm->mouse_position.x, gm->mouse_position.y);
    SDL_SetWindowTitle(gm->screen.window, buffer);
  }

  switch (gm->gamestate)
  {
    case eGamestateVideo:
      if (gm->video.playing)
      {
        update_video(gm);
      }

      else
      {
        // If video is finished playing, clearing out the *play video* flag
        gm->todo &= ~eTodoPlayVideo;
      }
      break;
    case eGamestateAudio:
      break;
    case eGamestatePlay:
    default:
      break;
  }
}

static void
game_init(game_manager_t* gm)
{
  // gm->scenes_path = "scenes.sexp";
  sexp_t* scenes = scenes_load("data/scenes.sexp");
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

  // These calls are optional
  // They only serves to make the pixels square and clean when resizing the window
  if (!SDL_SetRenderVSync(gm.screen.renderer, SDL_RENDERER_VSYNC_ADAPTIVE))
  {
    fprintf(stderr, "SDL_SetRenderVSync() failed: %s", SDL_GetError());
  }
  if (!SDL_SetRenderLogicalPresentation(gm.screen.renderer, kScreenWidth, kScreenHeight, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE))
  {
    fprintf(stderr, "SDL_SetRenderLogicalPresentation() failed: %s", SDL_GetError());
  }
  if (!SDL_SetDefaultTextureScaleMode(gm.screen.renderer, SDL_SCALEMODE_NEAREST))
  {
    fprintf(stderr, "SDL_SetDefaultTextureScaleMode() failed: %s", SDL_GetError());
  }

  // TODO: HARDEN!
  gm.screen.surface = SDL_GetWindowSurface(gm.screen.window);

  SDL_Log("SDL3 Application Started!");

  game_init(&gm);
  while (!gm.quit)
  {
    events_process(&gm);
    todo_process(&gm);
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
