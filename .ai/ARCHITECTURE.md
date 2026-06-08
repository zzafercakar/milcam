# MilCAM Architecture

## Sistem Bağlamı

```
                ┌─────────────────────────────────────────────┐
                │           VEC-VE 12" panel PC               │
                │           Linux/ARM64 + X11                 │
                │                                              │
   USB tools   │ ┌─────────────────┐    ┌──────────────────┐ │
   USB stock   │ │  CodeSys 3.5    │    │  MilCAM (Qt6)    │ │
   ──────────► │ │  - RTE          │    │  - UI shell      │ │
   EtherCAT    │ │  - TargetVisu   │◄──►│  - Import        │ │
   ───────────►│ │  - SoftMotion   │    │  - CAM           │ │
   Modbus      │ │    CNC          │    │  - CodesysBridge │ │
   ───────────►│ └────┬──────┬─────┘    └──────┬──────┬────┘ │
                │     │      │  OPC UA          │      │      │
                │     │      └──────────────────┘      │      │
                │     │  /var/cnc/jobs/*.gcode         │      │
                │     └────────────────────────────────┘      │
                │                                              │
                └─────────────────────────────────────────────┘
                                  ▲
                                  │ HDMI/RGB + touch + ethernet
                                  │
                ┌─────────────────────────────────────────────┐
                │      Operator (dokunmatik ekran, klavyesiz) │
                └─────────────────────────────────────────────┘
```

## Process Topology

MilCAM, CodeSys ile aynı kutuda **ayrı user-space process'ler** olarak çalışır.
Tek bir X server (genelde `:0`) ikisini de host eder.

| Process               | Role                              | Lifetime    |
| --------------------- | --------------------------------- | ----------- |
| `codesyscontrol`      | PLC runtime (RT öncelikli)        | systemd     |
| `codesystargetvisu`   | HMI (X11 client, pencereli)       | systemd     |
| `milcam`              | CAM uygulaması                    | systemd     |
| `milcam-launcher`     | Üst şerit, HMI↔CAM switch         | systemd     |
| `openbox` veya `sway` | Window manager                    | session     |

## Layered Architecture (process-içi)

```
┌──────────────────────────────────────────────────────────┐
│  QML UI                                                   │
│  ui/qml/Main.qml — touch-first 56px controls              │
│  Qt VirtualKeyboard — text input                          │
├──────────────────────────────────────────────────────────┤
│  Adapter (C++20, namespace MilCAM)                        │
│  ┌────────────────┐ ┌────────────────┐ ┌──────────────┐   │
│  │ ImportFacade   │ │ CamFacade      │ │ CodesysBridge│   │
│  │ STEP/IGES/...  │ │ ops + post     │ │ OPC UA+drop  │   │
│  └────────┬───────┘ └────────┬───────┘ └──────┬───────┘   │
│           │                  │                │           │
│  ┌────────▼──────────────────▼──┐  ┌──────────▼───────┐   │
│  │ CadDocument (FreeCAD Document │  │ open62541 client │   │
│  │   wrapper, import-only mode)  │  └──────────────────┘   │
│  └────────┬──────────────────────┘                         │
├───────────┼────────────────────────────────────────────────┤
│  FreeCAD slim subset (LGPL)                                │
│  freecad/Base — type system, math, I/O                     │
│  freecad/App  — Document, Property, Transaction            │
│  freecad/Mod/Material/App — material catalogue             │
│  freecad/Mod/Part/App     — TopoShape, importers           │
├──────────────────────────────────────────────────────────┤
│  OpenCASCADE 7.6+                                          │
│  STEPCAFControl_Reader, IGESCAFControl_Reader, StlAPI, …  │
└──────────────────────────────────────────────────────────┘
```

## Data Flow (Tipik İş Akışı)

