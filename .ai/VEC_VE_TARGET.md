# VEC-VE Target Platform

MilCAM'in deployment hedefi. Bu dosyayi oku — saha cihazinin gerçekleri
mimari kararlarin kokunde.

> **2026-06-09 update:** Cihaza ilk fiziksel erisim saglandi (RS232 serial
> console). Detayli erisim rehberi: [SERIAL_CONSOLE_ACCESS.md](SERIAL_CONSOLE_ACCESS.md).
> Cihazdan canli alinan veriler: [DEVICE_PROFILE.md](DEVICE_PROFILE.md).
> Network probe: [NETWORK_PROBE_2026-06-09.md](NETWORK_PROBE_2026-06-09.md).

## Erisim Yollari (2026-06-09 itibariyle)

| Yontem            | Durum          | Detay                                       |
|-------------------|----------------|---------------------------------------------|
| RS232 serial console | ✅ CALISIYOR | 115200 8N1 no flow control; pin 4=RX, 8=TX, 6=GND |
| Network ping       | ✅ 3 ms RTT   | 192.168.1.123 (default vendor IP)            |
| SSH (port 22)      | ❌ kapali     | dropbear yuklu mu? — Faz 0.7'de kontrol      |
| Telnet (port 23)   | ❌ kapali     | telnetd yok                                  |
| CodeSys gateway (11740) | ✅ acik   | CODESYS Online uzerinden runtime erisim       |
| WebVisu (8080)     | ❌ kapali     | uygulamada Visualization eklenmemis           |
| OPC UA (4840)      | ❌ kapali     | MilCAM Faz 3'te acilacak                     |
| FreeCAD AppImage   | ✅ kanitli   | 1.0.2 aarch64 daha once calistirilmis (.docx) |

## Donanim Profili (Belgelenen)

Kaynak: `doc/VEC-VE-MU-AH 12 inch.pdf` + `doc/How to Run FreeCAD(2).docx`.

| Ozellik       | Deger                                      |
| ------------- | ------------------------------------------ |
| Model         | VEC-VE-MU-AH 12 inch                       |
| CPU           | Quad-core ~1.6 GHz Cortex-A sınıfı (ARM)   |
| **Arch**      | **aarch64 (ARM 64-bit)** — DOGRULANDI 2026-06-09 (FreeCAD AppImage adi `...aarch64...`) |
| RAM           | **DATASHEET'TE BELIRTILMEMIS** — Faz 0.7'de ölçülecek (cihazda `free -h`) |
| Disk          | microSD (TICard) + dahili flash            |
| Ekran         | 12" 1024×768 TFT, PLED backlight 50K saat  |
| Touch         | 4-tel dirençli, ±%2 doğruluk               |
| OS            | Embedded Linux (busybox userland)          |
| Display srv   | X11, `DISPLAY=:0`, `QT_QPA_PLATFORM=xcb`   |
| Touch device  | `/dev/input/event1` (tslib)                |
| CodeSys       | **3.15.20** Control RTE for Linux ARM (DOGRULANDI 2026-06-09: `/root/codesyscontrol_3.15.20_exit`) |
| EtherCAT      | Master, CoE, CiA402                        |
| Modbus        | RTU (RS485) + TCP (port 502/503)           |
| Ethernet      | 192.168.1.123 (default)                    |
| GPU           | Framebuffer; OpenGL durumu Faz 5'te kontrol |

## Kanit: FreeCAD'in Bu Cihazda Calistigi

`How to Run FreeCAD(2).docx` belgesi, FreeCAD 1.0.2 AppImage'inin bu cihazda
basariyla calistirildigini gosteriyor. Adimlar:

```bash
# Cihaz uzerinden:
mkdir -p /root/FreeCAD
cd /root/FreeCAD
# (AppImage USB/NFS uzerinden yerlestirildi)
./FreeCAD_1.0.2-conda-Linux-aarch64-py311.AppImage --appimage-extract

# Env vars (kritik):
export DISPLAY=:0
export QT_QPA_PLATFORM=xcb
export TSLIB_TSDEVICE=/dev/input/event1
export QT_QPA_GENERIC_PLUGINS=tslib:/dev/input/event1
export XDG_RUNTIME_DIR=/usr/lib/
export LD_LIBRARY_PATH=/lib:/usr/lib:/usr/lib/ts

# Launch:
/root/FreeCAD/squashfs-root/AppRun &
```

