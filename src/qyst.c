#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sexp.h>

#include <SDL3/SDL.h>

#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

#include "log.c"

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef float f32;
typedef double f64;
typedef uintptr_t uptr;
typedef ptrdiff_t isize;
typedef size_t usize;

#define kWindowTitle "QYST - Development build"
#define kScreenWidth 640
#define kScreenHeight 480

#define kScriptPath "data/script.sexp"
#define kConfigPath "data/config.sexp"

#define kStringLength 256
#define kMaxVariables 256

#define kAutoCenterValue (kScreenWidth*2)
#define kAutoCenterPoint ((SDL_Point){kAutoCenterValue,0})

bool g_quit = false;

enum gamestate_e
{
  eGamestatePlay,
  eGamestateProcess = 1u << 0,
  eGamestateVideo = 1u << 1,
  eGamestateSound = 1u << 2,
  eGamestateText = 1u << 3,
  eGamestateIntro = 1u << 4
};
typedef enum gamestate_e gamestate_t;

enum celltype_e
{
  eStacktypeNULL,
  eStacktypeTarget,
  eStacktypeVideo,
  eStacktypeSound,
  eStacktypeSet,
  eStacktypeText
};
typedef enum celltype_e celltype_t;

typedef struct variable_s variable_t;
struct variable_s
{
  char* name;
  bool value;
};

typedef struct video_conf_s video_conf_t;
struct video_conf_s
{
  char* path;
  bool cutscene;
  SDL_Point position;
};

typedef union celldata_u celldata_t;
union celldata_u
{
  i32 target;
  char* path;
  char* text;
  video_conf_t video_conf;
  variable_t set;
};

typedef struct cell_s cell_t;
struct cell_s
{
  celltype_t type;
  celldata_t data;
};

typedef union vec2i_u vec2i_t;
union vec2i_u
{
  struct
  {
    i32 x;
    i32 y;
  } xy;
  struct
  {
    i32 w;
    i32 h;
  } wh;
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
  char* label;
  char* condition;
  SDL_Rect bounds;
  cell_t stack[32];
  isize stack_size;
};

typedef struct scene_s scene_t;
struct scene_s
{
  i32 id;
  char name[kStringLength];
  SDL_Texture* background;
  hotspot_t* hotspot[8];
  i32 hotspot_count;
  char* music_path;
};

typedef struct audio_s audio_t;
struct audio_s
{
  SDL_AudioSpec spec;
  u8* data;
  u32 data_len;
};

typedef struct video_s video_t;
struct video_s
{
  bool playing;
  bool cutscene;
  plm_t* player;
  SDL_Texture* texture;
  SDL_FRect src_rect;
  SDL_FRect dest_rect;
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
  SDL_Point position;
  SDL_Texture* image[3];
  cursorstate_t state;
};

typedef struct config_s config_t;
struct config_s
{
  char game_title[kStringLength];
  bool intro_skip;
  vec2i_t cursor_size;
  vec2i_t cursor_hot_pixel;
  vec2i_t font_size;
  SDL_Color font_color;
  bool retro_color_mode;
};

typedef struct game_manager_s game_manager_t;
struct game_manager_s
{
  config_t config;
  screen_manager_t screen;
  SDL_AudioDeviceID audio_device;
  SDL_AudioStream* audio_stream;
  SDL_AudioStream* music_stream;
  bool quit;
  double last_time;
  usize scene_count;
  i32 scene_current;

  cursor_t cursor;

  scene_t** scene;
  audio_t sound;
  audio_t music;
  bool music_playing;
  video_t video;

  usize variable_count;
  variable_t variables[kMaxVariables];

  cell_t (*stack)[32];
  isize stack_size;
  isize stack_idx;

  gamestate_t gamestate;

  bool debug;
  sexp_t* script;
};

game_manager_t* g_gm = NULL;

