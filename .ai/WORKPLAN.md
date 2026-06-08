# MilCAM Workplan

Bu dosya MilCAM'in faz faz yol haritasini tutar. Her faz: amac, kabul kriteri,
somut adimlar, riskler.

## Faz 0 — Iskelet  ✅  (2026-06-08)

**Amac:** Calisılabilir bir proje yapisi.

**Tamamlanan:**
- Dizin agaci olusturuldu.
- CADNC'den `cam/`, `adapter/`, `util/`, `viewport/`, `resources/` kopyalandi.
- FreeCAD slim subset (`Base`, `App`, `Mod/Material/App`, `Mod/Part/App`)
  + 3rdParty + cmake yerlestirildi. Sketcher ve PartDesign **dahil edilmedi**.
- Yeni top-level `CMakeLists.txt` yazildi — adapter glob Sketch* ve Nest*'i
  build'e almıyor.
- `app/main.cpp` (Virtual Keyboard + single-instance + crash handler).
- `ui/qml/Main.qml` touch-first shell.
- `adapter/inc/ImportFacade.h` + `adapter/src/ImportFacade.cpp` (stub).
- `adapter/inc/CodesysBridge.h` + `adapter/src/CodesysBridge.cpp` (drop folder
  ve `wmctrl` raise hazir; OPC UA stub).
- `tests/test_codesys_bridge.cpp` smoke test.
- CLAUDE.md, README.md, LICENSE, .gitignore, .ai/ dokuman seti.
- .vscode/ workspace + launch + tasks.

## Faz 0.5 — VEC-VE Hardware Reality Check  🔜  (TEK SOMUT EKSIK ADIM)

**Amac:** Cihazin gercek kapasitesini bilmek. Aksi halde butun plan spekulasyon.

**Adimlar:**
1. SSH ile panel PC'ye bagla (varsayim: `root@192.168.1.123`).
2. Su komutlari calistir ve sonuclari `.ai/ENGINEERING_LOG.md`'ye yapistir:
   ```bash
   uname -a
   cat /etc/os-release 2>/dev/null
   cat /proc/cpuinfo | grep -E "model name|Hardware|Features" | sort -u
   free -h
   df -h /
   df -h /root
   ls -la /opt/codesys/ /etc/codesys* 2>/dev/null
   cat /etc/codesys*/CODESYSControl.cfg 2>/dev/null | head -100
   ps auxf | head -40
   glxinfo 2>/dev/null | grep -E "OpenGL (version|renderer)"
   which wmctrl xdotool onboard
   ls /dev/input/by-path/
   ```
3. **GO / NO-GO karar agaci:**
   - RAM >= 2 GB ✅ Plan A devam.
   - RAM 1-2 GB ⚠ Slim CADNC viewport, footprint dietleme zorunlu.
   - RAM < 1 GB ⛔ Plan C: MilCAM ayri workstation'da, panel PC yalniz drop-folder
     receiver.
   - OpenGL >= 2.0 ✅
   - OpenGL yok / yetersiz ⛔ Software rasterizer (cok yavas) veya 2D-only mod.
   - Bos disk >= 500 MB ✅

**Kabul kriteri:** Hardware durumu ENGINEERING_LOG.md'de belgelenmis. Plan A/B/C
sectim, donanim diaspora dietleme listesi netlesmis.

---

## Faz 1 — Adapter Slimming

**Amac:** CAM-only adapter — gereksiz kodu temizle.

**Adimlar:**
1. Sil: `adapter/inc/SketchFacade.h`, `adapter/inc/SketchDocument.h`,
   `adapter/inc/SketchEntity.h`, `adapter/inc/NestFacade.h`,
   `adapter/inc/SceneManager.h`.
2. Sil: `adapter/src/SketchFacade.cpp`, `adapter/src/NestFacade.cpp`.
3. `CadEngine.h/.cpp` icindeki `SketchFacade*` ve `NestFacade*` referanslarini
   cikar. Rename: `CadEngine` → `MilCamEngine`. (Veya kalsin, sade isim
   degisikligi.)
4. `CadDocument` icindeki sketch ile ilgili API'leri kaldir (eger varsa).
5. `PartFacade` → import-modu disinda her sey kaldirilir; isim `ImportFacade`'e
   gecis. Mevcut `ImportFacade` su an stub — `PartFacade`'in import koduna ihtiyac
   var.
6. CMake glob filtresini kaldir; artik fiziksel olarak yok zaten.

**Kabul kriteri:** Adapter clean compile, `nm libmilcam_adapter.a` icinde
Sketch/Nest yok.

---

## Faz 2 — Geometry Intake

**Amac:** STEP/IGES/BREP/DXF/STL gerçek olarak içeri alinmasi.

