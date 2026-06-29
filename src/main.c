#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sexp.h>

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

#include "log.c"

#include "types.h"

#define kWindowTitle "QYST - Development build"
#define kScreenWidth 640
#define kScreenHeight 480

bool g_quit = false;

enum gamestate_e
{
  eGamestatePlay = 1u << 0,
  eGamestateProcess = 1u << 1,
  eGamestateVideo = 1u << 2,
  eGamestateSound = 1u << 3,
  eGamestateText = 1u << 4,
  eGamestateIntro = 1u << 5
};
typedef enum gamestate_e gamestate_t;

enum celltype_e
{
  eStacktypeNULL,
  eStacktypeTarget,
  eStacktypeVideo,
  eStacktypeSound,
  eStacktypeText
};
typedef enum celltype_e celltype_t;

typedef union celldata_u celldata_t;
union celldata_u
{
  i32 target;
  char* path;
  char* text;
};

typedef struct cell_s cell_t;
struct cell_s
{
  celltype_t type;
  celldata_t data;
};

typedef struct screen_manager_s screen_manager_t;
struct screen_manager_s
{
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Texture* texture;
  SDL_Surface* surface;
};

typedef struct hotspot_s hotspot_t;
struct hotspot_s
{
  SDL_Rect bounds;
  cell_t stack[32];
  isize stack_size;
};

typedef struct scene_s scene_t;
struct scene_s
{
  i32 id;
  char* name;
  SDL_Texture* background;
  hotspot_t* hotspot[8];
  i32 hotspot_count;
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

enum cursorstate_e
{
  eCursorstateIdle,
  eCursorstateClick,
  eCursorstateWait
};
typedef enum cursorstate_e cursorstate_t;

typedef struct cursor_s cursor_t;
struct cursor_s
{
  SDL_Point hot;
  SDL_Point position;
  SDL_Texture* image[3];
  cursorstate_t state;
};

typedef struct game_manager_s game_manager_t;
struct game_manager_s
{
  screen_manager_t screen;
  bool quit;
  double last_time;
  i32 scene_count;
  i32 scene_current;

  cursor_t cursor;

  scene_t** scene;
  video_t video;

  cell_t (*stack)[32];
  isize stack_size;
  isize stack_idx;

  gamestate_t gamestate;

