[English](README.md)        [Русский](README_Russian.md)        [中文](README_Chinese.md)        [हिन्दी](README_Hindi.md)        [Español](README_Spanish.md)        [Français](README_French.md)        [Deutsch](README_German.md)        [Português](README_Portuguese.md)        [日本語](README_Japanese.md)        [Bahasa Indonesia](README_Indonesian.md)

[![Build (Windows)](https://github.com/OpenBK2/OpenBK2/actions/workflows/build-windows.yml/badge.svg)](https://github.com/OpenBK2/OpenBK2/actions/workflows/build-windows.yml) [![Build (Linux)](https://github.com/OpenBK2/OpenBK2/actions/workflows/build-linux.yml/badge.svg)](https://github.com/OpenBK2/OpenBK2/actions/workflows/build-linux.yml)

[![Blitzkrieg II Trailer](Blitzkrieg_2.png)](https://www.youtube.com/watch?v=Cw8rA2hvDGg)

कंप्यूटर गेम [Blitzkrieg 2](https://en.wikipedia.org/wiki/Blitzkrieg_2) दूसरी कड़ी है असली समय आधारित रणनीतिक युद्ध गेम्स की प्रसिद्ध सीरीज की, जिसे [Nival Interactive](http://nival.com/) ने विकसित किया था और 2005 में जारी किया गया था।

यह गेम अभी भी [Steam](https://store.steampowered.com/app/313500/Blitzkrieg_2_Anthology) और [GOG.com](https://www.gog.com/en/game/blitzkrieg_2_anthology) पर उपलब्ध है।

2025 में इस गेम का सोर्स कोड [special license](LICENSE.md) के तहत जारी किया गया, जो व्यावसायिक उपयोग को प्रतिबंधित करता है लेकिन गेम समुदाय, शिक्षा और शोध के लिए पूरी तरह खुला है। कृपया उपयोग से पहले [license agreement](LICENSE.md) की शर्तें ध्यान से पढ़ें।

## तकनीकी स्टैक

- **गेम इंजन**: कस्टम 3D इंजन, मुख्यतः C++ में लिखा गया  
- **स्क्रिप्टिंग भाषा**: Lua  
- **एनिमेशन**: Granny Animation (RAD Game Tools) ⚠️ *व्यावसायिक लाइसेंस - शामिल नहीं*
- **वीडियो**: Bink Video Technology ⚠️ *व्यावसायिक लाइसेंस - शामिल नहीं*
- **ऑडियो**: FMOD sound system ⚠️ *व्यावसायिक लाइसेंस - शामिल नहीं*  

## इस रिपॉजिटरी में क्या है

- `Complete` — गेम डेटा और संसाधन  
- `Design` — डिज़ाइन दस्तावेज़ और आर्ट संसाधन  
- `Soft` — सोर्स कोड और विकास उपकरण  
- `Sound` — गेम के साउंड संसाधन  
- `Tools` — विकास और बिल्ड उपकरण  
- `Localizations` — स्थानीयकरण फ़ाइलें
- `Versions` — विभिन्न बिल्ड कॉन्फ़िगरेशन और परीक्षण वातावरण  
- `Versions/Temporary/Engine/Sources` — गेम इंजन का पूर्ण सोर्स कोड  

---

# गेम चलाना

## बुनियादी लॉन्च  
1. `Complete/bin/` डायरेक्टरी में जाएँ  
2. गेम का कार्यन्वित फ़ाइल चलाएँ (यदि उपलब्ध हो)  

---

# मैप एडिटर और विकास उपकरण

## मैप एडिटर  
- **स्थान**: `Complete/Editor/`  
- **दस्तावेज़ीकरण**: `Design/Manuals/MapEditorManual/`  
- **मैनुअल**: `Design/Manuals/MapEditorManual/Final/`  
- **FAQ**: `Design/Manuals/MapEditorManual/FAQ/`  

## विकास उपकरण  
- **Maya प्लगइन्स**: `Tools/MayaScripts/`  
- **टेक्सचर कनवर्टर्स**: `Tools/TexConv.exe`, `Tools/DxTex.exe`  
- **फ़ॉन्ट जनरेटर**: `Tools/FontGen.exe`  
- **Granny टूल्स**: `Tools/Granny/`  

---


# प्रोजेक्ट निर्माण

## बिल्ड आवश्यकताएँ  
- Microsoft Visual Studio (2003)  
- DirectX SDK  
- अतिरिक्त निर्भरताएँ दस्तावेज़ीकरण में निर्दिष्ट हैं

---

## लाइसेंस जानकारी

यह प्रोजेक्ट NIVAL INTERNATIONAL LTD के **विशेष गैर-व्यावसायिक लाइसेंस** के तहत जारी किया गया है।

### ✅ जो शामिल है और ओपन सोर्स है:
- **गेम इंजन सोर्स कोड** - NIVAL INTERNATIONAL LTD का कस्टम लाइसेंस (केवल गैर-व्यावसायिक उपयोग)
- **zlib कंप्रेशन लाइब्रेरी** - zlib लाइसेंस (अनुमतिशील, व्यावसायिक उपयोग की अनुमति)
- **गेम स्क्रिप्ट्स, एसेट्स और डेटा** - NIVAL INTERNATIONAL LTD का कस्टम लाइसेंस (केवल गैर-व्यावसायिक उपयोग)

### ⚠️ सोर्स कोड में शामिल नहीं अतिरिक्त उपकरण:
- **FMOD Audio System**
- **Bink Video Technology**
- **Granny3D Animation System**
- **Stingray Studio UI Components**
- **MySQL Database**
- **S3TC Texture Compression**

### 📋 तृतीय-पक्ष लाइसेंस:
- **zlib** (v1.1.3) - Copyright (C) 1995-1998 Jean-loup Gailly and Mark Adler - zlib लाइसेंस

इस कोड का उपयोग करने से पहले पूर्ण [लाइसेंस समझौता](LICENSE.md) की समीक्षा करें।  
