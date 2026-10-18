[English](README_English.md)        [Русский](README.md)        [中文](README_Chinese.md)        [हिन्दी](README_Hindi.md)        [Español](README_Spanish.md)        [Français](README_French.md)        [Deutsch](README_German.md)        [Português](README_Portuguese.md)        [日本語](README_Japanese.md)        [Bahasa Indonesia](README_Indonesian.md)

[![Build (Windows)](https://github.com/OpenBK2/OpenBK2/actions/workflows/build-windows.yml/badge.svg)](https://github.com/OpenBK2/OpenBK2/actions/workflows/build-windows.yml) [![Build (Linux)](https://github.com/OpenBK2/OpenBK2/actions/workflows/build-linux.yml/badge.svg)](https://github.com/OpenBK2/OpenBK2/actions/workflows/build-linux.yml)

[![Blitzkrieg Trailer](Blitzkrieg.png)](https://www.youtube.com/watch?v=zNxMvTcsJbk)

The computer game [Blitzkrieg](https://wikipedia.org/wiki/Blitzkrieg_(video_game)) is the first installment of the legendary series of real-time strategy war games, developed by [Nival Interactive](http://nival.com/) and released by 1C Company on March 28, 2003.

The game is still available on [Steam](https://store.steampowered.com/app/313480/Blitzkrieg_Anthology/) and [GOG.com](https://www.gog.com/en/game/blitzkrieg_anthology).

In 2025, the game's singleplayer source code was released under a [special license](LICENSE.md) that prohibits commercial use but is completely open for the game's community, education and research.
Please review the terms of the [license agreement](LICENSE.md) carefully before using it.

# What is in this repository
- `Data` - game data
- `Soft` and `Tools` - development tools
- `Versions` - compiled versions of the game, including map editors
- `Sources` - source code and tools
- `Versions/Temporary/Engine/Sources` - complete game engine source code

# Preparation

All libraries from the SDK directory are needed for compilation. The paths to them must be entered in **Tools => Options => Directories** in the following order:

## Include
```
C:\PROGRAM FILES\MICROSOFT VISUAL STUDIO\VC98\STLPORT
C:\SDK\BINK (not included in the repository)
C:\SDK\FMOD\API\INC (not included in the repository)
C:\SDK\S3TC
C:\SDK\STINGRAY STUDIO 2002\INCLUDE\TOOLKIT (not included in the repository)
C:\SDK\STINGRAY STUDIO 2002\INCLUDE (not included in the repository)
C:\SDK\STINGRAY STUDIO 2002\REGEX\INCLUDE (not included in the repository)
C:\SDK\Maya4.0\include
```

## Lib
```
C:\SDK\BINK (not included in the repository)
C:\SDK\FMOD\API\LIB (not included in the repository)
C:\SDK\S3TC
C:\SDK\STINGRAY STUDIO 2002\LIB (not included in the repository)
C:\SDK\STINGRAY STUDIO 2002\REGEX\LIB (not included in the repository)
C:\SDK\Maya4.0\lib
```

In addition, **DirectX 8.1** or higher is required (it will automatically be added to the paths).

### Important Notes

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

- **Bink, FMOD, Stingray** libraries are not included in this repository as they require separate licensing.
- **stlport** *must* be located in the Visual C directory, alongside `include`.
- The path `C:\PROGRAM FILES\MICROSOFT VISUAL STUDIO\VC98\STLPORT` must be **first**, otherwise, the build will fail.

---

# Additional Tools

- The **tools** directory contains utilities used during the build process.
- Resources are stored in **zip (deflate)** format and are packed/unpacked using **zip/unzip**.
- **Do not use pkzip** — it truncates file names and does not use the deflate algorithm.
- Some data is edited manually using an **XML-editor**, as frequent editing was not necessary and writing a separate editor was impractical.

---

# Files in `data`

In the game's directory, under **data**, there are files that are manually edited or simply placed:

- `sin.arr` — binary file with a sine table (just place it, do not touch).
- `objects.xml` — registry of game objects (edited manually).
- `consts.xml` — game constants for designers (edited manually).
- `MusicSettings.xml` — music settings (edited manually).
- `partys.xml` — country data (which squad to use for gun crew, parachutist model, etc.).

## Files in `medals`

In the **medals** subdirectory, files `ranks.xml` contain ranks and **experience** needed to obtain them, organized by country.