static void audio_decode_callback(plm_t* player, plm_samples_t* samples, void* user);
static void click_process(game_manager_t* gm, i32 x, i32 y);
static void events_process(game_manager_t* gm);
static void game_draw(game_manager_t* gm);
static void game_init(game_manager_t* gm);
static void game_stack_pop(game_manager_t* gm);
static void game_update(game_manager_t* gm);
static void gamestate_process(game_manager_t* gm);
static void hotspot_alloc(scene_t* scene);
static void hotspot_parse(game_manager_t* gm, sexp_t* s, hotspot_t* hs);
static void hotspot_stack_push(hotspot_t* hs, celltype_t type, celldata_t data);
static void hotspots_draw(game_manager_t* gm);
static void intro_play(game_manager_t* gm);
static bool is_autocenter(SDL_Point point);
static bool is_next_element_list(sexp_t* s);
static bool is_next_element_value(sexp_t* s);
static bool is_over_hotspot(hotspot_t* hs, i32 x, i32 y);
static bool is_value(sexp_t* s);
static i32 scene_id_find(game_manager_t* gm, char* scene_name);
static void scene_music_load(scene_t* scene, char* path);
static void scene_init(game_manager_t* gm, sexp_t* s, scene_t* scene);
static usize scenes_count(sexp_t* s);
static void scenes_alloc(game_manager_t* gm);
static sexp_t* script_load(char* path);
static void script_unload(sexp_t* script);
static bool variable_check(game_manager_t* gm, char *name);
static void variable_set(game_manager_t* gm, variable_t set);
static void video_decode_callback(plm_t* player, plm_frame_t* frame, void* user);
static plm_t* video_load(game_manager_t* gm, const char* path);
static void video_play(game_manager_t* gm, plm_t* video, bool cutscene, SDL_Point position);
static void video_update(game_manager_t* gm);
static void video_unload(game_manager_t* gm);

static bool
is_autocenter(SDL_Point point)
{
  return point.x == kAutoCenterValue;
}

static void
video_decode_callback(plm_t* player, plm_frame_t* frame, void* user)
{
  (void)player;

  game_manager_t* gm = (game_manager_t*)user;

  SDL_UpdateYUVTexture(gm->video.texture, NULL, frame->y.data, (i32)frame->y.width, frame->cb.data, (i32)frame->cb.width, frame->cr.data, (i32)frame->cr.width);
}

static void
audio_decode_callback(plm_t* player, plm_samples_t* samples, void* user)
{
  (void)player;

  game_manager_t* gm = (game_manager_t*)user;

  i32 length = (i32)(sizeof(float) * samples->count * 2);
  if (!SDL_PutAudioStreamData(gm->audio_stream, samples->interleaved, length))
  {
    log_error("SDL_PutAudioStreamData() error: %s", SDL_GetError());
  }
}

static bool
variable_check(game_manager_t* gm, char *name)
{
  for (usize i = 0; i < gm->variable_count; ++i)
  {
    if (!strcmp(name, gm->variables[i].name))
    {
      return gm->variables[i].value;
    }
  }
  return false;
}

static bool
condition_check(game_manager_t* gm, char* condition)
{
  if (!condition || !condition[0])
  {
    return true;
  }
  if (condition[0] == '!')
  {
    condition++;
    return !variable_check(gm, condition);
  }
  return variable_check(gm, condition);
}

static void
variable_set(game_manager_t* gm, variable_t set)
{
  for (usize i = 0; i < gm->variable_count; ++i)
  {
    if (!strcmp(set.name, gm->variables[i].name))
    {
      gm->variables[i].value = set.value;
      return;
    }
  }
  if (gm->variable_count >= kMaxVariables)
  {
    log_error("Too many variables set (max=%d).", kMaxVariables);
  }
  else
  {
    gm->variables[gm->variable_count++] = set;
  }
}

static void
hotspots_draw(game_manager_t* gm)
{
  for (i32 i = 0; i < gm->scene[gm->scene_current]->hotspot_count; ++i)
  {
    hotspot_t* hs = gm->scene[gm->scene_current]->hotspot[i];
    SDL_Rect r = hs->bounds;
    if (condition_check(gm, hs->condition))
    {
      SDL_SetRenderDrawColor(gm->screen.renderer, 0, 255, 0, 255);
    }
    else
    {
      SDL_SetRenderDrawColor(gm->screen.renderer, 255, 0, 0, 255);
    }
    SDL_RenderRect(gm->screen.renderer, &(SDL_FRect){.x = (f32)r.x, .y = (f32)r.y, .w = (f32)r.w, .h = (f32)r.h});
  }

  SDL_SetRenderDrawColor(gm->screen.renderer, 0, 0, 0, 255);
}

