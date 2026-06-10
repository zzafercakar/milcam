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

## SIRADAKI (P0.3 — Qt app gelince)
Statik yol Qt için geçmez (Qt dinamik + plugin'ler). O zaman:
- VEC-VE sysroot kur: cihaz `/usr/lib` + `/lib` rsync'le (glibc 2.29 + Qt 5.12.4 .so'lar)
  → `/opt/sysroot/vec-ve`; Qt 5.12.4 header'larını kaynak tarball'dan ekle.
- Toolchain dosyasına `CMAKE_SYSROOT` + Qt `CMAKE_PREFIX_PATH` ekle.
- linuxfb QPA + evdevtouch ile "hello MilCAM" penceresi, fb0 capture ile doğrula.
