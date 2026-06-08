# MilCAM Architecture

## Sistem Baglami

```
                ┌─────────────────────────────────────────────┐
                │           VEC-VE 12" panel PC               │
                │           Linux/ARM64 + X11                 │
                │                                              │
   USB tools   │ ┌─────────────────┐    ┌──────────────────┐ │
   USB stock   │ │  CodeSys 3.5    │    │  MilCAM          │ │
   ──────────► │ │  - RTE          │    │  = slim FreeCAD  │ │
   EtherCAT    │ │  - TargetVisu   │◄──►│  + overlay       │ │
   ───────────►│ │  - SoftMotion   │    │                  │ │
   Modbus      │ │    CNC          │    │  (FreeCAD'in     │ │
   ───────────►│ └────┬──────┬─────┘    │   kendi UI'si)   │ │
                │     │      │  OPC UA  └──────┬──────┬────┘ │
                │     │      └─────────────────┘      │      │
                │     │  /var/cnc/jobs/*.gcode        │      │
                │     └───────────────────────────────┘      │
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

MilCAM cihazda iki ayri process olarak calisir:
- `codesyscontrol` + `codesystargetvisu` (PLC + HMI, real-time-priority)
- `FreeCAD` (MilCAM, normal priority)

Ayni X server (genelde :0) ikisini de host eder.

## Build-Time Architecture

Bu MilCAM'in ozgun katmanlama'sidir — kac satir kod MilCAM yazmasi gerek
goz onunde tutmak icin.

```
┌──────────────────────────────────────────────────────────┐
│                MilCAM CMakeLists.txt                      │
│                                                            │
│  set(BUILD_FEM       OFF CACHE BOOL "" FORCE)             │
│  set(BUILD_BIM       OFF CACHE BOOL "" FORCE)             │
│  set(BUILD_TECHDRAW  OFF CACHE BOOL "" FORCE)             │
│  ... (~20 flag)                                            │
│                                                            │
│  add_subdirectory(${FREECAD_SOURCE_DIR}                   │
│                   ${CMAKE_BINARY_DIR}/freecad-build)      │
│  add_subdirectory(milcam/CodesysBridge)                   │
│  install(milcam/Mod/MilCAM/  →  $datadir/Mod/MilCAM/)     │
└──────────────────────────────────────────────────────────┘
         │                          │
         ▼                          ▼