static void
game_draw(game_manager_t* gm)
{
  SDL_RenderClear(gm->screen.renderer);

  if (!(gm->video.playing && gm->video.cutscene))
  {
    // -- Draw scene background
    SDL_RenderTexture(gm->screen.renderer, gm->scene[gm->scene_current]->background, NULL, NULL);
  }

  if (gm->video.playing)
  {
    SDL_RenderTexture(gm->screen.renderer, gm->video.texture, &gm->video.src_rect, &gm->video.dest_rect);
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
      .x = (f32)(gm->cursor.position.x - gm->config.cursor_hot_pixel.xy.x),
      .y = (f32)(gm->cursor.position.y - gm->config.cursor_hot_pixel.xy.y),
      .w = (f32)gm->config.cursor_size.wh.w,
      .h = (f32)gm->config.cursor_size.wh.h};

    SDL_RenderTexture(gm->screen.renderer, gm->cursor.image[gm->cursor.state], NULL, &cursor_draw_rect);
  }

  SDL_RenderPresent(gm->screen.renderer);
}

static plm_t*
video_load(game_manager_t* gm, const char* path)
{
  plm_t* player = plm_create_with_filename(path);
  if (!player)
  {
    log_fatal("Couldn't open '%s'", path);
    exit(EXIT_FAILURE);
  }

  plm_set_video_decode_callback(player, video_decode_callback, gm);

  if (plm_get_num_audio_streams(player) > 0)
  {
    plm_set_audio_lead_time(player, 4096.0 / (double)plm_get_samplerate(player)); // A bit arbitrary, but it's a lead time I guess...
    plm_set_audio_decode_callback(player, audio_decode_callback, gm);
  }
  else
  {
    plm_set_audio_enabled(player, false);
  }

  return player;
}

static void
video_unload(game_manager_t* gm)
{
  plm_destroy(gm->video.player);
  memset(&gm->video, 0, sizeof(video_t));
}

