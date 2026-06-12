#ifndef S8_H
#define S8_H

// HELPER
//
//
//

#include "log.h"
#include "types.h"

#define sizeof(x) (usize)sizeof(x)
#define alignof(x) (usize) _Alignof(x)
#define countof(a) (sizeof(a) / sizeof(*(a)))
#define lengthof(s) (countof(s) - 1)
#define new(a, t, n) (t*)alloc(a, sizeof(t), alignof(t), n)

// static void osfail(void);
// static i32 osread(i32, u8*, i32);
// static bool oswrite(i32, u8*, i32);

// static void
// oom(void)
//{
// static const u8 msg[] = "out of memory\n";
// oswrite(2, (u8*)msg, lengthof(msg));
// osfail();

//}

typedef struct
{
  u8* beg;
  u8* end;
} arena;

static u8*
alloc(arena* a, usize objsize, usize align, usize count)
{
  usize avail = a->end - a->beg;
  usize padding = -(uptr)a->beg & (align - 1);
  if (count > (avail - padding) / objsize)
  {
    // oom();
    log_error("Out of memory!");
    exit(EXIT_FAILURE);
  }
  usize total = count * objsize;
  u8* p = a->beg + padding;
  a->beg += padding + total;
  for (usize i = 0; i < total; i++)
  {
    p[i] = 0;
  }
  return p;
}

static void
copy(u8* restrict dst, u8* restrict src, usize length)
{
  for (usize i = 0; i < length; i++)
  {
    dst[i] = src[i];
  }
}

typedef struct
{
  // includes null terminator (for bad functions)
  u8* data;
  // excludes null terminator (for copying memory)
  isize length;
} String;

#define S(s)              \
  (string_t)              \
  {                       \
    (u8*)(s), lengthof(s) \
  }
typedef struct
{
  u8* buf;
  usize length;
} string_t;

static string_t
string_span(u8* beg, u8* end)
{
  string_t s = {0};
  s.buf = beg;
  s.length = end - beg;
  return s;
}

static bool
string_equal(string_t a, string_t b)
{
  if (a.length != b.length)
  {
    return 0;
  }
  for (usize i = 0; i < a.length; i++)
  {
    if (a.buf[i] != b.buf[i])
    {
      return 0;
    }
  }
  return 1;
}

static usize
string_cmp(string_t a, string_t b)
{
  usize length = a.length < b.length ? a.length : b.length;
  for (usize i = 0; i < length; i++)
  {
    usize d = a.buf[i] - b.buf[i];
    if (d)
    {
      return d;
    }
  }
  return a.length - b.length;
}

static u32
string_hash(string_t s)
{
  u64 h = 0x100;
  for (usize i = 0; i < s.length; i++)
  {
    h ^= s.buf[i];
    h *= 1111111111111111111u;
  }
  return (h ^ h >> 32) & (u32)-1;
}

static string_t
string_clone(arena* a, string_t s)
{
  string_t c = {0};
  c.buf = new (a, u8, s.length);
  c.length = s.length;
  copy(c.buf, s.buf, s.length);
  return c;
}

#endif // S8_H
