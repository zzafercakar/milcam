# MilCAM — Start Here

> Bu dosyayi HER oturum basinda oku. Sonra sirayla: `context.yaml`,
> `DEVICE_PROFILE.md`, `SERIAL_CONSOLE_ACCESS.md`, `WORKPLAN.md`,
> `ARCHITECTURE.md`, `VEC_VE_TARGET.md`, `ENGINEERING_LOG.md`.

> **2026-06-09 — Cihaz erisimi acildi.** RS232 serial console uzerinden
> VEC-VE'ye baglandik, `/root/` icerigini gorduk, network probe yapildi.
> Detaylar: [DEVICE_PROFILE.md](DEVICE_PROFILE.md),
> [SERIAL_CONSOLE_ACCESS.md](SERIAL_CONSOLE_ACCESS.md),
> [NETWORK_PROBE_2026-06-09.md](NETWORK_PROBE_2026-06-09.md). Bir sonraki
> adim WORKPLAN.md "Faz 0.7 — Donanim Envanteri".

> **2026-06-10 — BÜYÜK PİVOT + İLERLEME.** Aşağıdaki "FreeCAD'in kendisidir"
> anlatımı ARTIK GEÇERLİ DEĞİL. Canlı cihaz testi FreeCAD 3B viewport'unun bu
> donanımda çalışmadığını kanıtladı (GL yok). MilCAM artık **standalone Qt 2.5B
> CAM**. Güncel gerçek: bu dosyanın "Bir Cümlede Proje (2026-06-10 güncel)"
> bölümü + [DISPLAY_ARCHITECTURE_2026-06-09.md](DISPLAY_ARCHITECTURE_2026-06-09.md)
> + spec/plan (`docs/superpowers/`).
> **Durum:** P2 (CODESYS DIN 66025 post) TAMAMLANDI ve aarch64'e cross-derlenip
> **cihazda `ALL PASS` ile doğrulandı** (statik core; [TOOLCHAIN_NOTES.md](TOOLCHAIN_NOTES.md)).
> Sıradaki görsel adım: **Phase 0.3 "Hello MilCAM" linuxfb penceresi** (henüz
> yok → ekranda hâlâ SMB boot logosu görünüyor; fb0'a çizen uygulama deploy
> edilmedi).

## Bir Cümlede Proje (2026-06-10 güncel)

MilCAM, CodeSys CNC panel PC'leri için **standalone bir Qt Widgets 2.5B CAM**
uygulamasıdır: linuxfb üzerinde (GL'siz, raster) çalışır, DXF + dahili şekillerden
Profile/Pocket/Drill toolpath üretir, **CODESYS DIN 66025** `.cnc` çıkarır ve
çalışan CodeSYS runtime'ın okuması için drop folder'a bırakır. GL/Qt'siz bir C++
core (geometry/DXF/CAM/post) host'ta TDD ile test edilir.

---
## (RETIRED) Eski Bir Cumlede Proje — slim FreeCAD modeli

> Aşağıdaki anlatım 2026-06-08–10 arası geçerliydi; 2026-06-10 pivotuyla terk
> edildi. Sadece bağlam için tutuluyor.

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

## Mevcut Durum (2026-06-09, akşam revizyon)

- **Faz 0 (eski iskelet) — IPTAL.** Yanlis mimari (kendi QML UI), tamamen silindi.
- **Faz 0.5 — Yeni iskelet:** TAMAMLANDI 2026-06-08. CodesysBridge C++ + CPython binding,
  Mod/MilCAM Python overlay, CMakeLists wraps FreeCAD with BUILD flags,
  postprocessor + 3 komut + 3 ikon, pytest smoke.
- **Faz 0.6 — Cihaz erisimi:** TAMAMLANDI 2026-06-09 ogle. RS232 serial console
  (115200 8N1 no flow control). `ls -l /root` ve network probe yapildi.
- **Faz 0.7 — Donanim envanteri:** ✅ TAMAMLANDI 2026-06-09 aksam.
  **Plan A onaylandi** — RAM 2 GB, Disk 12.6 GB, aarch64 Cortex-A53 ×4,
  NXP i.MX 8M SoC, PREEMPT-RT kernel, DRM/KMS DSI panel. Detay:
  [DEVICE_PROFILE.md](DEVICE_PROFILE.md).
- **Faz 0.8 — Cihaz hazirligi:** 🔜 SIRADAKI. CODESYSControl.cfg'ye
  `[SysProcess]` ekle + dropbear cross-build + TargetVisu windowed
  konfig + wmctrl kontrol.
- **Faz 1 — Build dogrulamasi:** Faz 0.8 sonrasi. End-to-end build hic denenmedi.
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