Bu MilCAM icin **iki onemli kanit:**

1. **FreeCAD UI bu cihazda calisir.** Yani slim FreeCAD = MilCAM yaklasimi
   donanim acisindan dogrulanmis.
2. **Touch + X11 + Qt birlikte calisir.** Qt VirtualKeyboard input method
   da calisma garantili.

## Bilinmeyen ve Faz 5'te Cevaplanacak Sorular

1. **RAM ne kadar?**
   - < 1 GB → MilCAM (slim FreeCAD ~600 MB-1 GB RSS) sigamaz. Plan iptal.
   - 1-2 GB → slim build'in agresif olmasi sart.
   - >= 2 GB → konforlu.
2. **OpenGL versiyonu nedir?** OCCT GL 2.1+ ister. Yoksa software rasterizer.
3. **TargetVisu windowed mode** (`targetvisuextern.cfg WindowType=0`) calisiyor mu?
4. **`wmctrl` cihazda kurulu mu?** Yoksa cross-build paketine ekle.
5. **`isolcpus`** kernel cmdline'da aktif mi? CodeSys CNC icin gerekli.
6. **CodeSys OPC UA server aktif mi?** SP17+ default acik degil.

## Jitter Disiplini

CodeSys CNC interpolatoru her ms servo komut uretir. Jitter > 200 µs =
servo izleme hatasi = parça kalitesi düşer veya makine durur.

MilCAM'in cihazda olusturabilecegi RT kontaminasyon:
- GPU draw call'lari → IRQ burst (FreeCAD viewport orbit ederken)
- USB touch event'leri → kernel IRQ
- Disk I/O (STEP import sirasinda buyuk dosya)
- Python GC pauses

**Mitigasyon:**

1. `isolcpus=2,3` kernel cmdline → CodeSys cekirdekleri izole.
2. systemd unit `freecad-milcam.service` icinde `CPUAffinity=0 1` → MilCAM
   non-RT cores.
3. CodeSys unit'inde `CPUAffinity=2 3` (mevcut vendor config'i kontrol et).
4. IRQ pinning: `/proc/irq/<n>/smp_affinity` ile GPU/USB IRQ'larini 0,1'e baglar.
5. Build sonrasi `cyclictest -p 80 -t1 -n -i 1000 -l 600000` ile dogrula.

**Hedef:** MilCAM aktifken (idle + orbit + STEP load testleri sirasinda)
`Max Latencies < 200 us`.

## TargetVisu Koeksistans Planlari

### Plan A — Pencereli TargetVisu + MilCAM Yan Yana

`/opt/codesys/targetvisu/targetvisuextern.cfg`:
```ini
[CmpTargetVisu]
WindowType=0
WindowSizeWidth=1024
WindowSizeHeight=720
WindowPositionX=0
WindowPositionY=48     ; uste 48px serit birak
```

48px'lik serit MilCAM'in window manager'inda HMI ↔ MilCAM gecis butonlari.
`wmctrl` ile fokus degisimi.

### Plan B — Fullscreen TargetVisu + Workspace Switch

Vendor build pencereli moda izin vermezse:
- TargetVisu fullscreen workspace 1
- MilCAM fullscreen workspace 2
- `wmctrl -s 0` ↔ `wmctrl -s 1`

### Plan C — Tek Pencere (TargetVisu yok)

WebVisu varsa MilCAM icinde `QWebEngineView`. Ama Help/Web disable'liyiz —
QtWebEngine yok. Plan C calismaz mevcut konfigde.

Karar Faz 5'te.

## Operator Beklentisi

1. Boot tamamlanir → TargetVisu varsayilan acilir.
2. Operator USB STEP getirir, takar.
3. Uste serit `[CAM]` butonu → MilCAM one gelir (`wmctrl -a MilCAM`).
4. File → Open → STEP → operator gelisik FreeCAD CAM is akisini izler.
5. Touch ile sayisal deger girisi gerekirse Qt VirtualKeyboard yukarı kayar.
6. `Send to CodeSys` butonu → G-code dropuyor + PLC haberdar.
7. `[HMI]` butonu → TargetVisu one gelir.
8. Operator `Run` basar.

Hicbir adimda fiziksel klavye gerekmez.

## Cihaz Erisim

- IP: `192.168.1.123` (manual default)
- SSH: `root@192.168.1.123`
- microSD: `/dev/mmcblk1`

**Production parolalari .ai/'ya yazmayin.** Vault kullanin.
