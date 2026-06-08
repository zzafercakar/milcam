# MilCAM Workplan (revised 2026-06-08)

## Faz 0 — Yanlis Iskelet (IPTAL, silindi)

Ilk denemede CADNC tarzi kendi QML UI yazildi. Yanlis. Kullanici aciklamasi
sonrasi tamamen silindi. Detay: ENGINEERING_LOG.md "2026-06-08 - Pivot".

---

## Faz 0.5 — Overlay Iskelet  ✅  (2026-06-08)

**Tamamlanan:**

- `milcam/CodesysBridge/` — C++ core (drop folder + wmctrl) + CPython binding
- `milcam/Mod/MilCAM/` — Init.py, InitGui.py, 3 komut, codesys_post.py, 3 SVG ikon
- `CMakeLists.txt` — FreeCAD'i wrap eder, ~20 modul BUILD_<X>=OFF ile devre disi
- `CMakePresets.json` — default, debug, overlay-only, panel-pc preset'leri
- `tests/python/` — postprocessor + bridge icin pytest smoke
- Tum `.ai/` belgeleri yeni mimariye gore yeniden yazildi
- `CLAUDE.md`, `README.md`, `LICENSE`, `.gitignore` yenilendi

---

## Faz 1 — Ilk End-to-End Build  🔜 (sıradaki)

**Amac:** Slim FreeCAD'in derlenebildigini ve calistigini dogrulamak.

**Adimlar:**

1. FreeCAD build prerekleri host'a kurulu mu kontrol et:
   ```bash
   apt list --installed 2>/dev/null | grep -E "qt5|coin3|opencascade|python3-dev|boost|xerces"
   ```
2. `cmake --preset default` cagir. CMake basari ile yapilandirmazsa
   FreeCAD'in eksik dep mesajlarini takip et.
3. `cmake --build build -j$(nproc)` calistir. Disable edilen modullerin
   gercekten build'e dahil edilmediginden emin ol:
   ```bash
   grep -r "BUILD_FEM" build/CMakeCache.txt    # OFF olmali
   grep -r "BUILD_CAM" build/CMakeCache.txt    # ON olmali
   ```
4. `DESTDIR=$PWD/_inst cmake --install build` — staging install.
5. `_inst/usr/local/bin/FreeCAD --help` cagir, version cikiyorsa OK.

**Beklenen pürüzler:**
- `BUILD_HELP=OFF` ile FreeCAD CMake'i dogru sekilde Help modulunu skip ediyor mu — kontrol.
- Cross-cutting module dependency'leri olabilir (orn. Inspection Part'a baglidir, BIM Draft'a).
  Eger build hatasi varsa, bagimlilik gerektiren X'i tekrar `ON` yap.
- `add_subdirectory()` ile sub-project olarak cagirildiginda FreeCAD bazen
  `CMAKE_CURRENT_SOURCE_DIR` varsayimlari ile karisikilik cikarir.
  Workaround: build dir'i `/tmp/freecad-build` gibi mutlak path'e tasi.

**Kabul kriteri:**
- Build sifir hata ile tamamlanir.
- FreeCAD baslar, version yazar.
- Workbench listesinde MilCAM gozukur, FEM/BIM/TechDraw GOZUKMEZ.

---

## Faz 2 — Workbench Gorunurluk + Send-to-CodeSys Smoke

**Amac:** MilCAM workbench gerçekten CAM is akisini sergileyebilir.

**Adimlar:**

1. FreeCAD'i ac, MilCAM workbench se. Boş bir Job olustur.
2. Bir kare cubuk import et (STEP veya basit Part::Box).
3. Profile operasyonu ekle, post-process et — `Send to CodeSys` butonuna bas.
4. `/tmp/milcam/send_to_codesys.gcode` olustu mu kontrol.
5. `~/var/cnc/jobs/<job_id>.gcode` dropuyor mu kontrol (testte yoksa
   `CodesysBridge.drop_folder = "/tmp/cnc-test"` yap).
6. `wmctrl -a TargetVisu` test et (TargetVisu yoksa "no window found" beklenen).

**Kabul kriteri:** Operatorun manuel adimi olmadan G-code dosyasi dogru
yerlere dususuyor; FreeCAD Console'da "MilCAM: dropped N bytes" mesaji
gorulur.

---

## Faz 3 — Branding (App Name, Splash, Default WB)

**Amac:** Operator "FreeCAD" yerine "MilCAM" gorsun.

**Adimlar:**

