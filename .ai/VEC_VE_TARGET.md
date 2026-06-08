# VEC-VE Target Platform Notes

MilCAM'in tasarımının her noktası bu donanımdan beslenir. Bu dosyayı oku.

## Donanım Profili (Doğrulanmış)

Kaynak: `/home/embed/Dev/CADNC/doc/VEC-VE-MU-AH 12 inch.pdf` + `How to Run FreeCAD(2).docx`.

| Özellik       | Değer                                      |
| ------------- | ------------------------------------------ |
| Model         | VEC-VE-MU-AH 12 inch                       |
| CPU           | Quad-core ~1.6 GHz Cortex-A sınıfı (ARM)   |
| RAM           | **DATASHEET'TE BELİRTİLMEMİŞ — Faz 0.5'te ölçülecek** |
| Disk          | microSD (TICard) + dahili flash            |
| Display       | 12" 1024×768 TFT, PLED backlight 50K saat  |
| Touch         | 4-tel dirençli, ±%2 doğruluk               |
| OS            | Embedded Linux (busybox userland)          |
| Display srv   | X11, `DISPLAY=:0`, `QT_QPA_PLATFORM=xcb`   |
| Touch device  | `/dev/input/event1` (tslib)                |
| CodeSys       | 3.5.15.10, Control RTE for Linux ARM       |
| EtherCAT      | Master, CoE, CiA402                        |
| Modbus        | RTU (RS485) + TCP (port 502/503)           |
| Ethernet      | 192.168.1.123 (default)                    |
| GPU           | Çerçeve tampon (framebuffer); OpenGL durumu Faz 0.5'te kontrol |

## Yazılım Çevresi (Kanıt)

`How to Run FreeCAD(2).docx`'ten alınan başarılı çalışan env-var seti
(MilCAM da bunları kullanmalı):

```bash
export DISPLAY=:0
export QT_QPA_PLATFORM=xcb
export TSLIB_TSDEVICE=/dev/input/event1
export QT_QPA_GENERIC_PLUGINS=tslib:/dev/input/event1
export XDG_RUNTIME_DIR=/usr/lib/        # tipik geçici workaround
export LD_LIBRARY_PATH=/lib:/usr/lib:/usr/lib/ts
```

FreeCAD 1.0.2 AppImage bu cihazda bir kez **başarıyla** çalıştırıldı — bu
"CAM-class Qt6+OCCT uygulaması yürür" kanıtıdır.

## Bilinmeyen ve Faz 0.5'te Cevaplanacak Sorular

1. **RAM ne kadar?** En kritik soru. < 1 GB ise MilCAM çalıştırma yerine
   workstation modeli (Plan C).
2. **OpenGL versiyonu?** OCCT 7.6 GL 2.1+ ister. Yoksa software rasterizer
   (yavaş).
3. **TargetVisu windowed mode (`WindowType=0`) destekli mi?** Vendor build'i
   stock CodeSys ayarlarını değiştirmiş olabilir.
4. **`wmctrl` / `xdotool` cihazda kurulu mu?** Yoksa toolchain'e ekle.
5. **`isolcpus` kernel cmdline'da var mı?** RT izolasyon zorunlu.
6. **OPC UA server CodeSys'te aktif mi?** SP17+'da default değil.

Komutlar `WORKPLAN.md` Faz 0.5'te listelendi.

## Jitter Discipline (KRİTİK)

CodeSys CNC interpolatörü her ms'de servo komut üretir. Jitter > 200 μs
**servo izleme hatasına** sebep olur (parça hatalı, makine durur).

MilCAM'in jitter'a katkısı:
- GPU draw call'ları → IRQ storm potansiyeli
- USB-touch event'leri → kernel IRQ
- Disk I/O (özellikle import sırasında STEP okurken)

**Mitigasyon kuralları:**
1. CodeSys çekirdekleri (`isolcpus=2,3` veya benzeri) MilCAM'in **dışında**.
2. MilCAM `taskset -c 0,1 milcam` ile pinlenir.
3. Büyük import işlemleri background thread'de, throttled.
4. cyclictest disiplini: her özellik PR'ı öncesi/sonrası **ölçülmeli**:
   ```bash
   cyclictest -p 80 -t1 -n -i 1000 -l 600000 -m
   ```
   Hedef: `Max Latencies < 200 us` MilCAM aktifken (orbit + import dahil).

## TargetVisu Koeksistans Senaryoları

### Plan A — Windowed TargetVisu + MilCAM Side-by-Side

`/opt/codesys/targetvisu/targetvisuextern.cfg`:
```
[CmpTargetVisu]
WindowType=0
WindowSizeWidth=1024
WindowSizeHeight=720           ; üst şerit 48px ayır
WindowPositionX=0
WindowPositionY=48
```

MilCAM aynı X server'da paralel pencere. Üst şerit (milcam-launcher) `wmctrl -a`
ile birini öne getirir.

### Plan B — Fullscreen TargetVisu + Workspace Switch

Vendor build pencereli moda izin vermezse:
- TargetVisu fullscreen workspace 1'de
- MilCAM fullscreen workspace 2'de
- `wmctrl -s 0` ↔ `wmctrl -s 1` ile switch

### Plan C — Tek Pencere WebVisu

TargetVisu'yu hiç kullanma, CodeSys WebVisu'yu MilCAM'in içine
`QWebEngineView` ile göm. Tek pencere, sekme tabanlı UI. Maliyet: Chromium
runtime ~150 MB extra disk + RAM.

Karar Faz 0.5 sonrası verilecek.

## Operator Workflow Beklentisi

1. Cihaz açılır → boot tamamlanır → launcher şeridi gözükür, TargetVisu
   varsayılan görünür.
2. Operator ofisten USB ile STEP getirir, USB'yi takar.
3. Üst şeritteki `[CAM]` butonuna dokunur → MilCAM öne gelir.
4. `Import` butonuna dokunur → dosya seçici → STEP yüklenir.
5. `Operations` → sayısal değer girişi gerektiğinde Qt VirtualKeyboard otomatik
   yukarı kayar.
6. `Post + G-code` → önizleme.
7. `→ Send to CodeSys` → G-code drop edilir, OPC UA üzerinden PLC haberdar.
8. `[HMI]` butonuna dokunur → TargetVisu öne gelir → operator `Run` basar.

Hiçbir adımda klavye gerekmez. Dosya yolları, iş isimleri, operasyon
parametreleri hep Virtual Keyboard ile.

## Cihaz Erişim Bilgileri (Şimdilik)

- IP: `192.168.1.123` (varsayım, manuel'deki default)
- SSH: `root@192.168.1.123` (parola sahada belirlenir)
- NFS mount edilebilir: `busybox mount -o nolock -t nfs ...`
- microSD slot dış arayüz: `/dev/mmcblk1` tipik

**Üretim cihazında SSH/parola kayıtları `.ai/`'ya YAZILMAMALI.** Ayrı bir
şifreli store (1Password, vault) kullan.
