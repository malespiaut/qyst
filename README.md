# qyst

A Myst-like game engine written in C using SDL3.

## Prerequisites

- SDL3
- SDL3_mixer
- sfsexp (S-Expression library)

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
- `data/`: Configuration and scene definitions (S-Expressions).
- `assets/`: Placeholder for game assets (images, sounds).

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
The logging library (`src/log.h`) is copyright (c) 2020 rxi.
The PL_MPEG library (`src/pl_mpeg.h`) is from [Dominic Szablewski](https://phoboslab.org).