**Adimlar:**
1. `ImportFacade::importFile` icinde uzantiya gore:
   - `.step|.stp`  → `Part::ImportStep` (FreeCAD API)
   - `.iges|.igs`  → `Part::ImportIges`
   - `.brep`       → `BRepTools::Read`
   - `.stl`        → `StlAPI_Reader`
   - `.obj`        → OCCT OBJ reader
   - `.dxf`        → libArea (FreeCAD's importDXF) — read-only
2. Yuklenen TopoShape'i `CamGeometrySource` ile CAM modulune baglar.
3. Viewport'a duzgun render et: bbox'a fit, varsa renkleri aktar.
4. UI: import progress, hatali dosyada anlasilir mesaj.

**Kabul kriteri:** `tests/test_import_step.cpp` — 5 ornek STEP, 3 DXF, 2 STL
hatasiz acilir; bbox dogrulanir.

---

## Faz 3 — OPC UA + Drop Folder

**Amac:** PLC ile gerçek IPC.

**Adimlar:**
1. `MILCAM_ENABLE_OPCUA=ON` ile build, open62541 cekiliyor.
2. `CodesysBridge::connect` — secure channel (opt: encrypt yok ilk surumde),
   namespace browse, sembolleri register et.
3. Sembol haritasi (CodeSys tarafinda kurulacak — ayri dokuman):
   - `MilCAM.JobReadyPath`  (string, MilCAM → PLC)
   - `MilCAM.RunRequest`     (bool, MilCAM → PLC)
   - `PLC.MachineState`      (int, PLC → MilCAM)
   - `PLC.CurrentLine`       (int, PLC → MilCAM)
   - `PLC.EStop`             (bool, PLC → MilCAM)
4. `CodesysBridge::notifyJobReady(path)` — sembolu set et + RunRequest event.
5. Subscribe to MachineState/CurrentLine/EStop → Qt signal'lara map.
6. UI'da PLC durum LED'i (yesil/sari/kirmizi).

**Kabul kriteri:** Bir test PLC programi ile end-to-end calisma: MilCAM job
drop ediyor, PLC dosyayi okuyor, MilCAM CurrentLine guncellemesini goruyor.

---

## Faz 4 — VEC-VE Deploy + Coexistence

**Amac:** Iki uygulama (CodeSys TargetVisu + MilCAM) ayni cihazda calisiyor.

**Adimlar:**
1. Cross-compile presetini (`panel-pc`) gerçek bir aarch64 sysroot ile besle.
   `scripts/toolchain-aarch64.cmake` yaz.
2. `scripts/deploy.sh` — rsync ile binary + qml kaynagi + qt runtime panel PC'ye.
3. Cihazda `targetvisuextern.cfg` icine `WindowType=0` ekle. TargetVisu pencereli
   acilsin mi kontrol et.
4. Yoksa Plan B: TargetVisu fullscreen kalsin, MilCAM ayri X workspace'inde,
   `wmctrl -s` ile switch.
5. systemd unit yaz: `milcam.service`, `milcam-launcher.service` (uste sabit
   switcher bar). Boot sirasi: codesys → launcher → milcam.
6. CodeSys ST kodunda "Open CAM" butonu → `SysProcessExecuteCommand('wmctrl -a MilCAM ...')`.
7. `/etc/codesys*/CODESYSControl.cfg`'ye `[SysProcess] Command=AllowAll` ekle.

**Kabul kriteri:**
- Cihaz acilirken: TargetVisu pencereli, MilCAM gizli/arkada, ust serit gorunur.
- "CAM" butonu basinca MilCAM one geliyor.
- "HMI" butonu basinca TargetVisu one geliyor.
- `cyclictest -p 80 -t1 -n -i 1000 -l 600000` MilCAM aktifken max latency
  < 200 us (Faz 0.5'te belirlenmis limit).

---

## Faz 5 — Saha Kabul Testi + Hardening

**Amac:** Production-ready.

**Adimlar:**
1. 24 saat dayaniklilik — HMI↔CAM gecisi 1000+ defa.
2. Power-cut testi — beklenmedik kapatma sonrasi disk butunlugu.
3. `/var/cnc/jobs/` dolma senaryosu — graceful warning, eski jobs temizleme.
4. EStop senaryosu — PLC EStop'lardiginda MilCAM red banner.
5. Turkce/English UI dil destegi.
6. Operator kullanim kilavuzu — `doc/USER_GUIDE_TR.md`.

**Kabul kriteri:** Saha pilotu (1 makine, 2 hafta) sorunsuz.

---

## Cross-cutting concerns

- **Real-time jitter discipline** her fazda dikkate alinmali. Yeni bir IO
  islemine baslamadan once `.ai/VEC_VE_TARGET.md`'deki kurali oku.
- **CAM modul senkronizasyonu** — burada cam/'da degisiklik yaparsan, CADNC
  cam/'in da hizalanmasi gerek.
- **freecad/ minimum modification** — upstream uyumu kritik.

---

## Riskler ozeti

1. **VEC-VE donanim yetersiz** — Faz 0.5 olcumune kadar bilinmiyor.
2. **TargetVisu vendor build'i windowed mode izin vermeyebilir** — fallback
   WebVisu (QWebEngineView).
3. **PartFacade'in import API'si MilCAM scope'una uymayabilir** — Faz 2'de
   secim: ya genisle ya tamamen yeni `ImportFacade` yaz.
4. **CADNC cam/ ile divergence riski** — disiplinli backport zorunlu.
5. **OPC UA sembol sözlesmesi PLC ekibiyle yazili olmali** — sembol isim
   degisirse iki tarafta da kirilir.
