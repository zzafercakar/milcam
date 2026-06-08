# FreeCAD Modul Alt Kumesi — Hangi Modul ON, Hangi OFF?

MilCAM, FreeCAD'in CMake `BUILD_<X>` flag'lerini overrider eder. Bu dosya
hangi flag hangi modulu kontrol ediyor ve niye dahil/haric ettigimizi
belgeler.

## ON Tutulanlar (CAM is akisi icin elzem)

| Flag                | Niye                                                          |
|---------------------|---------------------------------------------------------------|
| `BUILD_GUI`         | FreeCAD penceresinin kendisi. Olmazsa command-line only.      |
| `BUILD_PART`        | STEP/IGES/BREP import + Part::TopoShape                        |
| `BUILD_PART_DESIGN` | Body / parametrik feature — CAM bunlardan face seçer          |
| `BUILD_SKETCHER`    | 2D sketches — bazi Profile/Engrave ops sketches gerektirir    |
| `BUILD_CAM`         | THE CAM workbench (FreeCAD 1.0+ adlandirma; eskiden BUILD_PATH)|
| `BUILD_DRAFT`       | DXF import + 2D primitives. CAM Profile ops DXF'ten beslenebilir |
| `BUILD_MESH`        | STL import                                                    |
| `BUILD_MESH_PART`   | Mesh ↔ Part interop (STL → surface dönüsümü)                  |
| `BUILD_MATERIAL`    | Part'in zorunlu bagimliligi                                   |
| `BUILD_SPREADSHEET` | Tool tables, job parametre tablosu                            |
| `BUILD_MEASURE`     | Operatorun mesafe/açı kontrolü için                           |
| `BUILD_IMPORT`      | STEP/IGES/BREP importer (Part'in I/O katmani)                 |

## OFF Yapilanlar (kapsamim disinda — agirlik kazanci)

| Flag                  | Modul          | Niye OFF                                  |
|-----------------------|----------------|-------------------------------------------|
| `BUILD_FEM`           | FEM            | Sonlu eleman analizi — CAM is akisinda yok |
| `BUILD_BIM`           | BIM            | Yapı tasarımı                              |
| `BUILD_ASSEMBLY`      | Assembly       | Coklu part montaji — CAM kapsamı dışı       |
| `BUILD_TECHDRAW`      | TechDraw       | 2D teknik resim üretimi — CAM is akisi yok |
| `BUILD_DRAWING`       | Drawing        | Eski TechDraw'ın deprecated halefi          |
| `BUILD_SURFACE`       | Surface        | Ileri yuzey modelleme — CAM'in min temasi gerek |
| `BUILD_OPENSCAD`      | OpenSCAD       | OpenSCAD interop                            |
| `BUILD_PLOT`          | Plot           | Matplotlib plotting                         |
| `BUILD_ROBOT`         | Robot          | Robot kinematik simulasyon                  |
| `BUILD_WEB`           | Web            | QtWebEngine — agir + saha cihazinda anlamsiz |
| `BUILD_HELP`          | Help           | QtWebEngine — yine agir + kiosk modda anlamsiz |
| `BUILD_START`         | Start          | Start page — kiosk default WB ile gereksiz |
| `BUILD_INSPECTION`    | Inspection     | Mesh karşılaştırma                          |
| `BUILD_JTREADER`      | JtReader       | Siemens JT format (default OFF zaten)       |
| `BUILD_MATERIAL_EXTERNAL` | -          | Harici materyal DB'leri                     |
| `BUILD_POINTS`        | Points         | Nokta bulutu                                |
| `BUILD_REVERSEENGINEERING` | RE        | Ters muhendislik                            |
| `BUILD_SHOW`          | Show           | Görünürlük automation                       |
| `BUILD_TUX`           | Tux            | Tema/palette araci                          |
| `BUILD_ADDONMGR`      | AddonManager   | Saha cihazina addon kurmuyoruz              |
| `BUILD_CLOUD`         | Cloud          | Bulut entegrasyonu (zaten default OFF)      |
| `BUILD_VR`            | VR             | Oculus support (zaten default OFF)          |
| `BUILD_TEST`          | Test           | FreeCAD'in test workbench'i                 |
| `BUILD_SANDBOX`       | Sandbox        | Dev test                                    |
| `BUILD_TEMPLATE`      | Template       | Dev test                                    |
| `BUILD_FLAT_MESH`     | -              | Mesh flattening (mesh modulu zaten yetiyor) |
| `BUILD_DESIGNER_PLUGIN`| -             | Qt Designer plugin (dev)                    |
| `BUILD_TRACY_FRAME_PROFILER` | -       | Tracy profiler                              |

## Build Boyut Hedefi

Stock FreeCAD 1.2 install: **~3.5 GB** (lib + share + Python deps).
MilCAM hedef: **< 1.5 GB** (modul stripleme + agir QtWeb dep cikartma).

Asagidaki agir bilesenleri OFF yaparak Qt Web Engine ve Netgen'den
kurtuluyoruz:
- `BUILD_HELP=OFF`  → QtWebEngineCore, QtWebEngineWidgets gerekmez
- `BUILD_WEB=OFF`   → QtWebEngineCore gerekmez
- `BUILD_FEM=OFF`   → Netgen ve VTK gerekmez (cok agir)
- `BUILD_BIM=OFF`   → IfcOpenShell gerekmez

Bu dort modul tek basina ~1.5 GB'lik dep azaltir.

## Inter-Module Bagimlilik Riskleri

FreeCAD'in modulleri kismi gevsek baglidir ama bazi tek-yon bagimliliklar
var:
- `Inspection`  → `Mesh`, `Part`. (Inspection OFF, dolayli sorun yok)
- `BIM`         → `Arch`, `Draft`. (BIM OFF; Draft ON kaliyor)
- `Robot`       → `Part`. (Robot OFF; Part ON kaliyor)
- `Mesh_Part`   → `Mesh`, `Part`. (ikisi de ON)
- `Path/CAM`    → `Part`, `PartDesign`, `Sketcher`, `Draft`. **Hepsi ON**.
- `TechDraw`    → `Part`, `Draft`. (TechDraw OFF, sorun yok)

Faz 1'de gercek build'e gectigimizde inter-module link hatalari cikabilir
— o zaman bu listeyi guncelle.

## Eger Bir Module Geri Eklemek Gerekirse

Mesela kullanici "DXF cizebileyim" derse Draft yetersizse ve Sketcher acik
hali gerekirse: Sketcher zaten ON. Tum Mod/MilCAM workbench yanindaki
"keep" listesinde Sketcher var. Yani gizli ama eğer ihtiyac olursa
Workbench Selector'de "Sketcher" workbench'i secip kullanmasi mumkun
(operatore kapali, gelistirici-mod).

DXF yazmak (export) gerekirse: Draft'in DXF export'u var, zaten ON.

3D printing (Slic3r integration) gerekirse: ayri workbench, MilCAM
scope'unda yok.

## Asimi Kontrol Mekanizmasi

Build sonrasi `find install/usr/local/lib -name "*.so" | xargs du -sh`
calistir. Beklenmedik bir Mod*.so cikiyorsa BUILD_<X> flag'ini bir kere
daha goz at.
