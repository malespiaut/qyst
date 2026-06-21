# QYST technical documentation

## Medias

QYST uses standard and basic medias to avoid code overhead, and also because
“perfect is the enemy of good”: WAV for sounds and music, BMP for still images,
and MPEG-1 for video.

Those formats where used back in the day.

Althought BMP and WAV are raw formats, QYST v1.1 plan to support ZIP archives.
That way, all of the data will be compressed.

## Videos

Qyst uses MPEG-1 videos, because it's a public domain codec, that is easy to use
with FFMpeg to encode videos, and Dominic Szablewski's PL_MPEG to decode them.

However, encoding MPEG-1 videos to reproduce the look of old point-and-click
games, such as Urban Runner (1996), can be difficult.

One reason is that PC games after used codecs like Cinepak, Intel Indeo 3,
or Sorenson Vector Quantizer 1, which allowed low framerate (Urban Runner
videos play at 12.5 fps) while MPEG-1 only allows 23.976, 24, 25, 29.97, 30,
50, 59.94, and 60 fps (see https://stackoverflow.com/a/75663483).

Encoding an MPEG-1 video with a frame rate below 23.976 is therefore impossible,
**AND**, a lower count of frames per seconds leads to higher compression visual
artifacts.

A good compromise is to have FFMpeg process the input video as 10 FPS, but
encode a 60 FPS MPEG-1 video.

```
ffmpeg -i video.mp4 \
  -vf "scale=320:-1,fps=10,format=yuv420p" \
  -c:v mpeg1video \
  -r 60 \
  output.mpg
```

If your video is shot in a different aspect ratio than 4:3, the following option
will automatically crop it on proportions at the center:

`-vf "crop=in_h*4/3:in_h:(in_w-in_h*4/3)/2:0,scale=160:-1,fps=10,format=yuv420p"`

therefore:

```
ffmpeg -i video.mp4 \
  -vf "crop=in_h*4/3:in_h:(in_w-in_h*4/3)/2:0,scale=320:-1,fps=10,format=yuv420p"
  -c:v mpeg1video \
  -r 60 \
  output.mpg
```

One additional reason videos from those old games look the way they do is
because they use nearest neighbor for scaling the image down.

This can be achieved using the parameters `-sws_flags neighbor+full_chroma_int`,
therefore:

```
ffmpeg -i video.mp4 \
  -sws_flags neighbor+full_chroma_int \
  -vf "crop=in_h*4/3:in_h:(in_w-in_h*4/3)/2:0,scale=320:-1,fps=10,format=yuv420p"
  -c:v mpeg1video \
  -r 60 \
  output.mpg
```