┌──────────────────────┐   ┌──────────────────────────┐
│  FreeCAD upstream    │   │ milcam/CodesysBridge/    │
│  Base + App + Gui    │   │ — C++ pure (no FreeCAD,  │
│  Mod/Part, Sketcher, │   │   no Qt, no FreeCAD dep) │
│  PartDesign, CAM,    │   │ — CPython stable ABI     │
│  Draft, Mesh,        │   │ — installs as            │
│  Material, Spreadsheet│   │   CodesysBridge.so       │
│  Measure, Import     │   └──────────────────────────┘
│                      │
│  ~10 modul ON       │   ┌──────────────────────────┐
│  ~20 modul OFF      │   │ milcam/Mod/MilCAM/        │
└──────────────────────┘   │ — saf Python              │
                            │ — InitGui.py: hide WBs   │
                            │ — Commands/: 3 buton     │
                            │ — PostProcessor/codesys_ │
                            │   post.py                │
                            │ — Resources/icons/*.svg  │
                            └──────────────────────────┘
```

## Run-Time Architecture

Kullanici FreeCAD'i acar:

```
1. FreeCAD `main()` calisir (upstream)
2. FreeCAD `Mod/*/Init.py` ve `Mod/*/InitGui.py` tarar
   ├─ Mod/CAM/InitGui.py → CAM workbench register
   ├─ Mod/Part/InitGui.py → Part workbench register
   ├─ ...
   └─ Mod/MilCAM/InitGui.py → MilCAM workbench register
       │                       + hide non-CAM workbenches via prefs
       │                       + set AutoloadModule = MilCamWorkbench
       └─ import CodesysBridge (C extension)
3. Workbench picker default = MilCAM
4. Operator MilCAM acar:
   ├─ FreeCAD's standard 3-panel UI gosterilir (tree, viewport, properties)
   ├─ MilCAM toolbar gosterilir: [Send to CodeSys] [Switch to HMI]
   └─ CAM toolbar gosterilir: [Profile] [Pocket] [Drilling] ...
5. Operator akisi:
   a. File → Open → STEP yukle
   b. CAM → Job olustur, tool ekle
   c. CAM → Profile op ekle, depth/feed/stepover gir
   d. (Optional) Simulate
   e. MilCAM → Send to CodeSys
       └─ Mod/MilCAM/Commands/SendToCodesys.py calisir:
           1. CAM_Post komutunu codesys post ile cagir
           2. Olusan G-code'u oku
           3. CodesysBridge.drop_gcode(jobId, gcode)
              └─ /var/cnc/jobs/<jobId>.gcode (atomic rename)
           4. CodesysBridge.notify_job_ready(path)
              └─ OPC UA: MilCAM.JobReadyPath = path
   f. Operator MilCAM → Switch to HMI
       └─ wmctrl -a "TargetVisu" → TargetVisu one gelir
6. Operator HMI'da Run basar → PLC SMC_ReadNCFile2 ile G-code'u okur
```

## Tasarim Kararlari (FAQ)

### Niye FreeCAD'i fork etmiyoruz?

Fork = surekli rebase yuku. Module disable + Python overlay sayesinde
FreeCAD upstream'i hic dokunulmamis kalir. Upstream rebase her zaman
mumkun: `cd /home/embed/Downloads/FreeCAD-main-1-1-git && git pull`,
sonra MilCAM rebuild.

### Niye Mod/MilCAM/ saf Python? C++ daha hizli olmaz mi?

Workbench overlay islerinin tamami UI-level: command register, preferences
set, dialog ac, dosya yaz. Bunlar mikro-saniyelik isler degil. Python =
hizli iterasyon, FreeCAD'in pattern'ine uygunluk, eden-yiyene editlenebilir
overlay.

CPU-yogun isler (OPC UA, atomic write) C++'da: `CodesysBridge.so`.

### Niye PyCXX yerine CPython stable ABI?

PyCXX FreeCAD dahili kullaniliyor ama bagimliligi sabit degil (3rdParty/PyCXX
versiyonu FreeCAD ile birlikte degisir). Stable ABI ile yazdigimiz
`CodesysBridge.so` Python 3.x'in tamaminda calisir, FreeCAD versiyon
gecislerinde kirilmaz.

### Niye FreeCAD source vendorlanmiyor?

3 GB. Git icin uygunsuz. Yerel klonu referansliyoruz; production build
sirasinda CMake `FREECAD_SOURCE_DIR` parametresini alir.

### Workbench gizleme niye preferences yoluyla?

FreeCAD `Preferences > Workbenches > Disabled` listesini okur. Bizim
overlay startup'ta bu listeyi set eder. Operatoru sasirtan workbench
picker temizlenir. Compile-time strip + runtime hide combo ile maksimum
agirlik tasarrufu.

### Send-to-CodeSys neden tek butonda 4 is yapiyor?

Operator gozunden tek aksiyon: "is hazir, makineye gonder". Implementasyon:
post → dosya → drop → OPC UA notify. Buton arkasinda. UI'da bir sayfalik
dialog gostermek = operatoru yorar.

### Touch-first niye sadece font size?

FreeCAD UI Qt widgets uzerine kurulu — natively touchable, sadece sizing
sorun. InitGui.py Activated() icinde font.pointSize >= 12 ve
QT_IM_MODULE=qtvirtualkeyboard set ediyoruz. Gerisi (toolbar icon boyu,
dialog padding) FreeCAD'in style sheet'i ile post-deploy ayarlanabilir.

## Threading & RT Disiplini

- FreeCAD UI thread: tum FreeCAD operasyonlari (uzun import dahil)
  FreeCAD'in kendi worker pool'unda.
- CodesysBridge OPC UA thread (Faz 4): open62541 background thread.
  Python tarafa cross-thread `QMetaObject::invokeMethod` ile.
- CodeSys runtime ayri process, ayri RT-priority. MilCAM ile bellek
  paylasimi yok — sadece dosya + OPC UA.
- isolcpus disiplini: CodeSys cekirdeklerinde MilCAM CHALISMASIN.
  Systemd unit'inde `CPUAffinity=0 1` (non-RT cores), CodeSys'in unitinde
  `CPUAffinity=2 3` (RT cores).
