# Reference Sources

MilCAM kodu yazarken hangi projeleri açıp bakmalı?

## Birinci Derece (her gün lazım olabilir)

### CADNC
- **Konum:** `/home/embed/Dev/CADNC/`
- **Niye:** MilCAM'in CAM modülü, adapter, viewport ve resources buradan
  geliyor. CAM koduna olan tüm değişiklikler önce CADNC'de yapılmalı,
  sonra MilCAM'e portlanmalı.
- **Önemli alt dizinler:**
  - `cam/inc/`, `cam/src/` — 8 operasyon tipi, post-processorlar
  - `adapter/inc/CamFacade.h` — UI ile CAM arası köprü
  - `viewport/` — OCCT V3d_Viewer QML entegrasyonu
  - `doc/VEC-VE-*.pdf` — donanım manuelleri (MilCAM doc/'a kopyalanmadı,
    ihtiyaç olunca CADNC'den okunur)
  - `.ai/` — kendi context dosyaları, MilCAM'inkilerle çapraz okunabilir

### FreeCAD upstream
- **Konum:** `/home/embed/Downloads/FreeCAD-main-1-1-git/`
- **Niye:** LGPL kaynak. `freecad/` ağacına bir bugfix geri portlanması
  gerekirse referans noktası.
- **Versiyon:** 1.2 (CADNC ve MilCAM'in baz aldığı)

## İkinci Derece (zaman zaman)

### MilCAD (önceki nesil)
- **Konum:** `/home/embed/Dev/MilCAD/`
- **Niye:** CAM modülü ilk burada gelişti. Bazen eski yaklaşımı görmek
  faydalı, ama **kodu körü körüne kopyalama** — CADNC'deki sürüm daha
  güncel/temiz.

### LibreCAD
- **Konum:** `/home/embed/Downloads/LibreCAD-master/`
- **Niye:** 2D CAD referansı (DXF okuma yaklaşımı, snap stratejileri).
- **Lisans:** GPL — kodu kopyalama, sadece davranış referansı.

### SolveSpace
- **Konum:** `/home/embed/Downloads/solvespace-master/`
- **Niye:** Constraint solver referansı — MilCAM kullanmıyor ama Sketcher
  geri eklenirse alternative motor olarak akılda tut.
- **Lisans:** GPL — kopyalama.

### OCCT örnekleri
- **occQt6:** https://github.com/mschollmeier/occQt6 — Qt6 + OCCT minimal
- **OCCT samples:** OCCT tarball içindeki `samples/qt/` dizini
- **Niye:** Viewport bug avlama, AIS_InteractiveContext örüntüleri.

## Üçüncü Derece (özel durumlar)

### CodeSys runtime dokümantasyonu
- **Online:** https://content.helpme-codesys.com/
- **Yerel kopyası YOK** — gerektikçe webten oku.
- Önemli sayfalar:
  - SysProcess: `SysProcessExecuteCommand2`
  - SMC CNC: `SMC_ReadNCFile2`, `SMC_NCInterpreter`
  - OPC UA Server: setup, namespace, security
  - Target Visualization: `targetvisuextern.cfg` parametreleri

### Qt Virtual Keyboard
- **Doc:** https://doc.qt.io/qt-6/qtvirtualkeyboard-index.html
- **Lisans:** GPL/Commercial — bizim use case (in-house industrial) GPL OK.

### open62541
- **Doc:** https://open62541.org/doc/master/
- **GitHub:** https://github.com/open62541/open62541

## Reverse Lookup: "X için nereye bakarım?"

| Soru                                        | Nereye                                  |
| ------------------------------------------- | --------------------------------------- |
| "Yeni bir operasyon türü nasıl eklenir?"     | `cam/inc/Operation.h` + CADNC'deki adaptörler |
| "STEP import nasıl yapılır?"                 | FreeCAD `Mod/Part/App/ImportStep.cpp` + OCCT STEPCAFControl_Reader docs |
| "OCCT viewport thread safety?"               | OCCT docs + CADNC'deki SceneManager (artık MilCAM'de yok ama referans) |
| "TargetVisu pencereli mod?"                  | CodeSys Forge thread (Web research, .ai/'da link yok — research notes) |
| "wmctrl alternatifleri Wayland?"             | `swaymsg` (Sway), `gdbus` (GNOME)       |

## Hangi Kaynaktan Kod Kopyalanır?

| Kaynak               | Kod kopyalanır mı? | Niçin                              |
| -------------------- | ------------------ | ---------------------------------- |
| CADNC                | EVET                | Aynı sahip, aynı lisans            |
| MilCAD               | EVET (CAM kısmı için) | Aynı sahip                       |
| FreeCAD              | EVET (`freecad/` ağacında zaten kopya var) | LGPL'e uygun korunarak |
| LibreCAD             | HAYIR               | GPL — proprietary'mize bulaştırır |
| SolveSpace           | HAYIR               | GPL                                |
| Luban (nesting)      | HAYIR (zaten scope dışı) | -                              |
| Internet code snippet| Dikkatli — lisans kontrol | Genelde MIT/BSD OK; GPL hayır |