1. FreeCAD'in app name override mekanizmasini kullan:
   - Linux `.desktop` dosyasi: Name=MilCAM (deploy.sh ile yerleştir)
   - Window title: Mod/MilCAM/InitGui.py'da `QApplication.setApplicationName("MilCAM")`
   - Splash: FreeCAD'in CMake'i `FREECAD_SPLASH_PIC` parametresi kabul ediyor mu? Kontrol.
2. Default workbench preferences:
   `User parameter:BaseApp/Preferences/General/AutoloadModule = "MilCamWorkbench"`
3. Icon override: kendi `MilCAM.svg`'mizi `.desktop`'ta kullan.

**Kabul kriteri:** Boot sonrasi pencere basligi "MilCAM 0.2.0", uygulamayi
acan task launcher'da ikon MilCAM.

---

## Faz 4 — OPC UA Gerçek Baglanti

**Amac:** PLC ile sembolik veri akisi.

**Adimlar:**

1. `MILCAM_ENABLE_OPCUA=ON` ile open62541 cek + link.
2. `CodesysBridge::connect()` icine session açma, subscription kurma:
   - Subscribe: `PLC.MachineState`, `PLC.CurrentLine`, `PLC.EStop`
   - Write: `MilCAM.JobReadyPath` (string), `MilCAM.RunRequest` (bool)
3. Heartbeat thread — saniyede 1 `MilCAM.AppHeartbeat++`.
4. Python tarafinda durum subscription notification'larini
   `FreeCAD.Console.PrintMessage` ile gosteren basit dinleyici.

**PLC tarafi:** ayri bir CodeSys projesinde GVL_MilCAM open. Sembol
sozlesmesi `.ai/CODESYS_BRIDGE.md`'de.

**Kabul kriteri:** Mock open62541 server'a karsi MilCAM job submit eder,
mock server JobReadyPath sembolunu okur, MachineState=Running set eder,
MilCAM bunu console'da yansitir.

---

## Faz 5 — VEC-VE Deploy + Saha

**Amac:** Cihazda calisir.

**Adimlar:**

1. **Donanim kontrolu (kritik):**
   ```bash
   ssh root@192.168.1.123 'uname -a; free -h; df -h; glxinfo | grep "OpenGL version"'
   ```
   RAM/GPU yetersizse, **dur ve mimari plan yeniden**.
2. `cmake --preset panel-pc` — cross-compile.
3. `./scripts/deploy.sh root@192.168.1.123` — rsync + systemd unit.
4. Cihazda `targetvisuextern.cfg` icine `WindowType=0` ekle.
5. CodeSys ST: "Open CAM" buton → `SysProcessExecuteCommand('wmctrl -a MilCAM')`.
6. 24 saat dayaniklilik testi, cyclictest jitter olcumu.

**Kabul kriteri:**
- MilCAM cihazda acilir, dokunmatikle calisir.
- HMI ↔ MilCAM gecisleri sorunsuz.
- PLC jitter MilCAM aktifken < 200 us.

---

## Riskler Ozeti

1. **FreeCAD add_subdirectory ile MilCAM altinda dogru build edilmeyebilir.**
   FreeCAD CMake'i kendi top-level oldugunu varsayar. Workaround: fork edip
   patch atmak yerine eski yontemle dis FreeCAD build'i + sadece overlay
   install (overlay-only preset).
2. **BUILD_<X>=OFF flag'leri inter-module dependency yuzunden kirilabilir.**
   Ornegin BIM=OFF dersek ama BIM bir baska modulu cagiriyorsa link hatasi
   olur. Faz 1'de gozlemlenecek.
3. **FreeCAD CAM workbench API'si versiyonlar arasi degisken.** 1.0/1.1/1.2
   farkli command isimleri (`Path_Profile` vs `CAM_Profile`). Mod/MilCAM
   bunlari adaptive olarak handle etmeli.
4. **VEC-VE donanim hala olculmedi.** Faz 5 SSH check kritik.
5. **CodesysBridge.so ABI uyumu** — FreeCAD'in Python versiyonu ile build
   ettigimiz extension'un Python versiyonu **ayni** olmali. Cross-build'de
   sysroot'a dikkat.

---

## Cross-cutting

- Postprocessor MilCAM'in CodeSys-ozgu cikis surumunu uretir. Yine de
  FreeCAD'in upstream postprocessor library'sinin formuna sadik kalir
  (PARAMS, TOOLTIP, ARGUMENTS, export(...) imzasi) ki upstream evrimine
  ayak uydursun.
- `Mod/MilCAM/` icindeki Python kod **FreeCAD ABI'sine bagli**. FreeCAD
  major versiyon atlamalarinda komut isim degisiklikleri olur. Surekli
  upstream takibi gerekir.
