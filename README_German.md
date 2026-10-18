[English](README.md)        [Русский](README_Russian.md)        [中文](README_Chinese.md)        [हिन्दी](README_Hindi.md)        [Español](README_Spanish.md)        [Français](README_French.md)        [Deutsch](README_German.md)        [Português](README_Portuguese.md)        [日本語](README_Japanese.md)        [Bahasa Indonesia](README_Indonesian.md)

[![Build (Windows)](https://github.com/OpenBK2/OpenBK2/actions/workflows/build-windows.yml/badge.svg)](https://github.com/OpenBK2/OpenBK2/actions/workflows/build-windows.yml) [![Build (Linux)](https://github.com/OpenBK2/OpenBK2/actions/workflows/build-linux.yml/badge.svg)](https://github.com/OpenBK2/OpenBK2/actions/workflows/build-linux.yml)

[![Blitzkrieg II Trailer](Blitzkrieg_2.png)](https://www.youtube.com/watch?v=Cw8rA2hvDGg)

Das Computerspiel [Blitzkrieg 2] ist der zweite Teil der legendären Serie von Echtzeit-Strategiespielen, entwickelt von [Nival Interactive] und 2005 veröffentlicht.

Das Spiel ist nach wie vor auf [Steam] und [GOG.com] verfügbar.

Im Jahr 2025 wurde der Quellcode des Spiels unter einer [special license] freigegeben, die kommerzielle Nutzung untersagt, aber vollständig offen für die Community, Bildung und Forschung ist. Bitte lesen Sie die Bedingungen des [license agreement] sorgfältig, bevor Sie ihn verwenden.

## Technologiestack

- **Game Engine**: eigener 3D-Engine, überwiegend in C++  
- **Scriptsprache**: Lua  
- **Animation**: Granny Animation (RAD Game Tools) ⚠️ *Kommerzielle Lizenz - nicht enthalten*
- **Video**: Bink Video Technology ⚠️ *Kommerzielle Lizenz - nicht enthalten*
- **Audio**: FMOD sound system ⚠️ *Kommerzielle Lizenz - nicht enthalten*  

## Inhalt des Repositorys

- Complete — Spieldaten und Ressourcen  
- Design — Designdokumente und Grafikressourcen  
- Soft — Quellcode und Entwicklungstools  
- Sound — Audioressourcen des Spiels  
- Tools — Entwicklungs- und Build-Tools  
- Localizations — Lokalisierungsdateien
- Versions — verschiedene Build-Konfigurationen und Testumgebungen  
- Versions/Temporary/Engine/Sources — vollständiger Quellcode der Spiel-Engine  

# Spielstart

## Grundlegender Start
1. Wechseln Sie in das Verzeichnis `Complete/bin/`  
2. Starten Sie die Spiel-Executable (falls vorhanden)  


# Karteneditor und Entwicklungstools

## Karteneditor
- Standort: `Complete/Editor/`  
- Dokumentation: `Design/Manuals/MapEditorManual/`  
- Handbuch: `Design/Manuals/MapEditorManual/Final/`  
- FAQ: `Design/Manuals/MapEditorManual/FAQ/`  

## Entwicklungstools
- Maya-Plugins: `Tools/MayaScripts/`  
- Texturkonverter: `Tools/TexConv.exe`, `Tools/DxTex.exe`  
- Schriftgenerator: `Tools/FontGen.exe`  
- Granny-Tools: `Tools/Granny/`  



# Projekt bauen

## Build-Anforderungen
- Microsoft Visual Studio (2003)  
- DirectX SDK  
- Weitere Abhängigkeiten siehe Dokumentation

---

## Lizenzinformationen

Dieses Projekt wird unter einer **speziellen nicht-kommerziellen Lizenz** von NIVAL INTERNATIONAL LTD veröffentlicht.

### ✅ Was enthalten und Open Source ist:
- **Spiel-Engine-Quellcode** - Benutzerdefinierte Lizenz von NIVAL INTERNATIONAL LTD (nur nicht-kommerzielle Nutzung)
- **zlib-Kompressionsbibliothek** - zlib-Lizenz (permissiv, kommerzielle Nutzung erlaubt)
- **Spielskripte, Assets und Daten** - Benutzerdefinierte Lizenz von NIVAL INTERNATIONAL LTD (nur nicht-kommerzielle Nutzung)

### ⚠️ Zusätzliche Tools, die nicht im Quellcode enthalten sind:
- **FMOD Audio System**
- **Bink Video Technology**
- **Granny3D Animation System**
- **Stingray Studio UI-Komponenten**
- **MySQL Database**
- **S3TC Texture Compression**

### 📋 Drittanbieter-Lizenzen:
- **zlib** (v1.1.3) - Copyright (C) 1995-1998 Jean-loup Gailly and Mark Adler - zlib-Lizenz

Bitte lesen Sie die vollständige [Lizenzvereinbarung](LICENSE.md) vor der Nutzung dieses Codes.  
