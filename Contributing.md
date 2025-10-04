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

go to `Project -> CMake Settings for b2` menu

click green plus sign (Add a new configuration), select `x86-Release`

add installation prefix to `CMake command arguments`. e.g. to install game into the `C:\Games\bk2` directory, use the following:

```bash
-DCMAKE_INSTALL_PREFIX=C:/Games/bk2
```
remove the default `x64-Debug` configuration (it won't build anyway).

save the file (Ctrl+S).

once CMake configuration is completed, use `Build -> Install Project` menu.

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
