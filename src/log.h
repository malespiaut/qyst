/**
 * Copyright (c) 2020 rxi
 * Copyright (c) 2026 Marc-Alexandre Espiaut
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license.
 */

#ifndef LOG_H
#define LOG_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#define LOG_VERSION "0.1.0"

#define MAX_CALLBACKS 32

typedef struct
{
  va_list ap;
  const char* fmt;
  const char* file;
  const char* func;
  void* udata;
  int line;
  int level;
} log_Event;

typedef void (*log_LogFn)(log_Event* ev);
typedef void (*log_LockFn)(bool lock, void* udata);

enum
{
  LOG_TRACE,
  LOG_DEBUG,
  LOG_INFO,
  LOG_WARN,
  LOG_ERROR,
  LOG_FATAL
};

typedef struct
{
  log_LogFn fn;
  void* udata;
  int level;
} Callback;

static struct
{
  void* udata;
  log_LockFn lock;
  int level;
  bool quiet;
  Callback callbacks[MAX_CALLBACKS];
} L;

static const char* level_strings[] = {
  "TRACE",
  "DEBUG",
  "INFO",
  "WARN",
  "ERROR",
  "FATAL"};

#ifndef LOG_COLOR_DISABLE
static const char* level_colors[] = {
  "\x1b[94m",
  "\x1b[36m",
  "\x1b[32m",
  "\x1b[33m",
  "\x1b[31m",
  "\x1b[35m"};
#endif

#define log_trace(...) log_log(LOG_TRACE, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define log_debug(...) log_log(LOG_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define log_info(...) log_log(LOG_INFO, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define log_warn(...) log_log(LOG_WARN, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define log_error(...) log_log(LOG_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define log_fatal(...) log_log(LOG_FATAL, __FILE__, __LINE__, __func__, __VA_ARGS__)

static void
stdout_callback(log_Event* ev)
{
#ifdef LOG_COLOR_DISABLE
  fprintf(ev->udata, "%-5s %s:%d: %s: ", level_strings[ev->level], ev->file, ev->line, ev->func);
#else
  fprintf(ev->udata, "%s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m %s: ", level_colors[ev->level], level_strings[ev->level], ev->file, ev->line, ev->func);
#endif
  vfprintf(ev->udata, ev->fmt, ev->ap);
  fprintf(ev->udata, "\n");
  fflush(ev->udata);
}

static void
file_callback(log_Event* ev)
{
  fprintf(ev->udata, "%-5s %s:%d: %s: ", level_strings[ev->level], ev->file, ev->line, ev->func);
  vfprintf(ev->udata, ev->fmt, ev->ap);
  fprintf(ev->udata, "\n");
  fflush(ev->udata);
}

static void
lock(void)
{
  if (L.lock)
  {
    L.lock(true, L.udata);
  }
}

static void
unlock(void)
{
  if (L.lock)
  {
    L.lock(false, L.udata);
  }
}

static const char*
log_level_string(int level)
{
  return level_strings[level];
}

static void
log_set_lock(log_LockFn fn, void* udata)
{
  L.lock = fn;
  L.udata = udata;
}

static void
log_set_level(int level)
{
  L.level = level;
}

static void
log_set_quiet(bool enable)
{
  L.quiet = enable;
}

static int
log_add_callback(log_LogFn fn, void* udata, int level)
{
  for (int i = 0; i < MAX_CALLBACKS; i++)
  {
    if (!L.callbacks[i].fn)
    {
      L.callbacks[i] = (Callback){fn, udata, level};
      return 0;
    }
  }
  return -1;
}

static int
log_add_fp(FILE* fp, int level)
{
  return log_add_callback(file_callback, fp, level);
}

static void
init_event(log_Event* ev, void* udata)
{
  ev->udata = udata;
}

static void
log_log(int level, const char* file, int line, const char* func, const char* fmt, ...)
{
  log_Event ev = {
    .fmt = fmt,
    .file = file,
    .func = func,
    .line = line,
    .level = level,
  };

  lock();

  if (!L.quiet && level >= L.level)
  {
    init_event(&ev, stderr);
    va_start(ev.ap, fmt);
    stdout_callback(&ev);
    va_end(ev.ap);
  }

  for (int i = 0; i < MAX_CALLBACKS && L.callbacks[i].fn; i++)
  {
    Callback* cb = &L.callbacks[i];
    if (level >= cb->level)
    {
      init_event(&ev, cb->udata);
      va_start(ev.ap, fmt);
      cb->fn(&ev);
      va_end(ev.ap);
    }
  }

  unlock();
}

#endif
