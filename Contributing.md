# Clone the repository

install [Git for Windows](https://git-scm.com/downloads/win)

and clone the repository with the following command:

```bash
git clone --recursive https://github.com/SSE4/OpenBK2.git
```

# Visual Studio 2022

Install [Microsoft Direct SDK (June 2010)](https://www.microsoft.com/en-us/download/details.aspx?id=6812).

Install [Microsoft Visual Studio 2022](https://visualstudio.microsoft.com/downloads/) with the following workloads:
- Desktop development with C++

Use `File -> Open -> Folder` menu (Ctrl+Shift+Alt+O) to open the cloned repository folder

go to `Project ->Edit CMake Presets for b2` menu

Edit the `configurePresets` part of the file, `CMAKE_INSTALL_PREFIX` path. e.g. to install game into the `C:\Games\bk2` directory, use the following:

```cmake
  "configurePresets": [
    {
      "name": "base-windows",
      "hidden": true,
      "displayName": "Base Windows Ninja Preset",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/out/build/${presetName}",
      "cacheVariables": {
        "CMAKE_EXPORT_COMPILE_COMMANDS": "ON",
        "CMAKE_VERBOSE_MAKEFILE": "ON",
        "CMAKE_INSTALL_PREFIX": "C:/Games/bk2"
      }
    },
```

save the file (Ctrl+S).

once CMake configuration is completed, use `Build -> Install b2` menu.

# copying data files

copy the following files and directories manually into the game directory:
- [Versions/Current/Profiles](Versions/Current/Profiles)
- [Versions/Current/Data](Versions/Current/Data)
- [Versions/Current/splash.bmp](Versions/Current/splash.bmp)

assuming installation into the `C:\Games\bk2` directory, the commands to copy will be:
```cmd
robocopy Versions\Current\Profiles\ C:\Games\bk2\Profiles\ /S /E >NUL
robocopy Versions\Current\Data\ C:\Games\bk2\Data\ /S /E >NUL
xcopy Versions\Current\splash.bmp C:\Games\bk2\
```

copying will take a while - there are **MANY** small files.

# launching the game

the game executable is `bin\Game.exe`. therefore, if game was installed into `C:\Games\bk2`, then `C:\Games\bk2\bin\Game.exe` should be launched.

