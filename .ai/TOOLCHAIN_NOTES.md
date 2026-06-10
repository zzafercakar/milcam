# Toolchain Notes — aarch64 cross-build for VEC-VE

> Plan görev 0.1/0.2 çıktısı. Standalone Qt 2.5B CAM kararı sonrası
> (bkz. [DISPLAY_ARCHITECTURE_2026-06-09.md](DISPLAY_ARCHITECTURE_2026-06-09.md))
> cihaza C++ core'u götürmek için cross-build durumu.

## Hedef cihaz (DOĞRULANDI 2026-06-10 canlı)
- `uname -m` → **aarch64**
- libc → **GNU C Library (Buildroot) stable release version 2.29** (`/lib/libc.so.6`)
- 4× Cortex-A53 (i.MX8M), PREEMPT-RT kernel 4.14.98.

## Host
- Ubuntu 24.04, `aarch64-linux-gnu-gcc/g++ 13.3.0` kurulu
  (`crossbuild-essential-arm64`). Host glibc **2.39**.
- **Risk:** host glibc 2.39 ↔ cihaz glibc 2.29 → dinamik linklenen binary
  cihazda sembol bulamaz (dropbear dersi, bkz. ENGINEERING_LOG 2026-06-09 gece).

## P2 kararı: STATİK linkli core (en düşük risk)
MilCAM C++ core'un **hiçbir üçüncü-parti bağımlılığı yok** (sadece libstdc++/
libc/libm). Bu yüzden **tam statik** (`-static -static-libgcc -static-libstdc++`)
aarch64 binary, cihazın glibc sürümünden **bağımsız** çalışır → sysroot'a gerek
kalmadan glibc uyumsuzluğu tamamen baypas edilir.

- Toolchain dosyası: [`cmake/toolchain-vecve-aarch64.cmake`](../cmake/toolchain-vecve-aarch64.cmake)
  - `CMAKE_SYSTEM_PROCESSOR aarch64`, `aarch64-linux-gnu-{gcc,g++}`
  - `-march=armv8-a -mtune=cortex-a53` (i.MX8M)
  - `CMAKE_EXE_LINKER_FLAGS_INIT "-static -static-libgcc -static-libstdc++"`
- Build script: [`scripts/build-arm64.sh`](../scripts/build-arm64.sh) → `build-arm64/tests/core_tests`.
- SMB-Q6R referansı (`/home/embed/Dev/QT6/TeachPendant/SMB-Q6R/cmake/aarch64-linux-gnu.cmake`)
  model alındı; o **RK3568** (farklı cihaz) hedefliyor ama aynı Cortex-A53 ABI.
  SMB-Q6R multiarch (`qtbase5-dev:arm64`) + DİNAMİK link kullanıyor; bizim P2
  core'umuz onun aksine **statik** (Qt yok). SMB-Q6R'ın deploy.sh + plc_link
  (open62541, Qt5.12) ilerideki P0.3/P5 için saklı.

## DOĞRULANDI: cihazda çalışıyor (2026-06-10)
```
build-arm64/tests/core_tests: ELF 64-bit LSB executable, ARM aarch64,
  statically linked, for GNU/Linux 3.7.0
# cihazda:
ALL PASS (5 cases)   exit=0
```
- Toolchain + glibc riski (projenin en büyük riski) **statik-core yolu için
  çürütüldü**: toolchain doğru aarch64 binary üretiyor, post mantığı gerçek
  donanımda host ile birebir aynı sonucu veriyor.

## Transfer yöntemi (cihazda sftp YOK)
Cihaz dropbear'ında `/usr/libexec/sftp-server` yok → `scp` (SFTP modu)
**çalışmaz**. Çözüm: ssh stdin üzerinden `cat` ile akıt:
```bash
SSH="ssh -i ~/.ssh/milcam_id root@192.168.1.123"
$SSH 'mkdir -p /root/milcam-test/golden'
$SSH 'cat > /root/milcam-test/core_tests' < build-arm64/tests/core_tests
$SSH 'cat > /root/milcam-test/golden/profile_square.cnc' < tests/core/golden/profile_square.cnc
$SSH 'chmod +x /root/milcam-test/core_tests && /root/milcam-test/core_tests'
```
(Alternatif: base64 — ama ham `cat` redirect ikili dosya için yeterli ve hızlı.)