  bool debug;
};

static void click_process(game_manager_t* gm, i32 x, i32 y);
static void events_process(game_manager_t* gm);
static void game_draw(game_manager_t* gm);
static void game_init(game_manager_t* gm);
static void game_stack_pop(game_manager_t* gm);
static void game_update(game_manager_t* gm);
static void gamestate_process(game_manager_t* gm);
static void hotspot_init(scene_t* scene);
static void hotspot_parse(game_manager_t* gm, sexp_t* s, hotspot_t* hs);
static void hotspot_stack_push(hotspot_t* hs, celltype_t type, celldata_t data);
static void hotspots_draw(game_manager_t* gm);
static void intro_play(game_manager_t* gm);
static bool is_next_element_list(sexp_t* s);
static bool is_next_element_value(sexp_t* s);
static bool is_over_hotspot(hotspot_t* hs, i32 x, i32 y);
static bool is_valid_hotspot(hotspot_t* hs);
static bool is_value(sexp_t* s);
static i32 scene_id_find(game_manager_t* gm, char* scene_name);
static void scene_music_load(scene_t* scene, char* path);
static void scene_parse(game_manager_t* gm, sexp_t* s, scene_t* scene);
static i32 scenes_count(sexp_t* s);
static void scenes_init(game_manager_t* gm, sexp_t* scene);
static sexp_t* scenes_load(char* path);
static void video_decode_callback(plm_t* player, plm_frame_t* frame, void* user);
static plm_t* video_load(game_manager_t* gm, const char* path);
static void video_play(game_manager_t* gm, plm_t* video);
static void video_update(game_manager_t* gm);

static void
video_decode_callback(plm_t* player, plm_frame_t* frame, void* user)
{
  (void)player;

  game_manager_t* gm = (game_manager_t*)user;

  SDL_UpdateYUVTexture(gm->video.texture, NULL, frame->y.data, (i32)frame->y.width, frame->cb.data, (i32)frame->cb.width, frame->cr.data, (i32)frame->cr.width);
}

static void
hotspots_draw(game_manager_t* gm)
{
  for (i32 i = 0; i < gm->scene[gm->scene_current]->hotspot_count; ++i)
  {
    hotspot_t* hs = gm->scene[gm->scene_current]->hotspot[i];
    SDL_Rect r = hs->bounds;
    SDL_SetRenderDrawColor(gm->screen.renderer, 255, 0, 0, 255);
    SDL_RenderRect(gm->screen.renderer, &(SDL_FRect){.x = (f32)r.x, .y = (f32)r.y, .w = (f32)r.w, .h = (f32)r.h});
  }

  SDL_SetRenderDrawColor(gm->screen.renderer, 0, 0, 0, 255);
}

static void
game_draw(game_manager_t* gm)
{
  SDL_RenderClear(gm->screen.renderer);

  // -- Draw scene background
  SDL_RenderTexture(gm->screen.renderer, gm->scene[gm->scene_current]->background, NULL, NULL);

  if (gm->video.playing)
  {
    i32 video_width = plm_get_width(gm->video.player);
    i32 video_height = plm_get_height(gm->video.player);

    if ((video_width < kScreenWidth) && (video_height < kScreenHeight))
    {

      SDL_RenderTexture(gm->screen.renderer, gm->video.texture, &gm->video.rectangle, &(SDL_FRect){.x = (f32)((kScreenWidth - video_width) / 2), .y = (f32)((kScreenHeight - video_height) / 2), .w = (f32)video_width, .h = (f32)video_height});
    }
    else
    {
      SDL_RenderTexture(gm->screen.renderer, gm->video.texture, &gm->video.rectangle, &gm->video.rectangle);
    }
  }
  else
  {
    if (gm->debug)
    {
      hotspots_draw(gm);
    }
  }

  // -- Draw cursor
  {
    SDL_FRect cursor_draw_rect = {
      .x = (f32)(gm->cursor.position.x - gm->cursor.hot.x),
      .y = (f32)(gm->cursor.position.y - gm->cursor.hot.y),
      .w = 32,
      .h = 32};

    SDL_RenderTexture(gm->screen.renderer, gm->cursor.image[gm->cursor.state], NULL, &cursor_draw_rect);
  }

  SDL_RenderPresent(gm->screen.renderer);
}

static plm_t*
video_load(game_manager_t* gm, const char* path)
{
  plm_t* result = plm_create_with_filename(path);
  if (!result)
  {
    log_error("Couldn't open '%s'", path);
    exit(EXIT_FAILURE);
  }

  plm_set_audio_enabled(result, false);
  plm_set_video_decode_callback(result, video_decode_callback, gm);

  return result;
}

static void
video_play(game_manager_t* gm, plm_t* video)
{
  gm->video.playing = true;
  gm->video.player = video;
  gm->video.texture = SDL_CreateTexture(
    gm->screen.renderer,
    SDL_PIXELFORMAT_IYUV,
    SDL_TEXTUREACCESS_STREAMING,
    plm_get_width(video),
    plm_get_height(video));

  gm->video.rectangle.w = (f32)plm_get_width(video);
  gm->video.rectangle.h = (f32)plm_get_height(video);

  gm->last_time = (double)SDL_GetTicks() / 1000.0;
}

static void
video_update(game_manager_t* gm)
{
  if (plm_has_ended(gm->video.player))
  {
    plm_rewind(gm->video.player);
    gm->video.playing = false;
  }

  double current_time = (double)SDL_GetTicks() / 1000.0;
  double elapsed_time = current_time - gm->last_time;
  if (elapsed_time > 1.0 / 30.0)
  {
    elapsed_time = 1.0 / 30.0;
  }
  gm->last_time = current_time;

  plm_decode(gm->video.player, elapsed_time);
}

static void
game_stack_pop(game_manager_t* gm)
{
  if ((gm->stack_idx == gm->stack_size - 1) || (gm->stack_idx == 32 - 1))
  {
    gm->stack = nullptr;
    gm->stack_size = 0;
    gm->stack_idx = -1;

    gm->gamestate = eGamestatePlay;
  }
  else
  {
    gm->stack_idx++;

    gm->gamestate = eGamestateProcess;
  }
}

static void
gamestate_process(game_manager_t* gm)
{
  switch (gm->gamestate)
  {
    case eGamestateIntro:
      if (!gm->video.playing)
      {
        gm->gamestate = eGamestatePlay;
      }
      break;

    case eGamestateProcess:
      // This is some weird shenanigans that I don't fully understand
      // just because I wanted gm->stack to explicitely be a
      // pointer to an array of 32 cell_t objects, and not just a
      // poiter to the first element.
      cell_t c = gm->stack[0][gm->stack_idx];

      switch (c.type)
      {
        case eStacktypeVideo:
          gm->gamestate = eGamestateVideo;
          plm_t* video = video_load(gm, c.data.path);
          video_play(gm, video);
          break;
        case eStacktypeSound:
          game_stack_pop(gm);
          break;
        case eStacktypeText:
          game_stack_pop(gm);
          break;
        case eStacktypeTarget:
          gm->scene_current = c.data.target;
          game_stack_pop(gm);
          break;
        case eStacktypeNULL:
          break;
        default:
          break;
      }
      break;

    case eGamestateVideo:
      if (!gm->video.playing)
      {
        gm->gamestate = eGamestatePlay;

        game_stack_pop(gm);
      }
      break;
    case eGamestateSound:
      break;
    case eGamestateText:
      break;
    case eGamestatePlay:
      break;
    default:
      break;
  }
}

static bool
is_over_hotspot(hotspot_t* hs, i32 x, i32 y)
{
  SDL_Rect r = hs->bounds;
  if ((x >= r.x) && (x <= (r.x + r.w)) &&
      (y >= r.y) && (y <= (r.y + r.h)))
  {
    return true;
  }
  else
  {
    return false;
  }
}

static void
click_process(game_manager_t* gm, i32 x, i32 y)
{
  for (i32 i = 0; i < gm->scene[gm->scene_current]->hotspot_count; ++i)
  {
    hotspot_t* hs = gm->scene[gm->scene_current]->hotspot[i];
    if (is_over_hotspot(hs, x, y))
    {
      gm->stack = &hs->stack;
      gm->stack_size = hs->stack_size;
      gm->stack_idx = 0;

      gm->gamestate = eGamestateProcess;

      return;
    }
  }
}

static void
events_process(game_manager_t* gm)
{
  SDL_Event event = {0};
  while (SDL_PollEvent(&event))
  {
    switch (event.type)
    {
      case SDL_EVENT_QUIT:
        gm->quit = true;
        break;

      case SDL_EVENT_KEY_DOWN:
        if (event.key.key == SDLK_F1)
        {
          gm->debug = !gm->debug;
          log_debug("Debug mode %s", gm->debug ? "ACTIVATED" : "DEACTIVATED");
          if (!gm->debug)
          {
            SDL_SetWindowTitle(gm->screen.window, kWindowTitle);
          }
        }
        if (event.key.key == SDLK_ESCAPE)
        {
          gm->quit = true;
        }
        break;
      case SDL_EVENT_MOUSE_BUTTON_UP:
        if (gm->gamestate == eGamestatePlay)

        {
          click_process(gm, (i32)event.button.x, (i32)event.button.y);
        }
        break;
      default:
        break;
    }
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

  scene->music_path = path;
}

static bool
is_valid_hotspot(hotspot_t* hs)
{
  return (
    (hs->bounds.x >= 0) && (hs->bounds.x <= kScreenWidth) &&
    (hs->bounds.y >= 0) && (hs->bounds.y <= kScreenHeight) &&
    ((hs->bounds.x + hs->bounds.w) >= 0) && ((hs->bounds.x + hs->bounds.w) <= kScreenWidth) &&
    ((hs->bounds.y + hs->bounds.h) >= 0) && ((hs->bounds.y + hs->bounds.h) <= kScreenHeight));
}

static bool
is_value(sexp_t* s)
{
  return (s && s->ty == SEXP_VALUE);
}

static i32
scene_id_find(game_manager_t* gm, char* scene_name)
{
  for (i32 i = 0; i < gm->scene_count; ++i)
  {
    if (gm->scene[i]->name && !strcmp(gm->scene[i]->name, scene_name))
    {
      return (i32)i;
    }
  }
  log_error("Target \"%s\" doesn't exist!", scene_name);
  return -1;
}

static void
hotspot_stack_push(hotspot_t* hs, celltype_t type, celldata_t data)
{
  if (!hs)
  {
    log_error("Pointer is NULL!");
    exit(EXIT_FAILURE);
  }

  if (hs->stack_size == (32 - 1))
  {
    log_error("Stack is already full!");
    exit(EXIT_FAILURE);
  }

  hs->stack[hs->stack_size].type = type;
  hs->stack[hs->stack_size].data = data;
  hs->stack_size++;
}

static void
hotspot_parse(game_manager_t* gm, sexp_t* s, hotspot_t* hs)
{
  if (!s || !hs)
  {
    log_error("Pointers are NULL!");
    exit(EXIT_FAILURE);
  }
  if (s->ty != SEXP_LIST)
  {
    log_error("Clickbox isn't a list!");
    exit(EXIT_FAILURE);
  }

  while (s)
  {
    if (is_value(s->list->next))
    {
      const char* type = s->list->val;

      if (!strncmp(type, "target", 6))
      {
        hotspot_stack_push(hs, eStacktypeTarget, (celldata_t){.target = scene_id_find(gm, s->list->next->val)});
      }

      else if (!strncmp(type, "video", 5))
      {
        hotspot_stack_push(hs, eStacktypeVideo, (celldata_t){.path = s->list->next->val});
      }

      else if (!strncmp(type, "sound", 5))
      {
        hotspot_stack_push(hs, eStacktypeSound, (celldata_t){.path = s->list->next->val});
      }

      else if (!strncmp(type, "text", 4))
      {
        hotspot_stack_push(hs, eStacktypeText, (celldata_t){.text = s->list->next->val});
      }
    }

    s = s->next;
  }
}

static void
hotspot_init(scene_t* scene)
{
  scene->hotspot[scene->hotspot_count] = calloc(1, sizeof *scene->hotspot[scene->hotspot_count]);

  if (!scene->hotspot[scene->hotspot_count])
  {
    perror("ERROR: hotspot_init(): Couldn't allocate hotspot memory!");
    exit(EXIT_FAILURE);
  }
  scene->hotspot_count++;
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

  sexp_t* cursor = s->list->next;
  while (cursor)
  {
    const char* type = cursor->list->val;

    if (!strncmp(type, "background", 10) && is_next_element_value(cursor))
    {

      char* path = cursor->list->next->val;

      SDL_Surface* background = SDL_LoadBMP(path);
      if (!background)
      {
        log_error("SDL_LoadBMP couldn't load %s", path);
        exit(EXIT_FAILURE);
      }

      scene->background = SDL_CreateTextureFromSurface(gm->screen.renderer, background);
      if (!scene->background)
      {
        log_error("Can't create a texture from an existing surface: %s", SDL_GetError());
        exit(EXIT_FAILURE);
      }

      SDL_DestroySurface(background);
    }

    else if (!strncmp(type, "music", 5) && is_next_element_value(cursor))
    {
      scene_music_load(scene, cursor->list->next->val);
    }

    else if (!strncmp(type, "hotspot", 7) && is_next_element_value(cursor))
    {
      sexp_t* cursor_hs = cursor->list->next;

      hotspot_init(scene);
      scene->hotspot[scene->hotspot_count - 1]->bounds.x = atoi(cursor->list->next->val);
      scene->hotspot[scene->hotspot_count - 1]->bounds.y = atoi(cursor->list->next->next->val);
      scene->hotspot[scene->hotspot_count - 1]->bounds.w = atoi(cursor->list->next->next->next->val);
      scene->hotspot[scene->hotspot_count - 1]->bounds.h = atoi(cursor->list->next->next->next->next->val);

      sexp_t* cursor_hs_list = cursor->list->next->next->next->next->next->list;

      hotspot_parse(gm, cursor_hs_list, scene->hotspot[scene->hotspot_count - 1]);

      cursor_hs = cursor_hs->next;
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

  usize fsize = (usize)ftell(fp);
  if (fseek(fp, 0, SEEK_SET))
  {
    char buf[256] = {0};
    snprintf(buf, sizeof(buf), "ERROR: scenes_load(): Couldn't seek %s", path);
    perror(buf);
    exit(EXIT_FAILURE);
  }

  char* buffer = calloc(fsize + 1, 1);

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

  sexp_t* sexp = parse_sexp(buffer, bytes_read);

  if (!sexp)
  {
    log_error("Couldn't parse scenes");
    free(buffer);
    exit(EXIT_FAILURE);
  }

  free(buffer);

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
  gm->scene = calloc((usize)gm->scene_count, sizeof(*gm->scene));

  if (!gm->scene)
  {
    perror("ERROR: scenes_init(): Couldn't allocate scene memory!");
    exit(EXIT_FAILURE);
  }

  for (i32 i = 0; i < gm->scene_count; ++i)
  {
    gm->scene[i] = calloc(1, sizeof(*gm->scene[i]));

    if (!gm->scene[i])
    {
      perror("ERROR: scenes_init(): Couldn't allocate scene memory!");
      exit(EXIT_FAILURE);
    }

    gm->scene[i]->hotspot_count = 0;
  }

  for (i32 i = 0; i < gm->scene_count; ++i)
  {
    gm->scene[i]->name = scene->list->val;
    scene = scene->next;
  }
}

static void
game_update(game_manager_t* gm)
{
  SDL_FRect mouse_fposition = {0};
  SDL_GetMouseState(&mouse_fposition.x, &mouse_fposition.y);
  gm->cursor.position.x = (i32)mouse_fposition.x;
  gm->cursor.position.y = (i32)mouse_fposition.y;

  {
    bool hot_cursor = false;

    for (i32 i = 0; i < gm->scene[gm->scene_current]->hotspot_count; ++i)
    {
      hotspot_t* hs = gm->scene[gm->scene_current]->hotspot[i];
      if (is_over_hotspot(hs, gm->cursor.position.x, gm->cursor.position.y))
      {
        hot_cursor = true;
        break;
      }
    }

    if (gm->video.playing)
    {
      gm->cursor.state = eCursorstateWait;
    }
    else if (hot_cursor)
    {
      gm->cursor.state = eCursorstateClick;
    }
    else
    {
      gm->cursor.state = eCursorstateIdle;
    }
  }

  if (gm->debug)
  {
    char buffer[256] = {0};
    sprintf(buffer, "DEBUG MODE - %d hotspots - Cursor (%d;%d)", gm->scene[gm->scene_current]->hotspot_count, gm->cursor.position.x, gm->cursor.position.y);
    SDL_SetWindowTitle(gm->screen.window, buffer);
  }

  switch (gm->gamestate)
  {
    case eGamestateIntro:
    case eGamestateVideo:
      if (gm->video.playing)
      {
        video_update(gm);
      }
      break;
    case eGamestateSound:
      break;
    case eGamestateText:
      break;
    case eGamestateProcess:
      break;
    case eGamestatePlay:
      break;
    default:
      break;
  }
}

static void
intro_play(game_manager_t* gm)
{
  plm_t* intro_video = video_load(gm, "data/videos/intro.mpg");
  video_play(gm, intro_video);
}

static void
game_init(game_manager_t* gm)
{

  sexp_t* scenes = scenes_load("data/scenes.sexp");
  gm->scene_count = scenes_count(scenes);

  sexp_t* scene = scenes->list;
  scenes_init(gm, scene);
  for (i32 i = 0; i < gm->scene_count; ++i)
  {
    scene_parse(gm, scene, gm->scene[i]);
    scene = scene->next;
  }

  gm->stack_size = 0;
  gm->stack_idx = -1;

  gm->gamestate = eGamestateIntro;

  gm->debug = false;

  // -- Cursors
  {

    // -- Cursor hot spot
    gm->cursor.hot.x = 12;
    gm->cursor.hot.y = 6;

    typedef struct cursortuple_s cursortuple_t;
    struct cursortuple_s
    {
      cursorstate_t state;
      char* bmp_path;
    };

    cursortuple_t pairs[3] =
      {
        (cursortuple_t){.state = eCursorstateIdle, .bmp_path = "data/cursors/idle.bmp"},
        (cursortuple_t){.state = eCursorstateClick, .bmp_path = "data/cursors/click.bmp"},
        (cursortuple_t){.state = eCursorstateWait, .bmp_path = "data/cursors/wait.bmp"}};

    for (usize i = 0; i < 3; ++i)
    {
      SDL_Surface* cursor_bmp;
      if (!(cursor_bmp = SDL_LoadBMP(pairs[i].bmp_path)))
      {
        fprintf(stderr, "Unable to load image %s! SDL_image error: %s\n", pairs[i].bmp_path, SDL_GetError());
        exit(EXIT_FAILURE);
      }

      // -- Color key color
      if (!SDL_SetSurfaceColorKey(cursor_bmp, true, SDL_MapRGB(SDL_GetPixelFormatDetails(cursor_bmp->format), NULL, 0xff, 0x00, 0xff)))
      {
        fprintf(stderr, "Unable to color key! SDL error: %s", SDL_GetError());
      }

      if (!(gm->cursor.image[pairs[i].state] = SDL_CreateTextureFromSurface(gm->screen.renderer, cursor_bmp)))
      {
        log_error("Can't create a color cursor: %s", SDL_GetError());
        exit(EXIT_FAILURE);
      }
      SDL_DestroySurface(cursor_bmp);
    }
  }
}

int
main(void)
{

  game_manager_t gm = {0};

  if (!SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO))
  {
    SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Can't initialize the SDL library: %s\n", SDL_GetError());
    SDL_Quit();
    return SDL_APP_FAILURE;
  }

  gm.screen.window = SDL_CreateWindow(kWindowTitle, kScreenWidth, kScreenHeight, 0);
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

  if (!SDL_HideCursor())
  {
    fprintf(stderr, "SDL_HideCursor() failed: %s", SDL_GetError());
  }

  gm.screen.surface = SDL_GetWindowSurface(gm.screen.window);

  game_init(&gm);

  intro_play(&gm);

  while (!gm.quit)
  {
    events_process(&gm);
    gamestate_process(&gm);
    game_update(&gm);
    game_draw(&gm);
  }

  SDL_DestroyRenderer(gm.screen.renderer);
  SDL_DestroyWindow(gm.screen.window);
  SDL_QuitSubSystem(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  SDL_Quit();

  return SDL_APP_SUCCESS;
}
