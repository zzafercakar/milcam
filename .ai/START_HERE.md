# MilCAM — Start Here

> Bu dosyayi HER oturum basinda oku. Sonra `context.yaml`, `WORKPLAN.md`,
> `ENGINEERING_LOG.md`, ve `VEC_VE_TARGET.md` dosyalarini oku.

## Proje Nedir?

MilCAM, **CodeSys CNC panel PC'leri** (VEC-VE 12" sinif) icin tasarlanmis,
**dokunmatik oncelikli** bir CAM uygulamasidir. STEP/IGES/BREP/DXF/STL gibi
notr dosyalari okur, uzerinde Profile/Pocket/Drill/Facing/Adaptive/Engrave
operasyonlari kurar, **CODESYS post-processor** ile G-code uretir ve sonucu
ayni cihazda calisan CodeSys CNC kontrolcusune **OPC UA + drop folder** ile
teslim eder.

**MilCAM CAD degildir.** Sketcher, PartDesign, parametrik feature agaci, 2D
constraint solver YOK. Geometri sadece **okunur** — saha operatoru cizmez,
ofis tasarlar, MilCAM uretir.

## Niye Var?

| Sorun                                              | MilCAM cevabi                         |
| -------------------------------------------------- | ------------------------------------- |
| Full CAD (FreeCAD, CADNC) panel PC'de cok agir     | Sadece CAM tasinir; ~300-600 MB hedef |
| Dokunmatik ekran, klavyesiz operator               | Qt Virtual Keyboard, 56px buton boyu  |
| HMI ile yan yana calismasi gerek                   | TargetVisu + MilCAM ayni X server     |
| G-code'u PLC'ye ulastirma                          | OPC UA notify + atomic drop folder    |
| Real-time PLC'yi bozmamak                          | `isolcpus`, IRQ pinning disiplini     |

Detayli senaryolar ve karar kayitlari icin `.ai/VEC_VE_TARGET.md`.

## Kardes Projeler

| Proje    | Konum                          | MilCAM ile iliski                                   |
| -------- | ------------------------------ | --------------------------------------------------- |
| **CADNC**| `/home/embed/Dev/CADNC/`       | Full CAD+CAM workstation. CAM modulunun ust kaynagi |
| **MilCAD**| `/home/embed/Dev/MilCAD/`     | Onceki nesil CAD; CAM ilk burada gelistirildi       |
| **FreeCAD ref**| `/home/embed/Downloads/FreeCAD-main-1-1-git/` | Upstream LGPL kaynak  |

CAM modulunde anlamli bir bugfix bulursan, **CADNC'ye de portla**. CAM koduna
butch divergence olusmamali. Detay: `.ai/REFERENCE_SOURCES.md`.

## Mimari (Bir Bakista)

```
┌──────────────────────────────────────────────────────────┐
│  QML UI Shell  +  Qt VirtualKeyboard                     │
│  ui/qml/Main.qml — touch-first, 56px buttons             │
├──────────────────────────────────────────────────────────┤
│  Adapter (C++)                                           │
│  CamFacade, ImportFacade, CodesysBridge,                 │
│  PartFacade (import-only mode), CadDocument              │
├──────────────────────────────────────────────────────────┤
│  CAM Module        FreeCAD slim subset                   │
│  Profile/Pocket/   Base, App, Material, Part             │
│  Drill/Facing/...  (LGPL)                                │
│  CODESYS post                                            │
├──────────────────────────────────────────────────────────┤
│  OpenCASCADE (OCCT) Kernel                               │
└──────────────────────────────────────────────────────────┘

Cevre:
  CodesysBridge ──┬── OPC UA  ──→  CodeSys 3.5 runtime
                  └── drop folder /var/cnc/jobs/*.gcode
```

## Kritik Kurallar (ASLA IHLAL ETME)

1. **UI kodu FreeCAD header'i include etmez** — sadece adapter/ uzerinden.
2. **AIS_InteractiveContext islemleri SADECE render thread'de.**
3. **Linux'ta EGL KULLANILMAZ** — OCCT GLX ile derlenmis.
4. **Sketcher / PartDesign / Nesting tekrar EKLENMEZ** — kapsam disi.
5. **Her interaktif kontrol >= 48px** (hedef 56px). Touch-first sart.
6. **Fiziksel klavye varsayma** — Qt VirtualKeyboard.
7. **Real-time disiplini** — bkz. `.ai/VEC_VE_TARGET.md` "Jitter discipline".
8. **freecad/ alti minimum degisiklik** — upstream uyumlulugu.

## Dizin Yapisi

```
MilCAM/
├── .ai/           # Bu dosyalar (Claude memory)
├── .vscode/       # VS Code workspace + launch + tasks
├── adapter/       # CadDocument, PartFacade, CamFacade, ImportFacade, CodesysBridge
├── app/           # main.cpp + AppVersion.h.in
├── cam/           # 8 operation type, post-processors (CADNC'den inherit)
├── doc/           # User docs
├── freecad/       # SLIM subset: Base, App, Material, Part (Sketcher/PartDesign YOK)
├── resources/     # SVG ikonlar, logolar
├── scripts/       # Cross-compile toolchain, deploy/rsync helpers
├── tests/         # Unit + smoke testler
├── ui/qml/        # Touch-first QML UI
├── util/          # Yardimci fonksiyonlar
└── viewport/      # OCCT V3d_Viewer + QQuickFramebufferObject
```

## Build & Run

```bash
cmake --preset default
cmake --build build -j$(nproc)
DISPLAY=:0 QT_QPA_PLATFORM=xcb ./build/milcam

# Test
ctest --test-dir build --output-on-failure

# Panel PC cross-build
cmake --preset panel-pc
cmake --build build-aarch64 -j$(nproc)
```

## Mevcut Durum (2026-06-08)

- **Faz 0 — Iskelet:** TAMAMLANDI bugun. CADNC'den CAM, util, viewport, adapter
  kopyalandi. FreeCAD subset (Base+App+Material+Part) yerlestirildi. Touch QML
  shell ve `CodesysBridge` stub'u var.
- **Faz 1 — Adapter slimming:** SIRADAKI. SketchFacade/SketchDocument/NestFacade
  dosyalari adapter/'da kalmis durumda; CMake bunlari build'e dahil etmiyor ama
  silinmeleri lazim.
- **Faz 2 — Import wiring:** Bekliyor. `ImportFacade::importFile` su an stub.
- **Faz 3 — OPC UA + drop:** Bekliyor. `CodesysBridge::connect` su an stub.
- **Faz 4 — VEC-VE deploy:** Bekliyor. SSH ile cihaza erisim, hardware check.
- **Faz 5 — Saha kabul testi:** Bekliyor.

Detayli plan: `.ai/WORKPLAN.md`.

## Iletisim Tonu

- Kod ve yorumlar: **English**
- Kullanici ile iletisim: **Turkish**
- Memory dosyalari (`.ai/*`): **mixed** — Turkish anlatim, English teknik terim