static void
video_play(game_manager_t* gm, plm_t* video, bool cutscene, SDL_Point position)
{
  i32 video_width = plm_get_width(video);
  i32 video_height = plm_get_height(video);

  if (is_autocenter(position))
  {
    position.x = (kScreenWidth - video_width)/2;
    position.y = (kScreenHeight - video_height)/2;
  }

  gm->video.playing = true;
  gm->video.cutscene = cutscene;
  gm->video.player = video;
  gm->video.texture = SDL_CreateTexture(
    gm->screen.renderer,
    SDL_PIXELFORMAT_IYUV,
    SDL_TEXTUREACCESS_STREAMING,
    video_width,
    video_height);

  gm->video.src_rect.w = (f32)video_width;
  gm->video.src_rect.h = (f32)video_height;

  gm->video.dest_rect = gm->video.src_rect;
  gm->video.dest_rect.x = (f32)position.x;
  gm->video.dest_rect.y = (f32)position.y;

  gm->last_time = (double)SDL_GetTicks() / 1000.0;

  SDL_AudioSpec spec = {
    .format = SDL_AUDIO_F32,
    .channels = 2,
    .freq = plm_get_samplerate(video),
  };

  SDL_SetAudioStreamFormat(gm->audio_stream, &spec, NULL);

  if (cutscene && gm->music_playing)
  {
    SDL_ClearAudioStream(gm->music_stream);
  }
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
    gm->stack = NULL;
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
switch_music(game_manager_t* gm, char* old_path, char* path)
{
  if (!path)
  {
    SDL_ClearAudioStream(gm->music_stream);
    gm->music_playing = false;
  }
  else if (!old_path || strcmp(old_path, path) != 0)
  {
    SDL_ClearAudioStream(gm->music_stream);

    if (gm->music.data)
    {
      SDL_free(gm->music.data);
      gm->music = (audio_t){0};
    }

    if (!SDL_LoadWAV(path, &gm->music.spec, &gm->music.data, &gm->music.data_len))
    {
      log_fatal("Couldn't load .wav file: %s", SDL_GetError());
      gm->music_playing = false;
      return;
    }

    SDL_SetAudioStreamFormat(gm->music_stream, &gm->music.spec, NULL);
    gm->music_playing = true;
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
        video_unload(gm);
        gm->gamestate = eGamestatePlay;
        switch_music(gm, NULL, gm->scene[gm->scene_current]->music_path);
      }
      break;

    case eGamestateProcess:
      {
        // This is some weird shenanigans that I don't fully understand
        // just because I wanted gm->stack to explicitely be a
        // pointer to an array of 32 cell_t objects, and not just a
        // poiter to the first element.
        cell_t c = gm->stack[0][gm->stack_idx];

        switch (c.type)
        {
          case eStacktypeVideo:
            gm->gamestate = eGamestateVideo;
            plm_t* video = video_load(gm, c.data.video_conf.path);
            video_play(gm, video, c.data.video_conf.cutscene, c.data.video_conf.position);
            break;
          case eStacktypeSound:
            gm->gamestate = eGamestateSound;
            if (!SDL_LoadWAV(c.data.path, &gm->sound.spec, &gm->sound.data, &gm->sound.data_len))
            {
              log_error("SDL_LoadWAV() error: %s", SDL_GetError());
              game_stack_pop(gm);
              break;
            }

            SDL_SetAudioStreamFormat(gm->audio_stream, &gm->sound.spec, NULL);
            SDL_ResumeAudioStreamDevice(gm->audio_stream);

            SDL_PutAudioStreamData(gm->audio_stream, gm->sound.data, (i32)gm->sound.data_len);
            SDL_FlushAudioStream(gm->audio_stream);

            SDL_free(gm->sound.data); // TODO: maybe store sound data in memory a bit longer so it can be reused
            gm->sound = (audio_t){0};
            break;
          case eStacktypeText:
            game_stack_pop(gm);
            break;
          case eStacktypeTarget:
            scene_t* current_scene = gm->scene[gm->scene_current];
            scene_t* target_scene = gm->scene[c.data.target];
            switch_music(gm, current_scene->music_path, target_scene->music_path);
            gm->scene_current = c.data.target;
            game_stack_pop(gm);
            break;
          case eStacktypeSet:
            variable_set(gm, c.data.set);
            game_stack_pop(gm);
            break;
          case eStacktypeNULL:
            break;
          default:
            break;
        }
      }
      break;

    case eGamestateVideo:
      if (!gm->video.playing)
      {
        gm->gamestate = eGamestatePlay;

        video_unload(gm);

        game_stack_pop(gm);
      }
      break;
    case eGamestateSound:
      if (SDL_GetAudioStreamAvailable(gm->audio_stream) <= 0)
      {
        // SDL_PauseAudioStreamDevice(gm->audio_stream);
        game_stack_pop(gm);
      }
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
    if (is_over_hotspot(hs, x, y) && condition_check(gm, hs->condition))
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
    log_fatal("Pointers are NULL!");
    exit(EXIT_FAILURE);
  }

  scene->music_path = path;
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
    if (!strcmp(gm->scene[i]->name, scene_name))
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
    log_fatal("Pointer is NULL!");
    exit(EXIT_FAILURE);
  }

  if (hs->stack_size == (32 - 1))
  {
    log_fatal("Stack is already full!");
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
    log_fatal("Pointers are NULL!");
    exit(EXIT_FAILURE);
  }
  if (s->ty != SEXP_LIST)
  {
    log_fatal("Clickbox isn't a list!");
    exit(EXIT_FAILURE);
  }

  while (s)
  {
    if (is_value(s->list->next))
    {
      const char* type = s->list->val;
      const usize str_length = s->list->val_used;

      if (!strncmp(type, "target", str_length))
      {
        hotspot_stack_push(hs, eStacktypeTarget, (celldata_t){.target = scene_id_find(gm, s->list->next->val)});
      }

      else if (!strncmp(type, "video", str_length))
      {
        video_conf_t video_conf = {0};
        //TODO: check?
        video_conf.cutscene = s->list->next->val[0] == 't';
        sexp_t* position = s->list->next->next->list;
        if (position)
        {
          video_conf.position.x = atoi(position->val);
          video_conf.position.y = atoi(position->next->val);
        }
        else
        {
          video_conf.position = kAutoCenterPoint;
        }
        video_conf.path = s->list->next->next->next->val;

        hotspot_stack_push(hs, eStacktypeVideo, (celldata_t){.video_conf = video_conf});
      }

      else if (!strncmp(type, "sound", str_length))
      {
        hotspot_stack_push(hs, eStacktypeSound, (celldata_t){.path = s->list->next->val});
      }

      else if (!strncmp(type, "text", str_length))
      {
        hotspot_stack_push(hs, eStacktypeText, (celldata_t){.text = s->list->next->val});
      }

      else if (!strncmp(type, "set", str_length))
      {
        variable_t set = {
          .name = s->list->next->val,
          .value = s->list->next->next->val[0] == 't'};
        hotspot_stack_push(hs, eStacktypeSet, (celldata_t){.set = set});
      }
    }

    s = s->next;
  }
}

