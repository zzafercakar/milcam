# Gelecek Oturum Icin Ilk Mesaj

## Onerilen Ilk Okumalar

```
Read .ai/START_HERE.md
Read .ai/context.yaml
Read .ai/WORKPLAN.md
Read .ai/ARCHITECTURE.md
Read .ai/ENGINEERING_LOG.md
```

## Sonraki Beklenen Adim (2026-06-08 itibariyle)

**Faz 1 — Ilk End-to-End Build Denemesi.**

```bash
cd /home/embed/Dev/MilCAM
cmake --preset default      # FreeCAD'i wrap eden CMake'i çalistir
cmake --build build -j$(nproc)
```

Beklenen pürüzler (`.ai/ENGINEERING_LOG.md` "Beklenen Hata Listesi"
bolumunden):
- `add_subdirectory(${FREECAD_SOURCE_DIR})` FreeCAD CMake varsayimlariyla
  cakisabilir. Belirti: `Could not find configure_file` benzeri.
- `BUILD_<X>=OFF` flag'leri bazi inter-module link hatasina sebep olabilir.
- Eksik sistem paketleri (Coin3D, OCCT, Boost, Xerces, Qt5/6 Quick Controls).

Eger build basariliysa:
```bash
DESTDIR=$PWD/_inst cmake --install build
DISPLAY=:0 QT_QPA_PLATFORM=xcb _inst/usr/local/bin/FreeCAD
```

Acilan FreeCAD'in workbench picker'inda **sadece** MilCAM + CAM + Part +
Sketcher + PartDesign + Draft + Mesh + Spreadsheet gozukmeli. FEM/BIM/
TechDraw/Robot YOK.

## Eger Kullanici "MilCAM ic tasarimini degistirmek istiyorum" derse

| Istek                                  | Nereyi degistir                                  |
|----------------------------------------|--------------------------------------------------|
| "Bir buton ekle"                       | `milcam/Mod/MilCAM/Commands/<Yeni>.py`           |
| "G-code formatini degistir"             | `milcam/Mod/MilCAM/PostProcessor/codesys_post.py`|
| "Hangi workbench'ler gozuksun?"         | `milcam/Mod/MilCAM/InitGui.py` `keep` set       |
| "Bir FreeCAD modulu daha gerek"         | `CMakeLists.txt` `BUILD_<X>` flag'ini ON yap     |
| "Font/ikon boyutu"                      | `Mod/MilCAM/InitGui.py` `Activated()`           |
| "Drop folder default'u degissin"        | `CodesysBridge::Impl::dropFolder` (C++)         |
| "OPC UA sembol adi degissin"            | `.ai/CODESYS_BRIDGE.md` + Faz 4 implementasyon  |

## Eger Kullanici "FreeCAD UI'sini istemiyorum, yeni bir UI olsun" derse

DUR. Bunu derseler proje yon degistiriyor demektir. Tartisip kullanici ile
dogrula:
- CADNC mimarisine geri mi donmek istiyorlar?
- MilCAM iptal edilip CADNC mi gelistirilsin?

Yeni UI yazimi `.ai/START_HERE.md` "Kritik Kurallar 1" maddesi ile celiskili.
Kullanici acik direktif vermeden bu yola **gitme**.

## Eger Kullanici "Workbench overlay hizla iter etmek istiyorum" derse

Overlay-only preset:

```bash
cmake --preset overlay-only
cmake --build build-overlay
sudo cmake --install build-overlay
# (Mod/MilCAM /usr/share/freecad/Mod/MilCAM'e gider)
freecad                              # sistem freecad ile test
```

Tek build cikar (`CodesysBridge.so`), Python kod copy edilir.

## Eger Kullanici "Cihaza deploy et" derse

Bu Faz 5. Once Faz 1-2-3-4 sirayla tamamlanmis olmali. Cihaz erisimi (SSH,
parola) icin user'a sor — production credentials .ai/'ya yazma.

## Kritik Hatirlatici

- Kullanici: Türkçe konus.
- Kod ve yorumlar: English.
- "DO NOT build custom UI" → MilCAM mimarisinin temel kurali.
- FreeCAD kaynagina yama yok — sadece CMake flag + Python overlay + preferences.
- CAM/postprocessor değişiklikleri: FreeCAD upstream'in patternine sadik kal.
