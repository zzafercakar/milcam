# Cihaz Görüntü Mimarisi — Canlı Bulgu (2026-06-09)

> **KRİTİK DÜZELTME.** Bu belge, daha önceki `.ai/` dokümanlarındaki
> "cihaz X11 + xcb kullanır" varsayımını **canlı kanıtla çürütüyor**.
> SSH (dropbear) üzerinden read-only probe ile toplandı; CodeSYS runtime'a
> hiç dokunulmadı (jitter disiplini korundu).

## Tek Cümle

Cihaz **stok hâlinde X11 KULLANMIYOR** — CodeSYS runtime doğrudan
`/dev/fb0` framebuffer'ına çiziyor (Qt `linuxfb` platformu). X server,
xcb plugin ve OpenGL userspace **yok**. FreeCAD'i çalıştıran vendor
prosedürü cihazı bir reboot ile X11'e *çeviriyor* (`temp_lib/replace.sh`),
ama bu ünitede uygulanmamış ve o dosyalar elimizde yok.

## Canlı Kanıt (root@192.168.1.123, 2026-06-09)

| Soru | Bulgu | Komut |
|------|-------|-------|
| X server var mı? | **YOK** (Xorg/Xvfb/Xwayland binary'si tüm FS'de yok) | `find / -iname Xorg ...` |
| xcb Qt plugin? | **YOK** (`libqxcb.so` tüm FS'de yok) | `find / -iname libqxcb.so` |
| Sistem Qt platformları | `directfb, linuxfb, minimal, offscreen, **vnc**` | `ls /usr/lib/qt/plugins/platforms/` |
| Sistem Qt sürümü | **5.12.4** (Qt5Core/Gui/Widgets var) | `ls /usr/lib/libQt5*.so*` |
| Framebuffer | `/dev/fb0`, 1024×768 | `cat /sys/class/graphics/fb0/virtual_size` |
| fb0 sahibi | **codesyscontrol** (PID 2553, 2866 — maps fb0) | `grep /dev/fb0 /proc/*/maps` |
| OpenGL/EGL/GLES userspace | **YOK** (`libGL/libEGL/libGLESv2` yok) | `find /usr/lib -iname "libGL*"` |
| DRM | `/dev/dri/card0` var (kernel düzeyi), userspace GL yok | `ls /dev/dri` |
| env.sh QPA | `QT_QPA_PLATFORM=linuxfb:fb=/dev/fb0:...` | `cat /etc/env.sh` |
| env.sh tarihi | **2020-05-14** — vendor orijinali, hiç değiştirilmemiş | `ls -l /etc/env.sh` |
| Touch | "Weida Hi-Tech CoolTouch" USB → `event1` | `/proc/bus/input/devices` |
| FreeCAD / temp_lib cihazda | **YOK** (`/root/FreeCAD` yok, `temp_lib`/`replace.sh` yok) | `find /` |
| glibc | 2.29, aarch64 | `libc.so.6 → libc-2.29.so` |
| Dinleyen port | yalnız `11740` (CodeSys gateway) | `netstat -tln` |

## "How to Run FreeCAD" Dokümanının Gerçek Anlamı

`doc/How to Run FreeCAD(2).docx` adımları (özet):

1. NFS ile Ubuntu host'tan (`192.168.1.18`) `/mnt` mount et.
2. `FreeCAD_1.0.2-conda-Linux-aarch64-py311.AppImage` kopyala + `--appimage-extract`.
3. **`temp_lib` klasörünü kopyala + `replace.sh` çalıştır.**  ← KRİTİK
4. **`reboot`.**
5. Env: `DISPLAY=:0`, `QT_QPA_PLATFORM=xcb`, `QT_QPA_GENERIC_PLUGINS=tslib:/dev/input/event1`.
6. `/root/FreeCAD/squashfs-root/AppRun &`.

**Çıkarım:** `replace.sh` cihaza bir **X server + xcb + (muhtemelen) yazılım
GL** yerleştiren ve `env.sh`'i `xcb`'ye çeviren vendor "sihirli kutu"sudur.
FreeCAD AppImage'in kendi paketlediği Qt yalnız **xcb** taşır; çalışması için
bir X server şart. Yani:

- FreeCAD bu donanımda **çalışabiliyor** (vendor kanıtladı).
- Ama **stok linuxfb modunda DEĞİL** — `replace.sh`+reboot ile X11 moduna
  geçtikten sonra.
- `temp_lib`/`replace.sh` **elimizde yok** (ne cihazda ne lokalde). Lokalde
  sadece **x86_64** AppImage var (`/home/embed/Downloads/FreeCAD_1.1.0-Linux-x86_64-py311.AppImage`)
  — cihaz **aarch64**, uyumsuz.

## Bunun Mimariye Etkisi (Düzeltilmesi Gerekenler)

Önceki dokümanlardaki şu varsayımlar **bu ünite için yanlış**:

1. `ARCHITECTURE.md`: "Aynı X server (:0) ikisini de host eder." → Stok hâlde
   X server yok. CodeSYS framebuffer'ı tek başına tutuyor.
2. `CLAUDE.md` kural #6: "Linux: GLX only." → GLX yok, X yok. Stok hâlde
   hiçbir GL yok.
3. **`SwitchToHmi` (wmctrl) komutu çalışamaz** stok hâlde — wmctrl X11/EWMH
   ister; linuxfb framebuffer'da pencere/WM kavramı yok.
4. **TargetVisu "windowed mode" + MilCAM yan yana** modeli, tek-framebuffer +
   WM-yok + X-yok bir sistemde **mümkün değil** — replace.sh ile X11'e
   geçmeden olmaz.

## İleri Giden Yollar (henüz karar verilmedi)

### Yol 1 — VNC platformu (girişimsiz, CodeSYS'e dokunmaz)
Sistem Qt'sinde `vnc` platform plugin'i var. Bir Qt uygulamasını
`QT_QPA_PLATFORM=vnc` ile çalıştır → GUI TCP :5900'den yayınlanır, host'tan
VNC viewer ile bakılır. fb0'a dokunmaz, CodeSYS'i bozmaz. **Sınır:** 3D
viewport için GL yok (OCCT yazılım rasterizer Mesa/llvmpipe gerektirir, o da
yok); cihazda çalıştırılabilir bir Qt binary'si yok (cross-build gerek).

### Yol 2 — Vendor X11 prosedürünü uygula (ağır, girişimsel)
`temp_lib` + aarch64 AppImage edin → `replace.sh` + reboot. Gerçek senaryoyu
test eder ama canlı CNC kontrolcüsünü reboot eder ve lib swap yapar. Elimizde
o dosyalar yok; vendordan istenmeli veya X server + Mesa-llvmpipe + xcb
aarch64 cross-build edilmeli.

### Yol 3 — Cihaz-dışı CAM (Plan C)
MilCAM/FreeCAD ayrı bir workstation'da çalışır; cihaz yalnız G-code alıcısı
(drop folder + OPC UA). Bu yapı bu donanım gerçeğiyle en uyumlu olan.

## TEST SONUÇLARI — Vendor X stack + FreeCAD canlı denendi (2026-06-09)

Vendor X stack'i **merge** ile kuruldu (`rm -rf /usr` YERİNE `tar`-pipe ile
üzerine yazıldı → **dropbear/SSH korundu**; vendor `/usr` zaten superset
olduğu için CodeSYS bağımlılıkları da korundu). `/usr/sbin/dropbear` statik
olduğu için `/usr` değişiminden etkilenmez.

