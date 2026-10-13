[English](README.md)        [Русский](README_Russian.md)        [中文](README_Chinese.md)        [हिन्दी](README_Hindi.md)        [Español](README_Spanish.md)        [Français](README_French.md)        [Deutsch](README_German.md)        [Português](README_Portuguese.md)        [日本語](README_Japanese.md)        [Bahasa Indonesia](README_Indonesian.md)

[![Blitzkrieg II Trailer](Blitzkrieg_2.png)](https://www.youtube.com/watch?v=Cw8rA2hvDGg)

电脑游戏 [Blitzkrieg 2] 是传奇实时战略战争游戏系列的第二部，由 [Nival Interactive] 开发，于 2005 年发布。

该游戏仍可在 [Steam] 和 [GOG.com] 平台获取。

2025 年，游戏源代码在 [special license] 下发布，该许可证禁止商业使用，但对游戏社区、教育和研究完全开放。请在使用前仔细阅读 [license agreement] 条款。

## 技术栈

- **游戏引擎**：自研 3D 引擎，主要用 C++ 编写  
- **脚本语言**：Lua  
- **动画**：Granny Animation (RAD Game Tools) ⚠️ *商业许可证 - 未包含*
- **视频**：Bink Video Technology ⚠️ *商业许可证 - 未包含*
- **音频**：FMOD sound system ⚠️ *商业许可证 - 未包含*  

仓库内容
- Complete — 游戏数据和资源  
- Design — 设计文档和美术资源  
- Soft — 源代码和开发工具  
- Sound — 游戏音频资源  
- Tools — 开发与构建工具  
- Localizations — 本地化文件
- Versions — 不同的构建配置和测试环境  
- Versions/Temporary/Engine/Sources — 完整的游戏引擎源代码  

运行游戏

基本启动  
1. 进入目录 Complete/bin/  
2. 运行游戏可执行文件（若存在）  


地图编辑器和开发工具

地图编辑器  
- 位置：Complete/Editor/  
- 文档：Design/Manuals/MapEditorManual/  
- 使用手册：Design/Manuals/MapEditorManual/Final/  
- 常见问题：Design/Manuals/MapEditorManual/FAQ/  

开发工具  
- Maya 插件：Tools/MayaScripts/  
- 贴图转换器：Tools/TexConv.exe，Tools/DxTex.exe  
- 字体生成器：Tools/FontGen.exe  
- Granny 工具：Tools/Granny/  



项目构建

## 构建要求  
- Microsoft Visual Studio (2003)  
- DirectX SDK  
- 其他依赖项详见文档

---

## 许可证信息

本项目在 NIVAL INTERNATIONAL LTD 的**特殊非商业许可证**下发布。

### ✅ 包含且开源的内容：
- **游戏引擎源代码** - NIVAL INTERNATIONAL LTD 自定义许可证（仅限非商业使用）
- **zlib 压缩库** - zlib 许可证（宽松许可证，允许商业使用）
- **游戏脚本、资产和数据** - NIVAL INTERNATIONAL LTD 自定义许可证（仅限非商业使用）

### ⚠️ 未包含在源代码中的附加工具：
- **FMOD 音频系统**
- **Bink 视频技术**
- **Granny3D 动画系统**
- **Stingray Studio UI 组件**
- **MySQL 数据库**
- **S3TC 纹理压缩**

### 📋 第三方许可证：
- **zlib** (v1.1.3) - Copyright (C) 1995-1998 Jean-loup Gailly and Mark Adler - zlib 许可证

请在使用此代码前仔细阅读完整的[许可协议](LICENSE.md)。  