## Golden dosya yolu relocate edilebilir
`core_tests` golden `.cnc`'leri derleme-zamanı `MILCAM_GOLDEN_DIR` ile bulur
(host'ta in-tree). Cihaz için configure'da override:
`-DMILCAM_GOLDEN_DIR=/root/milcam-test/golden`. Golden dosyayı o yola da kopyala.

## P0.3 — Qt Widgets app (DOĞRULANDI 2026-06-10, ekranda çalışıyor)

Statik yol Qt için geçmez (Qt dinamik). Çözülen reçete (script:
[`scripts/build-app-arm64.sh`](../scripts/build-app-arm64.sh)):

1. **glibc≤2.29 toolchain ŞART.** Host `aarch64-linux-gnu-g++` (gcc 13 / glibc
   2.39) ile linklenen Qt app cihazda `version 'GLIBC_2.34' not found` (2.32/
   2.34/2.35/2.36/2.38) verip ÇALIŞMAZ — statik core'da olmayan yeni problem,
   çünkü orada her şey statikti, burada glibc dinamik. **Çözüm:** Bootlin 2018.11
   aarch64 toolchain (**gcc 7.3 / glibc 2.27**). 2.27'ye linklenen binary 2.29'da
   çalışır (eski sembol sürümleri ileri-uyumlu). İndirme:
   `toolchains.bootlin.com/.../aarch64/tarballs/aarch64--glibc--stable-2018.11-1.tar.bz2`
   → `$HOME/milcam-sysroot/aarch64--glibc--stable-2018.11-1/`.
2. **Header:** desktop Qt 5.12.4 (aqt) `include/` — Qt header'ları arch-bağımsız
   (LP64). aqt: `$HOME/milcam-sysroot/aqtenv/bin/aqt install-qt linux desktop
   5.12.4 gcc_64 --archives qtbase --outputdir .../qt-desktop`. (qconfig.h vb.
   üretilmiş config header'ları kaynak tarball'da YOK; aqt install'da VAR.)
3. **Link:** cihazın gerçek `.so.5`'lerine `-l:libQt5Widgets.so.5` (versiyonlu
   soname; cihazda dev symlink yok). Sadece `-L$SYSROOT/usr/lib` (+ rpath-link);
   `$SYSROOT/lib`'i (cihaz libc/pthread/dl — GLIBC_PRIVATE) EKLEME → linker
   bunları kendi 2.27 toolchain'inden alır. `--allow-shlib-undefined` ile Qt
   `.so`'larının çözülmemiş sembollerine tolerans.
4. **`-static-libstdc++ -static-libgcc`:** cihazın libstdc++.so.6.0.25 (GCC8) ABI
   uyumsuzluğunu atla; C++ runtime'ı binary'ye göm. Sonuç NEEDED: yalnız
   libQt5{Widgets,Gui,Core}.so.5 + libm/libpthread/libc.so.6 (hepsi cihazda).
5. **moc:** desktop Qt'nin x86_64 moc'u (host aracı) — `MainWindow.h` → moc cpp.
6. **Çalıştırma (cihaz, Xorg KAPALI, linuxfb):**
   ```
   QT_QPA_PLATFORM_PLUGIN_PATH=/usr/lib/qt/plugins QT_QPA_FONTDIR=/usr/lib/fonts \
   QT_QPA_PLATFORM=linuxfb:fb=/dev/fb0 LD_LIBRARY_PATH=/usr/lib:/lib ./milcam
   ```
   evdevtouch Weida dokunmatiği `/dev/input/event1`'de otomatik buldu.
7. **Doğrulama:** `dd if=/dev/fb0 bs=4096 count=768` → host'ta PIL
   `Image.frombytes('RGB',(1024,768),data,'raw','BGRX')` → PNG. Sonuç: MilCAM
   penceresi tam ekran, koyu tema + toolbar, SMB logosu yerini aldı. ✅

**Sonuç:** GL'siz Qt Widgets app bu donanımda **çalışıyor** — standalone CAM
kararı uçtan uca doğrulandı (post + GUI ikisi de cihazda).

## Büyük SDK'lar nerede (repoya KOYMA)
`$HOME/milcam-sysroot/` (gitignore dışı, repoda değil): `vec-ve/` (cihaz lib'leri,
~130MB), `qt-desktop/` (Qt 5.12.4 header), `aarch64--glibc--stable-2018.11-1/`
(Bootlin toolchain), `aqtenv/` (venv). Taze checkout'ta bu README'deki komutlarla
yeniden üretilir.

## CMake'e geçiş (sonra)
`build-app-arm64.sh` doğrudan-g++ bir ilk-ışık spike'ı. CMake `milcam` target'ı
(`milcam/app/CMakeLists.txt`, `MILCAM_BUILD_APP=ON`) zaten var ama find_package(Qt5)
cihaz sysroot'unda Qt CMake config'i olmadığı için cross'ta kullanılamıyor.
İleride: Bootlin sysroot'una Qt5 CMake config + header yerleştir, toolchain
dosyasına `CMAKE_SYSROOT` + `CMAKE_PREFIX_PATH` ekle → tek CMake build.
