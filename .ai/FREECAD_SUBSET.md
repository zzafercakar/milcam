# FreeCAD Slim Subset — What MilCAM Carries

CADNC tüm büyük FreeCAD App modüllerini (`Base`, `App`, `Material`, `Part`,
`Sketcher`, `PartDesign`) inşa eder. MilCAM bilinçli olarak daha azını alır.

## Dahil Edilen Modüller

| Modül                  | Niçin gerekli                                    | Konum                       |
| ---------------------- | ------------------------------------------------ | --------------------------- |
| `freecad/Base`         | Math, IO, type system, Python interpreter init   | `freecad/Base/`             |
| `freecad/App`          | Document, Property, Transaction (Part'ın altyapı)| `freecad/App/`              |
| `freecad/Mod/Material/App` | Part'ın bağımlılığı (material catalogue)     | `freecad/Mod/Material/App/` |
| `freecad/Mod/Part/App` | TopoShape, STEP/IGES/BREP importers, geometry   | `freecad/Mod/Part/App/`     |

`freecad/3rdParty/` içinden tutulan:
- `PyCXX` — Python C++ binding (Base bağımlılığı)
- `zipios++` — ZIP IO (Base Reader/Writer için)
- `FastSignals` — sinyal kütüphanesi
- `json` — JSON (App'in document IO için)
- `lru` — LRU cache helpers
- `OndselSolver` — **gerçekte SADECE Sketcher tarafından kullanılır** ama
  cmake glob ile geldiği için bırakıldı; ileride budanabilir.

## Hariç Bırakılanlar (KASITLI)

| Modül                  | Niçin bırakıldı                                           |
| ---------------------- | --------------------------------------------------------- |
| `freecad/Mod/Sketcher`  | MilCAM 2D constraint geometry yazmaz                     |
| `freecad/Mod/PartDesign`| Parametrik feature tree MilCAM scope dışı                |
| `freecad/Gui`           | MilCAM kendi Qt6 UI'sını kullanır                        |
| `freecad/Tools`         | Build/release araçları, runtime'da gerek yok             |
| `freecad/Build`         | CMake build artefactları                                 |

## CMake Etkisi

`freecad/CMakeLists.txt` içinde:
```cmake
add_subdirectory(Base)
add_subdirectory(App)
add_subdirectory(Mod/Material/App)
add_subdirectory(Mod/Part/App)
# Sketcher + PartDesign deliberately not included.
```

Üst CMakeLists'ta `milcam_adapter` link satırı:
```cmake
target_link_libraries(milcam_adapter PUBLIC
    # NB: Sketcher / PartDesign deliberately NOT linked.
    Part Materials FreeCADApp FreeCADBase
    Qt6::Core Qt6::Quick
)
```

## Sonuç: Boyut Tasarrufu

| Build                | Disk    | RAM idle (tahmin) |
| -------------------- | ------- | ----------------- |
| CADNC full           | ~83 MB  | ~250 MB           |
| MilCAM slim          | ~50 MB (hedef) | ~150 MB    |

Sketcher tek başına `~48K satır`, PartDesign `~20K satır`. Disk +
RAM kazancı belirgin.

## CADNC ile Senkronizasyon Disiplini

CADNC `freecad/` ağacı upstream FreeCAD 1.2'den alındı. MilCAM `freecad/` aynı
ağacın **subset'i**. Bu yüzden:

1. CADNC upstream rebase yaparsa, MilCAM da aynı `Base/`, `App/`,
   `Mod/Material/`, `Mod/Part/` dosyalarını alır.
2. Sketcher veya PartDesign **DAHA SONRA EKLEME ZORUNLULUĞU OLURSA**
   (kapsam ekleme), CADNC'den olduğu gibi getirilir.
3. MilCAM `freecad/` içinde kendi başına değişiklik yapma — değişiklikler
   önce CADNC'ye, sonra MilCAM'e portla.

## Upgrade Pathway: Sketcher Geri Eklenirse?

Kararı tetikleyen olası senaryo: operator sahada basit 2D çizim de yapmak
istiyorsa. Adımlar:
1. `MILCAM_ENABLE_SKETCHER=ON` flag'i CMakeLists'a ekle.
2. CADNC'den `freecad/Mod/Sketcher/App/` ve `freecad/3rdParty/OndselSolver/`
   kopyala.
3. `freecad/CMakeLists.txt`'e `add_subdirectory(Mod/Sketcher/App)` koy.
4. Adapter'da silmeden bıraktığın `SketchFacade.h` dosyalarını geri getir.
5. UI'a Sketcher toolbar ekle.

Karar verilmeden önce: footprint büyür (~50 MB disk + ~50 MB RAM),
karmaşıklık artar, kullanıcı eğitimi gerek. Default cevap: **HAYIR**, ofis
çizsin.
