# Reference Sources

MilCAM gelistirirken hangi projeleri acmali?

## Birinci Derece — Her gun

### FreeCAD upstream
- **Konum:** `/home/embed/Downloads/FreeCAD-main-1-1-git/`
- **Boyut:** ~3.1 GB
- **Niye:** THE backbone. MilCAM'in yapisi FreeCAD'in dogrudan build'i.
  `FREECAD_SOURCE_DIR` bu konuma point eder.
- **Onemli alt dizinler:**
  - `src/Mod/CAM/` — CAM workbench (FreeCAD 1.0+ naming)
  - `src/Mod/CAM/PathScripts/` veya `src/Mod/CAM/Path/` — Python orchestration
  - `src/Mod/CAM/PostScripts/` — referans postprocessorlar (codesys_post buradan ilham aldi)
  - `src/Mod/Path/` — eski naming (1.0 oncesi)
  - `cMake/FreeCAD_Helpers/InitializeFreeCADBuildOptions.cmake` — BUILD_<X> flag listesi
  - `src/Gui/` — FreeCAD main window, workbench manager, command registration
  - `src/Mod/Gui/Workbench.h` — workbench API

### FreeCAD Forum + Wiki
- https://forum.freecad.org/ — workbench gelistirme, CAM
- https://wiki.freecad.org/Path_Workbench — kullanici dokumantasyonu
- https://wiki.freecad.org/Workbench_creation — Python workbench yapisi

## Ikinci Derece — Zaman zaman

### CADNC (kardes proje)
- **Konum:** `/home/embed/Dev/CADNC/`
- **Niye:** Tarihi referans. MilCAM eskiden CADNC'nin kopyasıydı. Artik
  **kod paylasmiyoruz**. Sadece VEC-VE dokumanlari icin acilir
  (`doc/VEC-VE-*.pdf` ayni belgeler, MilCAM/doc'a tasindi).
- **Kullanim sınırı:** CADNC'nin CAM modulu (cam/) MilCAM'e KOPYALANMAYACAK
  — MilCAM FreeCAD'in kendi CAM'ini kullanir.

### MilCAD (legacy)
- **Konum:** `/home/embed/Dev/MilCAD/`
- **Niye:** Onceki nesil; sadece teknik fikir baglami. Kod kopyalanmaz.

## Ucuncu Derece — Ozel durumlar

### CodeSys dokumantasyonu
- https://content.helpme-codesys.com/
- Onemli sayfalar:
  - `SysProcessExecuteCommand` — TargetVisu'dan harici program acma
  - `SMC_ReadNCFile2` + `SMC_NCInterpreter` — G-code yorumlama
  - OPC UA Server setup
  - Target Visualization config: `targetvisuextern.cfg`

### open62541 (Faz 4)
- https://open62541.org/doc/master/
- https://github.com/open62541/open62541

### Qt VirtualKeyboard
- https://doc.qt.io/qt-6/qtvirtualkeyboard-index.html
- FreeCAD'in Qt build'i VirtualKeyboard plugin'i ile derlenmis mi kontrol
  edilmeli (genelde evet, ama vendor build'leri farklı olabilir).

## Reverse Lookup — "X icin nereye?"

| Soru                                          | Kaynak                                          |
|-----------------------------------------------|-------------------------------------------------|
| "FreeCAD'in CAM workbench Job objesi nasil yaratilir?" | `src/Mod/CAM/CAMTests/` veya wiki Path_Job |
| "BUILD_X flag'inin gerçek anlamı?"            | `cMake/FreeCAD_Helpers/InitializeFreeCADBuildOptions.cmake` |
| "Workbench Python API"                        | `src/Mod/Start/StartGui/Workbench.py` (referans)|
| "Custom postprocessor nasil yazilir?"          | `src/Mod/CAM/CAMScripts/PostScripts/refactored_test_post.py` |
| "FreeCAD command register pattern"            | `src/Mod/Part/Gui/Command.cpp` veya herhangi Mod/*/Gui/Command*.py |
| "Workbench preferences (Disabled list)"        | Wiki: Preferences Editor                        |
| "FreeCAD'in main() entry point"               | `src/Main/MainGui.cpp`                          |

## Kopyalama Kurallari

| Kaynak              | Kod kopyalanir? | Niye                                          |
|---------------------|-----------------|-----------------------------------------------|
| FreeCAD             | ZATEN KULLANIYORUZ via add_subdirectory; "kopyalama" mantiksiz |
| FreeCAD postprocessor template | Evet — `codesys_post.py` derivative (LGPL) | Lisans uygun, FreeCAD pattern'ini koruma |
| CADNC               | HAYIR           | Farkli mimari, yanlis baglam                  |
| MilCAD              | HAYIR           | Legacy                                        |
| Web snippet         | Lisans kontrol  | MIT/BSD OK; GPL hayır                         |

## Eger Tum Bunlari Birden Aciklamayi Unutursan

Bir cumlede MilCAM = "FreeCAD with CAM-only modules + Python workbench
that hides others + Send-to-CodeSys command + CODESYS postprocessor".
