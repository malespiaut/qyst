<p align="center">
![Qyst logo](/docs/qyst_logo.gif)
</p>

# Qyst

A Myst-like point-and-click video game engine, written in C using SDL3.

The sole purpose of the engine is to display BMP images, play WAV sounds
and MPEG-1 video, and have clickable hotspots -- so that you can make
a point-and-click adventure game if you are creative enough!

![Screenshot of Qyst v0.1](/docs/qyst_v0.1_screenshot.png)

## Prerequisites

- [SDL3](https://github.com/libsdl-org/SDL)
- [SDL3_mixer](https://github.com/libsdl-org/SDL_mixer)
- [sfsexp](https://github.com/mjsottile/sfsexp) (S-Expression library)

## Included libraries

Qyst make uses of

- Dominic Szablewski's (phoboslab) [PL_MPEG](https://github.com/phoboslab/pl_mpeg) for playing MPEG-1 videos
- rxi's [log.c](https://github.com/rxi/log.c) for logging

Make sure to clone the project using `git clone --recursive` to download them as well.

## Editing

The Qyst executable expects, at the very minimum, a `data/` directory containing a `scenes.sexp` S-Expressions file and a `videos/intro.mpg` file.

Modify the `scenes.sexp` file to start making your own game.

## Limitations

The Qyst engine runs at a video resolution of 640×480, and **ONLY** supports `.wav` sounds and musics, `.bmp` images, and `.mpg` MPEG-1 videos.


As Qyst is licensed under the MIT [license](LICENSE), you are free to edit the source code and allieviate those limitations to suit your needs. 


## Building

To build the project, simply run:

```bash
make
```

Or use the provided build script:

```bash
./build.sh
```

## Running

Run the executable:

```bash
./qyst
```

## Project Structure

- `src/`: Source code and headers.
- `data/`: Game assets (images, videos, sounds) and the `scenes.sexp` scene definitions file (S-Expressions).
- `external/`: Single header external libraries

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

The `log.c` library is copyright (c) 2020 rxi, under the MIT license.  
The PL_MPEG library is from [Dominic Szablewski](https://phoboslab.org), under the Public Domain?

The cursors are drawn by [Kab Games](https://kaboff.itch.io/), under [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).
