[English](README.md)        [Русский](README_Russian.md)        [中文](README_Chinese.md)        [हिन्दी](README_Hindi.md)        [Español](README_Spanish.md)        [Français](README_French.md)        [Deutsch](README_German.md)        [Português](README_Portuguese.md)        [日本語](README_Japanese.md)        [Bahasa Indonesia](README_Indonesian.md)

[![Blitzkrieg II Trailer](Blitzkrieg_2.png)](https://www.youtube.com/watch?v=Cw8rA2hvDGg)

The computer game [Blitzkrieg 2](https://en.wikipedia.org/wiki/Blitzkrieg_2) is the second installment of the legendary series of real-time strategy war games, developed by [Nival Interactive](http://nival.com/) and released in 2005.

The game is still available on [Steam](https://store.steampowered.com/app/313500/Blitzkrieg_2_Anthology) and [GOG.com](https://www.gog.com/en/game/blitzkrieg_2_anthology).

In 2025, the game's source code was released under a [special license](LICENSE.md) that prohibits commercial use but is completely open for the game's community, education and research.
Please review the terms of the [license agreement](LICENSE.md) carefully before using it.

## Original Tech stack

- **Game engine**: Custom 3D engine, mostly written in C++
- **Scripting language**: Lua
- **Animation**: Granny Animation (RAD Game Tools) ⚠️ *Commercial license - not included*
- **Video**: Bink Video Technology ⚠️ *Commercial license - not included*
- **Audio**: FMOD sound system ⚠️ *Commercial license - not included*

## New Tech stack

- **Game engine**: Custom 3D engine, mostly written in C++
- **Scripting language**: Lua
- **Animation**: Granny Animation (RAD Game Tools) - from [uesp-esoapps](https://github.com/uesp/uesp-esoapps.git)
- **Audio**: FLESSD sound system - Open source version of FMOD

## New changes

- Old BK2 source code was ported to modern Visual Studio 2022, CMake
- BK2 can now also be compiled as 64 bit app
- Replaced FMOD with FLESSD
- Removed dependence on S3TC lib
- Videos are disabled for now - to be implemented with FFMPEG in the future
- Some new features:
    - Added an option to disable certain techlevels for certain nations in MP
    - Reworked reinforcement inheritance system in MP
    - Fixed the "green team base capping advantage" on neutral bases by introducing the contested base state
    - Player color pallete (HP bars) now supports up to 16 colors (which is also the max possible number of players)

## What is in this repository

- `Complete` — game data and resources
- `Design` — design documents and art resources  
- `Soft` — source code and development tools
- `Sound` — game sound resources
- `Tools` — development and build tools
- `Localizations` — localization files
- `Versions` — different build configurations and testing environments
- `Versions/Temporary/Engine/Sources` — complete game engine source code

---

# Running the game

## Basic launch
1. Navigate to the `Complete/bin/` directory
2. Run the game executable (if available)

---

# Map editor and development tools

## Map editor
- **Location**: `Complete/Editor/`
- **Documentation**: `Design/Manuals/MapEditorManual/`
- **Manual**: `Design/Manuals/MapEditorManual/Final/`
- **FAQ**: `Design/Manuals/MapEditorManual/FAQ/`

## Development tools
- **Maya plugins**: `Tools/MayaScripts/`
- **Texture converters**: `Tools/TexConv.exe`, `Tools/DxTex.exe`
- **Font generator**: `Tools/FontGen.exe`
- **Granny tools**: `Tools/Granny/`

---


# Building the project

See [Contributing.md](https://github.com/OpenBK2/OpenBK2/blob/port/Contributing.md) file.

---

## License Information

This project is released under a **special non-commercial license** from NIVAL INTERNATIONAL LTD.

### ✅ What's included and open source:
- **Game engine source code** - Custom license from NIVAL INTERNATIONAL LTD (non-commercial use only)
- **zlib compression library** - zlib License (permissive, commercial use allowed)
- **Game scripts, assets, and data** - Custom license from NIVAL INTERNATIONAL LTD (non-commercial use only)

### ⚠️ Additional tools not included in source code:
- **FMOD Audio System**
- **Bink Video Technology**
- **Granny3D Animation System**
- **Stingray Studio UI Components**
- **MySQL Database**
- **S3TC Texture Compression**

### 📋 Third-party licenses:
- **zlib** (v1.1.3) - Copyright (C) 1995-1998 Jean-loup Gailly and Mark Adler - zlib License

Please review the complete [license agreement](LICENSE.md) before using this code.

