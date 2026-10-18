[English](README.md)        [Русский](README_Russian.md)        [中文](README_Chinese.md)        [हिन्दी](README_Hindi.md)        [Español](README_Spanish.md)        [Français](README_French.md)        [Deutsch](README_German.md)        [Português](README_Portuguese.md)        [日本語](README_Japanese.md)        [Bahasa Indonesia](README_Indonesian.md)

[![Build (Windows)](https://github.com/OpenBK2/OpenBK2/actions/workflows/build-windows.yml/badge.svg)](https://github.com/OpenBK2/OpenBK2/actions/workflows/build-windows.yml) [![Build (Linux)](https://github.com/OpenBK2/OpenBK2/actions/workflows/build-linux.yml/badge.svg)](https://github.com/OpenBK2/OpenBK2/actions/workflows/build-linux.yml)

[![Blitzkrieg II Trailer](Blitzkrieg_2.png)](https://www.youtube.com/watch?v=Cw8rA2hvDGg)

Компьютерная игра [Blitzkrieg 2](https://en.wikipedia.org/wiki/Blitzkrieg_2) это вторая часть легендарной серии военных стратегий в реальном времени, разработанная [Nival Interactive](http://nival.com/) и выпущенная в 2005 года.

Игра до сих пор доступна в [Steam](https://store.steampowered.com/app/313500/Blitzkrieg_2_Anthology) и [GOG.com](https://www.gog.com/en/game/blitzkrieg_2_anthology).

В 2025 году исходный код игры был выпущен под [специальной лицензией](LICENSE.md), которая запрещает коммерческое использование, но полностью открыта для сообщества, образовательных и исследовательских целей. Пожалуйста, внимательно ознакомьтесь с условиями [лицензионного соглашения](LICENSE.md) перед использованием.

## Технологический стек

- **Движок**: собственный 3D-движок, преимущественно написанный на C++  
- **Язык сценариев**: Lua  
- **Анимация**: Granny Animation (RAD Game Tools) ⚠️ *Коммерческая лицензия - не включена*
- **Видео**: Bink Video Technology ⚠️ *Коммерческая лицензия - не включена*
- **Звук**: FMOD sound system ⚠️ *Коммерческая лицензия - не включена*  

## Что содержится в этом репозитории

- `Complete` — игровые данные и ресурсы  
- `Design` — документы по дизайну и арт-ресурсы  
- `Soft` — исходный код и инструменты разработки  
- `Sound` — звуковые ресурсы игры  
- `Tools` — инструменты для разработки и сборки  
- `Localizations` — файлы локализаций  
- `Versions` — различные конфигурации сборки и тестовые окружения  
- `Versions/Temporary/Engine/Sources` — полный исходный код игрового движка  

---

# Запуск игры

## Базовый запуск  
1. Перейдите в каталог `Complete/bin/`  
2. Запустите исполняемый файл игры (если он присутствует)  

---

# Редактор карт и инструменты разработки

## Редактор карт  
- **Расположение**: `Complete/Editor/`  
- **Документация**: `Design/Manuals/MapEditorManual/`  
- **Руководство**: `Design/Manuals/MapEditorManual/Final/`  
- **FAQ**: `Design/Manuals/MapEditorManual/FAQ/`  

## Инструменты разработки  
- **Плагины для Maya**: `Tools/MayaScripts/`  
- **Конвертеры текстур**: `Tools/TexConv.exe`, `Tools/DxTex.exe`  
- **Генератор шрифтов**: `Tools/FontGen.exe`  
- **Инструменты Granny**: `Tools/Granny/`  

---


# Сборка проекта

## Требования для сборки  
- Microsoft Visual Studio (2003)  
- DirectX SDK  
- Дополнительные зависимости указаны в документации

---

## Информация о лицензиях

Данный проект выпущен под **специальной некоммерческой лицензией** от NIVAL INTERNATIONAL LTD.

### ✅ Что включено и является открытым исходным кодом:
- **Исходный код игрового движка** - Пользовательская лицензия от NIVAL INTERNATIONAL LTD (только некоммерческое использование)
- **Библиотека сжатия zlib** - Лицензия zlib (разрешительная, коммерческое использование разрешено)
- **Игровые скрипты, ассеты и данные** - Пользовательская лицензия от NIVAL INTERNATIONAL LTD (только некоммерческое использование)

### ⚠️ Дополнительные инструменты, не включенные в исходный код:
- **FMOD Audio System**
- **Bink Video Technology**
- **Granny3D Animation System**
- **Stingray Studio UI Components**
- **MySQL Database**
- **S3TC Texture Compression**

### 📋 Лицензии сторонних компонентов:
- **zlib** (v1.1.3) - Copyright (C) 1995-1998 Jean-loup Gailly and Mark Adler - Лицензия zlib

Пожалуйста, внимательно ознакомьтесь с полным [лицензионным соглашением](LICENSE.md) перед использованием данного кода.  