```
1. Operator MilCAM açar (HMI'dan veya boot'tan)
2. Import → "şu STEP'i aç" (USB veya NFS mount)
3. ImportFacade.importFile()
   ├─► STEPCAFControl_Reader (OCCT)
   ├─► TopoDS_Shape oluştu
   └─► CamGeometrySource'a bağlandı, viewport'ta render
4. Operator: Operations → "Profile" ekle, takım seç, derinlik gir
   ├─► Qt VirtualKeyboard sayısal değer girişinde otomatik açılır
   └─► Toolpath solver çalıştı, viewport'ta önizleme
5. Post + G-code → CodesysPostProcessor
   └─► Bellek içinde G-code string'i oluşur
6. Send to CodeSys
   ├─► CodesysBridge.dropGCode("job_42", gcode)
   │   └─► /var/cnc/jobs/job_42.gcode.tmp → atomic rename → job_42.gcode
   └─► CodesysBridge.notifyJobReady(path)
       └─► OPC UA: MilCAM.JobReadyPath = "/var/cnc/jobs/job_42.gcode"
7. Operator HMI butonuna basar → wmctrl TargetVisu'yu öne getirir
8. PLC HMI'da "Load Job" butonu → SMC_ReadNCFile2 → SMC_NCInterpreter
9. Çalıştırma sırasında MilCAM PLC.CurrentLine'a subscribe → ilerlemeyi izler
```

## Önemli Tasarım Kararları

### Niye iki ayrı process (CodeSys + MilCAM), tek değil?

CodeSys runtime closed-source ve real-time öncelikli. MilCAM Qt6 + OCCT.
Bunları aynı process'e tıkmak ne mümkün ne istenir. Ayrı process model
şunları sağlar:

- **Crash isolation** — MilCAM çökmesi PLC'yi düşürmez.
- **Independent updates** — MilCAM güncellenirken CodeSys çalışmaya devam eder.
- **RT/non-RT ayrımı** — `isolcpus`+`taskset` ile CodeSys izole çekirdeklere
  pinlenir, MilCAM kalan çekirdeklerde döner.

### Niye OPC UA + drop folder, tek protokol değil?

İkisi farklı şeyler için:
- **Drop folder** = büyük binary G-code datası. Atomic rename garantisi.
- **OPC UA** = küçük durum değişiklikleri, real-time. JobReadyPath sinyali +
  CurrentLine takibi.

Drop folder olmadan G-code'u OPC UA üzerinden göndermek de mümkün ama büyük
string'leri symbol'a yazmak protokol kötüye kullanımı.

### Niye Qt VirtualKeyboard, sistem OSK (onboard) değil?

- Sistem OSK X11 input method olarak çalışır, geçişlerde gecikir.
- Qt VirtualKeyboard QML scene graph'inde gömülü, gecikme yok.
- Tema/dil kontrolü tam — Türkçe layout dahil.
- CodeSys TargetVisu'nun kendi numeric keypad'i var ve onunla çakışmaz çünkü
  herkes kendi process'inde input alır.

### Niye Sketcher/PartDesign kaldırıldı?

Saha senaryosu: operator çizmiyor. Geometri ofisten gelir. Sketcher koymak:
- ~50K satır C++ + planegcs solver bagımlılığı
- Constraint UI eklemek = ekstra QML, ekstra ikon, ekstra eğitim
- Reliability riski: solver bug saha durdurur

Şu an MilCAM 0.1.0 scaffold'unda freecad/ kopyalanmaz Sketcher/PartDesign,
adapter'da hayalet dosyalar var (Faz 1'de silinecek).

## Threading Model

- **UI thread (main):** QML, event loop, kullanıcı girdileri.
- **Render thread:** Qt scene graph + OCCT AIS_InteractiveContext. *Hiçbir
  AIS çağrısı UI thread'inden yapılmamalı.*
- **Worker pool (`QThreadPool`):**
  - Import I/O — büyük STEP yüklemeleri.
  - Toolpath hesabı — CAM solver.
  - G-code post — sentezleme.
- **OPC UA thread (open62541 internal):** event-driven, MilCAM Qt signal'lara
  marshall eder (`QMetaObject::invokeMethod` ile).

## Memory & Disk Footprint Hedefleri (VEC-VE için)

| Bileşen                    | Hedef RSS  | Notlar                     |
| -------------------------- | ---------- | -------------------------- |
| MilCAM idle                | < 200 MB   | Boş job ile                |
| MilCAM + 100k face STEP    | < 600 MB   | Tipik orta-büyük parça     |
| Disk (binary + qml + libs) | < 500 MB   | aarch64 cross-build        |
| OCCT GL surface            | OpenGL 2.0+ veya yumuşak software fallback |