static void
hotspot_alloc(scene_t* scene)
{
  scene->hotspot[scene->hotspot_count] = calloc(1, sizeof *scene->hotspot[scene->hotspot_count]);

  if (!scene->hotspot[scene->hotspot_count])
  {
    perror("ERROR: hotspot_alloc(): Couldn't allocate hotspot memory!");
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

static usize
scenes_count(sexp_t* s)
{
  usize result = 0;

  if (!s)
  {
    log_fatal("Pointers are NULL!");
    exit(EXIT_FAILURE);
  }

  if (s->ty != SEXP_LIST)
  {
    log_fatal("Scenes aren't correctly structured!");
    exit(EXIT_FAILURE);
  }

  for (sexp_t* scene = s->list; scene; scene = scene->next)
  {
    ++result;
  }

  return result;
}

static void
scene_init(game_manager_t* gm, sexp_t* s, scene_t* scene)
{
  if (!s || !scene)
  {
    log_fatal("Pointers are NULL!");
    exit(EXIT_FAILURE);
  }
  if (s->ty != SEXP_LIST)
  {
    log_fatal("Scene isn't a list!");
    exit(EXIT_FAILURE);
  }

  sexp_t* elem = s->list->next;
  while (elem)
  {
    const char* type = elem->list->val;
    const usize str_length = elem->list->val_used;

    if (!strncmp(type, "background", str_length) && is_next_element_value(elem))
    {

      char* path = elem->list->next->val;

      SDL_Surface* background = SDL_LoadBMP(path);
      if (!background)
      {
        log_fatal("SDL_LoadBMP() error: %s", SDL_GetError());
        exit(EXIT_FAILURE);
      }

      scene->background = SDL_CreateTextureFromSurface(gm->screen.renderer, background);
      if (!scene->background)
      {
        log_fatal("SDL_CreateTextureFromSurface() error: %s", SDL_GetError());
        exit(EXIT_FAILURE);
      }

      SDL_DestroySurface(background);
    }

    else if (!strncmp(type, "music", str_length) && is_next_element_value(elem))
    {
      scene_music_load(scene, elem->list->next->val);
    }

    else if (!strncmp(type, "hotspot", str_length) && is_next_element_value(elem))
    {
      hotspot_alloc(scene);

      // -- Hotspot label
      if (!(scene->hotspot[scene->hotspot_count - 1]->label = calloc(elem->list->next->val_used + 1, 1)))
      {
        perror("ERROR: scene_init(): Couldn't allocate memory!");
        exit(EXIT_FAILURE);
      }
      strncpy(scene->hotspot[scene->hotspot_count - 1]->label, elem->list->next->val, elem->list->next->val_used);

      // -- Hotspot condition

      if (!(scene->hotspot[scene->hotspot_count - 1]->condition = calloc(elem->list->next->next->val_used + 1, 1)))
      {
        perror("ERROR: scene_init(): Couldn't allocate memory!");
        exit(EXIT_FAILURE);
      }
      strncpy(scene->hotspot[scene->hotspot_count - 1]->condition, elem->list->next->next->val, elem->list->next->next->val_used);

      // -- Hotspot boundaries
      sexp_t* bounds = elem->list->next->next->next->list;
      scene->hotspot[scene->hotspot_count - 1]->bounds.x = atoi(bounds->val);
      scene->hotspot[scene->hotspot_count - 1]->bounds.y = atoi(bounds->next->val);
      scene->hotspot[scene->hotspot_count - 1]->bounds.w = atoi(bounds->next->next->val);
      scene->hotspot[scene->hotspot_count - 1]->bounds.h = atoi(bounds->next->next->next->val);

      sexp_t* hs_stack = elem->list->next->next->next->next->list;

      hotspot_parse(gm, hs_stack, scene->hotspot[scene->hotspot_count - 1]);
    }
    elem = elem->next;
  }
}

static sexp_t*
script_load(char* path)
{
  if (!path)
  {
    log_fatal("Scenes path isn't properly initialized!");
    exit(EXIT_FAILURE);
  }

  FILE* fp = fopen(path, "r");
  if (!fp)
  {
    char buf[256] = {0};
    snprintf(buf, sizeof(buf), "ERROR: script_load(): Couldn't open %s", path);
    perror(buf);
    exit(EXIT_FAILURE);
  }

  if (fseek(fp, 0, SEEK_END))
  {
    char buf[256] = {0};
    snprintf(buf, sizeof(buf), "ERROR: script_load(): Couldn't seek %s", path);
    perror(buf);
    exit(EXIT_FAILURE);
  }

  usize fsize = (usize)ftell(fp);
  if (fseek(fp, 0, SEEK_SET))
  {
    char buf[256] = {0};
    snprintf(buf, sizeof(buf), "ERROR: script_load(): Couldn't seek %s", path);
    perror(buf);
    exit(EXIT_FAILURE);
  }

  char* buffer = calloc(fsize + 1, 1);

  if (!buffer)
  {
    fclose(fp);
    perror("ERROR: script_load(): Couldn't allocate memory!");
    exit(EXIT_FAILURE);
  }

  usize bytes_read = fread(buffer, 1, fsize, fp);
  buffer[bytes_read] = '\0';
  if (fclose(fp))
  {
    char buf[256] = {0};
    snprintf(buf, sizeof(buf), "ERROR: script_load(): Couldn't close %s", path);
    perror(buf);
    exit(EXIT_FAILURE);
  }

  sexp_t* sexp = parse_sexp(buffer, bytes_read);

  if (!sexp)
  {
    log_fatal("Couldn't parse script");
    free(buffer);
    exit(EXIT_FAILURE);
  }

  free(buffer);

  if (sexp->ty != SEXP_LIST)
  {
    log_fatal("Scenes aren't structured correctly!");
    exit(EXIT_FAILURE);
  }

  return sexp;
}

static void
script_unload(sexp_t* script)
{
  if (script)
  {
    destroy_sexp(script);
  }
}

static void
scenes_alloc(game_manager_t* gm)
{
  gm->scene = calloc(gm->scene_count, sizeof(*gm->scene));

  if (!gm->scene)
  {
    perror("ERROR: scenes_alloc(): Couldn't allocate scene memory!");
    exit(EXIT_FAILURE);
  }

  for (usize i = 0; i < gm->scene_count; ++i)
  {
    gm->scene[i] = calloc(1, sizeof(*gm->scene[i]));

    if (!gm->scene[i])
    {
      perror("ERROR: scenes_alloc(): Couldn't allocate scene memory!");
      exit(EXIT_FAILURE);
    }
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

  if (gm->music_playing && !gm->video.cutscene)
  {
    // Get allways at least a full copy of the music data queued
    if (SDL_GetAudioStreamQueued(gm->music_stream) < ((int)gm->music.data_len))
    {
      SDL_PutAudioStreamData(gm->music_stream, gm->music.data, (int)gm->music.data_len);
    }
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

  gm->gamestate = eGamestateIntro;

  video_play(gm, intro_video, true, kAutoCenterPoint);
}

static void
config_parse(game_manager_t* gm)
{
  sexp_t* config = script_load(kConfigPath);

  sexp_t* s = config->list;

  while (s)
  {
    if (s->list->ty == SEXP_VALUE)
    {
      const char* type = s->list->val;
      const usize str_length = s->list->val_used;

      if (!strncmp(type, "game-title", str_length))
      {
        strncpy(gm->config.game_title, s->list->next->val, s->list->next->val_used);
      }

      else if (!strncmp(type, "skip-intro", str_length))
      {
        if (s->list->next)
        {
          gm->config.intro_skip = *s->list->next->val == 't' ? true : false;
        }
        else
        {
          log_fatal("Missing `skip-intro` value in `data/config.sexp`!");
          exit(EXIT_FAILURE);
        }
      }

      else if (!strncmp(type, "cursor-size", str_length))
      {
        if (s->list->next)
        {
          gm->config.cursor_size.wh.w = atoi(s->list->next->val);
        }
        else
        {
          log_fatal("Missing `cursor-size` width in `data/config.sexp`!");
          exit(EXIT_FAILURE);
        }

        if (s->list->next->next)
        {
          gm->config.cursor_size.wh.h = atoi(s->list->next->next->val);
        }
        else
        {
          log_fatal("Missing `cursor-size` height in `data/config.sexp`!");
          exit(EXIT_FAILURE);
        }
      }

      else if (!strncmp(type, "cursor-hot-pixel", str_length))
      {
        if (s->list->next)
        {
          gm->config.cursor_hot_pixel.xy.x = atoi(s->list->next->val);
        }
        else
        {
          log_fatal("Missing `cursor-hot-pixel` x in `data/config.sexp`!");
          exit(EXIT_FAILURE);
        }

        if (s->list->next->next)
        {
          gm->config.cursor_hot_pixel.xy.y = atoi(s->list->next->next->val);
        }
        else
        {
          log_fatal("Missing `cursor-hot-pixel` y in `data/config.sexp`!");
          exit(EXIT_FAILURE);
        }
      }

      else if (!strncmp(type, "font-size", str_length))
      {
        if (s->list->next)
        {
          gm->config.font_size.wh.w = atoi(s->list->next->val);
        }
        else
        {
          log_fatal("Missing `font-size` width in `data/config.sexp`!");
          exit(EXIT_FAILURE);
        }

        if (s->list->next->next)
        {
          gm->config.font_size.wh.h = atoi(s->list->next->next->val);
        }
        else
        {
          log_fatal("Missing `font-size` height in `data/config.sexp`!");
          exit(EXIT_FAILURE);
        }
      }

      else if (!strncmp(type, "font-color", str_length))
      {
        if (s->list->next)
        {
          gm->config.font_color.r = (u8)atoi(s->list->next->val);
        }
        else
        {
          log_fatal("Missing `font-color` red in `data/config.sexp`!");
          exit(EXIT_FAILURE);
        }

        if (s->list->next->next)
        {
          gm->config.font_color.g = (u8)atoi(s->list->next->next->val);
        }
        else
        {
          log_fatal("Missing `font-color` green in `data/config.sexp`!");
          exit(EXIT_FAILURE);
        }

        if (s->list->next->next->next)
        {
          gm->config.font_color.b = (u8)atoi(s->list->next->next->next->val);
        }
        else
        {
          log_fatal("Missing `font-color` blue in `data/config.sexp`!");
          exit(EXIT_FAILURE);
        }
      }

      else if (!strncmp(type, "256-color-mode", str_length))
      {
        if (s->list->next)
        {
          gm->config.retro_color_mode = *s->list->next->val == 't' ? true : false;
        }
        else
        {
          log_fatal("Missing `256-color-mode` value in `data/config.sexp`!");
          exit(EXIT_FAILURE);
        }
      }
    }

    s = s->next;
  }

  script_unload(config);
}

static void
game_init(game_manager_t* gm)
{
  gm->script = script_load(kScriptPath);
  gm->scene_count = scenes_count(gm->script);

  scenes_alloc(gm);

  // Setting the name of all scenes first
  sexp_t* scene = gm->script->list;
  for (usize i = 0; i < gm->scene_count; ++i)
  {
    strncpy(gm->scene[i]->name, scene->list->val, scene->list->val_used);
    scene = scene->next;
  }

  // Only then `scene_init()` will be able to link hotspots with scenes
  scene = gm->script->list;
  for (usize i = 0; i < gm->scene_count; ++i)
  {
    scene_init(gm, scene, gm->scene[i]);
    scene = scene->next;
  }

  gm->stack_size = 0;
  gm->stack_idx = -1;

  gm->debug = false;

  // -- Cursors
  {
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
        log_fatal("SDL_LoadBMP() error: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
      }

      // -- Color key color
      if (!SDL_SetSurfaceColorKey(cursor_bmp, true, SDL_MapRGB(SDL_GetPixelFormatDetails(cursor_bmp->format), NULL, 0xff, 0x00, 0xff)))
      {
        log_error("SDL_SetSurfaceColorKey() error: %s", SDL_GetError());
      }

      if (!(gm->cursor.image[pairs[i].state] = SDL_CreateTextureFromSurface(gm->screen.renderer, cursor_bmp)))
      {
        log_fatal("SDL_CreateTextureFromSurface() error: %s", SDL_GetError());
        exit(EXIT_FAILURE);
      }
      SDL_DestroySurface(cursor_bmp);
    }
  }
}

int
main(void)
{
  g_gm = calloc(1, sizeof(*g_gm));

  if (!g_gm)
  {
    perror("ERROR: main(): Couldn't allocate game manager memory!");
    exit(EXIT_FAILURE);
  }

  if (!SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO))
  {
    log_fatal("SDL_Init() error: %s\n", SDL_GetError());
    SDL_Quit();
    return SDL_APP_FAILURE;
  }

  g_gm->screen.window = SDL_CreateWindow(kWindowTitle, kScreenWidth, kScreenHeight, 0);
  if (!g_gm->screen.window)
  {
    log_fatal("SDL_CreateWindow() error: %s\n", SDL_GetError());
    SDL_Quit();
    return SDL_APP_FAILURE;
  }

  g_gm->screen.renderer = SDL_CreateRenderer(g_gm->screen.window, NULL);
  if (!g_gm->screen.renderer)
  {
    log_fatal("SDL_CreateRenderer() error: %s\n", SDL_GetError());
    SDL_DestroyWindow(g_gm->screen.window);
    SDL_Quit();
    return SDL_APP_FAILURE;
  }

  g_gm->screen.texture = SDL_CreateTexture(g_gm->screen.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, kScreenWidth, kScreenHeight);
  if (!g_gm->screen.texture)
  {
    log_fatal("SDL_CreateTexture() error: %s\n", SDL_GetError());
    SDL_DestroyWindow(g_gm->screen.window);
    SDL_Quit();
    return SDL_APP_FAILURE;
  }

  g_gm->audio_device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
  if (g_gm->audio_device == 0)
  {
    log_fatal("SDL_OpenAudioDevice() error: %s", SDL_GetError());
    SDL_DestroyWindow(g_gm->screen.window);
    SDL_Quit();
    return SDL_APP_FAILURE;
  }

  g_gm->audio_stream = SDL_CreateAudioStream(NULL, NULL);
  if (!g_gm->audio_stream)
  {
    log_fatal("SDL_OpenAudioDeviceStream() error: %s", SDL_GetError());
    SDL_DestroyWindow(g_gm->screen.window);
    SDL_Quit();
    return SDL_APP_FAILURE;
  }
  else if (!SDL_BindAudioStream(g_gm->audio_device, g_gm->audio_stream))
  {
    log_fatal("SDL_BindAudioStream() error: %s", SDL_GetError());
    SDL_DestroyWindow(g_gm->screen.window);
    SDL_Quit();
    return SDL_APP_FAILURE;
  }

  g_gm->music_stream = SDL_CreateAudioStream(NULL, NULL);
  if (!g_gm->music_stream)
  {
    log_fatal("SDL_CreateAudioStream() error: %s", SDL_GetError());
    SDL_DestroyWindow(g_gm->screen.window);
    SDL_Quit();
    return SDL_APP_FAILURE;
  }
  else if (!SDL_BindAudioStream(g_gm->audio_device, g_gm->music_stream))
  {
    log_fatal("SDL_BindAudioStream() error: %s", SDL_GetError());
    SDL_DestroyWindow(g_gm->screen.window);
    SDL_Quit();
    return SDL_APP_FAILURE;
  }

  // These calls are optional
  // They only serves to make the pixels square and clean when resizing the window
  if (!SDL_SetRenderVSync(g_gm->screen.renderer, SDL_RENDERER_VSYNC_ADAPTIVE))
  {
    log_error("SDL_SetRenderVSync() error: %s", SDL_GetError());
  }
  if (!SDL_SetRenderLogicalPresentation(g_gm->screen.renderer, kScreenWidth, kScreenHeight, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE))
  {
    log_error("SDL_SetRenderLogicalPresentation() error: %s", SDL_GetError());
  }
  if (!SDL_SetDefaultTextureScaleMode(g_gm->screen.renderer, SDL_SCALEMODE_NEAREST))
  {
    log_error("SDL_SetDefaultTextureScaleMode() error: %s", SDL_GetError());
  }

  if (!SDL_HideCursor())
  {
    log_error("SDL_HideCursor() error: %s", SDL_GetError());
  }

  g_gm->screen.surface = SDL_GetWindowSurface(g_gm->screen.window);

  config_parse(g_gm);
  game_init(g_gm);

  if (!g_gm->config.intro_skip)
  {
    intro_play(g_gm);
  }

  while (!g_gm->quit)
  {
    events_process(g_gm);
    gamestate_process(g_gm);
    game_update(g_gm);
    game_draw(g_gm);
  }

exit:
  script_unload(g_gm->script);

  SDL_free(g_gm->music.data);
  SDL_DestroyAudioStream(g_gm->audio_stream);
  SDL_DestroyAudioStream(g_gm->music_stream);
  SDL_CloseAudioDevice(g_gm->audio_device);
  SDL_DestroyRenderer(g_gm->screen.renderer);
  SDL_DestroyWindow(g_gm->screen.window);
  SDL_QuitSubSystem(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  SDL_Quit();

  return SDL_APP_SUCCESS;
}
