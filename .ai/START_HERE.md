# MilCAM — Start Here

> Bu dosyayi HER oturum basinda oku. Sonra `context.yaml`, `WORKPLAN.md`,
> `ARCHITECTURE.md`, `VEC_VE_TARGET.md` dosyalarini oku.

## Bir Cumlede Proje

MilCAM, **FreeCAD'in kendisidir** — sadece CAM disindaki modullerin compile
edilmedigi, uzerine ince bir overlay ile (i) CAM disindaki workbench'lerin
gizlendigi, (ii) CodeSys'e G-code teslimat butonu eklendigi, (iii) CODESYS
postprocessor'i yerlestirildigi slim bir surumu.

**MilCAM yeni bir uygulama DEGILDIR.** Yeni bir UI yapilmiyor, yeni bir
3D viewport yazilmiyor, yeni bir geometri motoru yok. FreeCAD birebir
kullaniliyor, sadece **hafifletiliyor**.

## Niye Yeniden Yazmak Yerine FreeCAD Kullaniyoruz?

Bir CAD/CAM UI'sini sifirdan yazmak yillarca surer. Operatorun ihtiyaci
basit: dokunmatik ekranda STEP ac, profil/cep/delik operasyonu kur, G-code
uret, makineye gonder. FreeCAD'in CAM workbench'i bu isi yapiyor zaten.
Eksigimiz tek sey: (1) digerleri operatoru sasirtmasin, (2) "Gonder"
butonu CodeSys icin uretsin. Bu iki ihtiyac sirasiyla **CMake flag**
ve **Python workbench overlay** ile karsilanir.

## Mimari (Bir Bakista)

```
  FREECAD_SOURCE_DIR  (yerel FreeCAD klonu — /home/embed/Downloads/...)
        │
        │  add_subdirectory()  ile MilCAM'in CMakeLists'inden cagirilir
        ▼
  ┌──────────────────────────────────────────────────────┐
  │  FreeCAD upstream                                     │
  │  Modulleri: Base, App, Gui                            │
  │  Workbench'ler: Part, PartDesign, Sketcher, CAM,      │
  │                 Draft, Mesh, Spreadsheet, Measure     │
  │                 (digerleri BUILD_<X>=OFF ile DISARDA) │
  └──────────────────────────────────────────────────────┘
        │
        │  Install
        ▼
  $prefix/share/freecad/Mod/MilCAM/    ← MilCAM overlay
       ├── Init.py  / InitGui.py        (workbench gorunurlugu)
       ├── Commands/SendToCodesys.py    (post + drop + OPC UA)
       ├── Commands/SwitchToHmi.py      (wmctrl)
       ├── PostProcessor/codesys_post.py
       ├── Resources/icons/*.svg
       └── CodesysBridge.so             (C extension)
```

Kullanici FreeCAD'i acar → MilCAM workbench varsayilan acilir → digerleri
yok / gizli → "Send to CodeSys" basinca G-code dropuyor.

## Sahiplik Tablosu (Bunu Karistirma)

| Ne                          | Sahip                                       |
|-----------------------------|---------------------------------------------|
| Pencere, menuler, agac      | FreeCAD upstream                            |
| 3D viewport                 | FreeCAD upstream                            |
| Sketcher / PartDesign       | FreeCAD upstream                            |
| CAM workbench               | FreeCAD upstream                            |
| Modul aktif/pasif flag'leri | **MilCAM** (root CMakeLists)                |
| Workbench gorunurlugu       | **MilCAM** (`Mod/MilCAM/InitGui.py`)        |
| Send-to-CodeSys komutu      | **MilCAM** (`Mod/MilCAM/Commands/`)         |
| CODESYS postprocessor       | **MilCAM** (`Mod/MilCAM/PostProcessor/`)    |
| OPC UA + drop folder        | **MilCAM** (`milcam/CodesysBridge/`)        |
| Touch ayarlari (font, OSK)  | **MilCAM** (`InitGui.py` Activated())       |

## Kardes Projeler

| Proje  | Durumu                                                  |
|--------|--------------------------------------------------------|
| CADNC  | Hala /home/embed/Dev/CADNC/ — full custom QML+OCCT CAD. MilCAM eskiden CADNC'nin kopyasiydi. **Artik degil.** CADNC ile MilCAM **kod paylasmiyor.** |
| MilCAD | /home/embed/Dev/MilCAD/ — eski donem CAD, sadece referans. |

## Kritik Kurallar (ASLA IHLAL ETME)

1. **YENI BIR Qt/QML UI YAZILMAZ.** FreeCAD UI birebir kullanilir.
2. **FREECAD KAYNAGINA YAMA YAPILMAZ.** Sadece CMake flag, runtime overlay,
   preferences ile mudahale edilir.
3. **`Mod/MilCAM/` saf Python'dur.** C++ olusturmuyoruz workbench tarafinda.
4. **`CodesysBridge` saf C++** (FreeCAD/Qt dep yok), CPython stable ABI ile
   bind edilir. FreeCAD versiyon atlamalarinda kirilmaz.
5. **Linux'ta GLX only** (OCCT GLX ile compile edilmis).
6. **Touch-first ekleme yapiyorsan ≥48px buton**, klavye varsayma.

## Build & Run

```bash
# Yerel FreeCAD klonun yoksa:
git clone https://github.com/FreeCAD/FreeCAD.git ~/Downloads/FreeCAD

cmake --preset default \
      -DFREECAD_SOURCE_DIR=$HOME/Downloads/FreeCAD
cmake --build build -j$(nproc)
DESTDIR=$PWD/_inst cmake --install build
DISPLAY=:0 QT_QPA_PLATFORM=xcb _inst/usr/local/bin/FreeCAD
```

Sadece overlay'i degistirip hizli iterasyon:

```bash
cmake --preset overlay-only
cmake --build build-overlay
sudo cmake --install build-overlay
```

## Mevcut Durum (2026-06-08, akşam revizyon)

- **Faz 0 (eski iskelet) — IPTAL.** Yanlis mimari (kendi QML UI), tamamen silindi.
- **Faz 0.5 — Yeni iskelet:** TAMAMLANDI. CodesysBridge C++ + CPython binding,
  Mod/MilCAM Python overlay, CMakeLists wraps FreeCAD with BUILD flags,
  postprocessor + 3 komut + 3 ikon, pytest smoke.
- **Faz 1 — Build dogrulamasi:** SIRADAKI. End-to-end build hic denenmedi.
  FreeCAD'in `add_subdirectory` ile cagirildiginda BUILD flag'lerinin
  beklenildigi gibi calistigi dogrulanmali.
- **Faz 2 — Workbench gorunurluk testi:** Bekliyor.
- **Faz 3 — Branding (splash, app name, default WB):** Bekliyor.
- **Faz 4 — OPC UA gercek baglantisi:** Bekliyor.
- **Faz 5 — VEC-VE deploy + saha:** Bekliyor.

Detay: `.ai/WORKPLAN.md`.

## Iletisim Tonu

- Kod ve yorumlar: English
- Kullanici ile iletisim: Turkish
- `.ai/` belgeleri: Turkish anlatim + English teknik terim