### ✅ X11 arayüz testi GEÇTİ (fiziksel panelde doğrulandı)
- `S40xorg` → `Xorg :0.0 vt01` başladı; log: `modeset(0): Output DSI-1
  connected`, modeline **1024×768**, sürücü **modesetting** (`/dev/dri/card0`).
- `xterm` DISPLAY=:0'da açıldı ve **fiziksel panelde GÖRÜNDÜ** (kullanıcı
  fotoğrafı ile doğrulandı — boot penguenleri gitti, yeşil-üzeri-siyah xterm
  geldi). `libqxcb.so` artık mevcut.
- Not: `/dev/fb0` yakalaması X çıktısını GÖSTERMEZ — modesetting DRM/KMS
  kullanır, legacy fbdev'i baypas eder. Görsel doğrulama yalnız panel
  fotoğrafı / DRM-aware araçla mümkün.

### ❌ FreeCAD 3B viewport ÇALIŞMAZ (kesin engel)
FreeCAD 1.0.2 aarch64 AppImage (716MB → extract 2.9GB) X'te çalıştırıldı:
- Süreç açıldı: **~287 MB RSS**, çekirdek ~30s'de yüklendi (idle).
- **FATAL:** `QXcbIntegration: Cannot create platform OpenGL context,
  neither GLX nor EGL are enabled` (log'da tekrar tekrar).
- Doğrulama: **Xorg'da `libglx.so` extension modülü YOK** → X server GLX
  sunmuyor. Sistemde `libGL`/`swrast_dri` yok. EGL yok. FreeCAD kendi
  `libGL.so.1`'ini taşıyor ama GLX/EGL köprüsü olmadan kullanamıyor.
- `tslib:/dev/input/event1` plugin'i FreeCAD'in paketli Qt'sinde **yok**
  (dokunmatik girdi de doğrudan çalışmaz).

**Sonuç:** X stack tam kurulsa bile FreeCAD'in OCCT/Coin3D 3B görünümü bu
donanımda **render edilemez** (GL yok). FreeCAD CAM iş akışı 3B görünüme
bağlı olduğundan, FreeCAD bu cihazda **CAM için pratikte kullanılamaz**.

### Ağırlık tablosu (FreeCAD, bu cihazda)
| Ölçüt | Değer |
|-------|-------|
| AppImage | 716 MB (transfer 2dk15s @ ağ) |
| Extract | 2.9 GB disk |
| Gereken X swap | 156 MB vendor `/usr` merge |
| RAM (idle, GUI) | ~287 MB RSS |
| Çekirdek açılış | ~30 s (Cortex-A53) |
| **3B viewport** | **ÇALIŞMAZ — GL yok** |
| Dokunmatik | paketli Qt'de tslib yok |

### Çıkarım → Standalone Qt 2.5B CAM lehine güçlü kanıt
Bir **QtWidgets** 2B uygulaması (QGraphicsView) **OpenGL GEREKTİRMEZ** —
Qt'nin raster paint engine'i ile çizer. Yani FreeCAD'in 3B'sinin çökdüğü bu
donanımda, stok Qt 5.12.4 + linuxfb/xcb üzerinde **2.5B CAM sorunsuz çalışır**.
Cihaz stok hâlinde Widgets/Quick/Svg/VirtualKeyboard + Python 3.7 taşıyor.

### KARAR + son durum (2026-06-10)
- **Mimari karar:** MilCAM = **standalone Qt 2.5B CAM** (Qt Widgets/linuxfb,
  GL'siz). FreeCAD'in GL bağımlı 3B'si bu donanımda çalışmadığı için terk
  edildi. CODESYS post'u DIN 66025 dialect'ine göre yeniden yazılacak
  (bkz. araştırma raporu; mevcut `codesys_post.py` standart ISO üretiyor =
  yanlış).
- **Xorg DEVRE DIŞI** bırakıldı (`/etc/init.d/S40xorg` → `_S40xorg`).
  Standalone CAM linuxfb kullanacağı için X gereksiz. Cihaz reboot edildi
  (kullanıcı tarafından) → her şey boot-safe geri geldi, **IP değişmedi**
  (eth1 192.168.1.123, gateway 11740 dinliyor).
- **Boot splash kuruldu:** `/etc/init.d/S95splash` açılışta
  `/etc/milcam-splash.raw`'ı (1024×768 BGRX, ortalanmış SMB logosu)
  `/dev/fb0`'a basıyor → penguenler yerine SMB logosu; bir uygulama fb0'a
  çizene dek kalır. Üretici: `scripts/make_splash.py` (gerçek logo PNG ile
  değiştirilebilir). Logo: **SMB Technics** yuvarlak logosu (beyaz zemin),
  `scripts/smb_logo.png`. Panelde fb0 capture ile doğrulandı.

### Temizlik / geri-dönüş notları
- `/usr` yedeği: `/root/usr-stock-backup.tgz` (cihazda).
- dropbear yedeği: `/root/dropbear`, `/root/dropbearkey` (cihazda).
- FreeCAD: `/root/FreeCAD/` (AppImage + squashfs-root) — silinebilir, ~3.6 GB.
- Vendor X stack `/usr`'a merge edildi (kalıcı; geri almak için stok yedek).

## Açık Sorular (vendordan / sahadan)

- `temp_lib/replace.sh` içinde tam olarak ne var? (X server türü? Mesa? xcb?)
- replace.sh sonrası CodeSYS TargetVisu hâlâ açılıyor mu, yoksa X11'e mi
  taşınıyor? (Yani FreeCAD ile gerçekten yan yana mı, yoksa biri diğerinin
  yerini mi alıyor?)
- Bu üniteye replace.sh hiç uygulandı mı, yoksa fabrika sıfırlaması mı yapıldı?
