[English](README.md)        [Русский](README_Russian.md)        [中文](README_Chinese.md)        [हिन्दी](README_Hindi.md)        [Español](README_Spanish.md)        [Français](README_French.md)        [Deutsch](README_German.md)        [Português](README_Portuguese.md)        [日本語](README_Japanese.md)        [Bahasa Indonesia](README_Indonesian.md)

[![Blitzkrieg II Trailer](Blitzkrieg_2.png)](https://www.youtube.com/watch?v=Cw8rA2hvDGg)

コンピュータゲーム [Blitzkrieg 2](https://en.wikipedia.org/wiki/Blitzkrieg_2) は伝説的なリアルタイムストラテジー戦争ゲームシリーズの第2作で、[Nival Interactive](http://nival.com/) によって開発され、2005年にリリースされました。

本作は現在も [Steam](https://store.steampowered.com/app/313500/Blitzkrieg_2_Anthology) と [GOG.com](https://www.gog.com/en/game/blitzkrieg_2_anthology) で入手可能です。

2025年に、本作のソースコードは商用利用を禁じる [special license](LICENSE.md) のもとで公開され、コミュニティ、教育、研究目的には完全にオープンになりました。使用にあたっては [license agreement](LICENSE.md) の条件をよくご確認ください。

## 技術スタック

- **ゲームエンジン**: カスタム3Dエンジン（主にC++で記述）  
- **スクリプト言語**: Lua  
- **アニメーション**: Granny Animation (RAD Game Tools) ⚠️ *商用ライセンス - 含まれていません*
- **ビデオ**: Bink Video Technology ⚠️ *商用ライセンス - 含まれていません*
- **オーディオ**: FMODサウンドシステム ⚠️ *商用ライセンス - 含まれていません*  

## このリポジトリの内容

- `Complete` — ゲームデータとリソース  
- `Design` — デザイン文書およびアートリソース  
- `Soft` — ソースコードと開発ツール  
- `Sound` — ゲームの音声リソース  
- `Tools` — 開発およびビルドツール  
- `Localizations` — ローカライズファイル
- `Versions` — 異なるビルド設定とテスト環境  
- `Versions/Temporary/Engine/Sources` — 完全なゲームエンジンのソースコード  

---

# ゲームの起動

## 基本的な起動方法  
1. `Complete/bin/` ディレクトリへ移動  
2. 実行ファイル（存在する場合）を起動  

---

# マップエディタと開発ツール

## マップエディタ  
- **場所**: `Complete/Editor/`  
- **ドキュメント**: `Design/Manuals/MapEditorManual/`  
- **マニュアル**: `Design/Manuals/MapEditorManual/Final/`  
- **FAQ**: `Design/Manuals/MapEditorManual/FAQ/`  

## 開発ツール  
- **Mayaプラグイン**: `Tools/MayaScripts/`  
- **テクスチャコンバータ**: `Tools/TexConv.exe`, `Tools/DxTex.exe`  
- **フォントジェネレータ**: `Tools/FontGen.exe`  
- **Grannyツール**: `Tools/Granny/`  

---


# プロジェクトのビルド

## ビルド要件  
- Microsoft Visual Studio (2003)  
- DirectX SDK  
- その他の依存関係はドキュメントに記載

---

## ライセンス情報

このプロジェクトはNIVAL INTERNATIONAL LTDの**特別な非商用ライセンス**の下で公開されています。

### ✅ 含まれているオープンソースコンテンツ：
- **ゲームエンジンソースコード** - NIVAL INTERNATIONAL LTDのカスタムライセンス（非商用利用のみ）
- **zlib圧縮ライブラリ** - zlibライセンス（寛容的、商用利用可能）
- **ゲームスクリプト、アセット、データ** - NIVAL INTERNATIONAL LTDのカスタムライセンス（非商用利用のみ）

### ⚠️ ソースコードに含まれていない追加ツール：
- **FMOD Audio System**
- **Bink Video Technology**
- **Granny3D Animation System**
- **Stingray Studio UI Components**
- **MySQL Database**
- **S3TC Texture Compression**

### 📋 サードパーティライセンス：
- **zlib** (v1.1.3) - Copyright (C) 1995-1998 Jean-loup Gailly and Mark Adler - zlibライセンス

このコードを使用する前に、完全な[ライセンス契約](LICENSE.md)をご確認ください。  
