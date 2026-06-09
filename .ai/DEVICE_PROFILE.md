# VEC-VE Device Profile — Live Inventory

Cihazdan **gerçek zamanlı toplanan** ve toplanacak donanım/yazılım envanteri.
Belgenin altında "DURUM" tablosu var — hangi bilgi alındı, hangisi bekliyor.

> **2026-06-09:** Cihazla serial console üzerinden ilk başarılı bağlantı yapıldı
> (bkz. [SERIAL_CONSOLE_ACCESS.md](SERIAL_CONSOLE_ACCESS.md)). `ls -l /root` ve
> network probe sonuçları alındı. Tam donanım envanteri **bir sonraki adımda**
> tek-blok komutla alınacak (komut bu dosyanın sonunda).

## Cihaz Bilgi Tablosu

| Bilan                          | Değer (alınmış)                                         | Kaynak                              |
| ------------------------------ | -------------------------------------------------------- | ----------------------------------- |
| Model                          | VEC-VE-MU-AH 12 inch                                     | doc/VEC-VE-MU-AH 12 inch.pdf        |
| Vendor / OEM                   | Shenzhen Vector Technology                               | manual                              |
| OS family                      | Embedded Linux                                            | serial shell prompt = `#`           |
| Shell                          | busybox / sh                                              | shell behavior                      |
| Boot dir                       | `/root/`                                                  | observed                            |
| CodeSys runtime path           | `/root/codesyscontrol_3.15.20_exit` (7 006 184 byte)     | `ls -l /root`                       |
| CodeSys version (substring)    | `3.15.20` (executable name)                              | filename                            |
| CodeSys config dosyası          | `/root/CODESYSControl.cfg` (2 690 byte)                  | `ls -l /root`                       |
| Modbus param dosyaları           | `Myfile_RTU.txt`, `Myfile_TCP{,1,2,3,4}.txt` (2 KB each) | `ls -l /root`                       |
| Touch test binary'leri          | `touch_test`, `touch_test2`, `touch_tslib`               | `/root/`                            |
| GPIO test binary'leri           | `test_gpio_2`, `test_gpio_3`                             | `/root/`                            |
| SPI test                       | `spitest`, `spitest2`                                    | `/root/`                            |
| Network — IP                    | `192.168.1.123` (manual default; ping başarılı 3 ms RTT)  | manuel + ping                       |
| Network — açık port            | **11740** (CodeSys gateway)                              | TCP probe                           |
| Network — KAPALI portlar         | 22 (SSH), 23 (Telnet), 80, 8080 (WebVisu), 4840 (OPC UA) | TCP probe                           |
| Host (geliştirme) erişimi       | 192.168.181.130/24 — farklı subnet, router üzerinden      | `ip -4 addr`                        |
| RS232 baud                     | **115200 8N1, no flow control**                          | SecureCRT screenshot + .docx        |
| RS232 pinout                   | Pin 4=RX, Pin 8=TX, Pin 6=GND **(standart DEĞİL)**         | .docx                               |
| Display                        | 12" 1024×768 TFT (PLED 50K saat)                          | datasheet                           |
| Touch                          | 4-wire resistive, ±2% accuracy                            | datasheet                           |
| Touch device                   | `/dev/input/event1` (tslib)                              | .docx env vars                      |
| Display server                 | X11 (`DISPLAY=:0`, `QT_QPA_PLATFORM=xcb`)                | .docx env vars                      |
| Framebuffer                    | `/dev/fb0`                                                | .docx env vars                      |
| Font dir                       | `/usr/lib/fonts`                                          | .docx env vars                      |
| Qt plugin dir                  | `/usr/lib/qt/plugins`                                     | .docx env vars                      |
| tslib config                   | `/etc/ts.conf`, `/etc/pointercal`, `/usr/lib/ts`         | .docx env vars                      |
| FreeCAD denenmiş versiyon       | 1.0.2 AppImage (conda-Linux-**aarch64**-py311) — **çalışmıştı**| .docx adım 7                        |
| **Mimari (kanıtlı)**           | **aarch64 (ARM 64-bit)** — Cortex-A53 × 4                | live `uname -a` + cpuinfo           |
| **SoC**                        | NXP **i.MX 8M serisi** (mini veya plus)                  | cmdline 0x30890000 base             |
| **CPU**                        | 4× **Cortex-A53** + AES/SHA/CRC HW                       | /proc/cpuinfo CPU part 0xd03        |
| **RAM**                        | **2 037 148 kB ≈ 2 GB**                                  | live `free -h`                      |
| **Disk**                       | 13.8 GB eMMC, 12.6 GB free                                | live `df -h`                        |
| **GPU**                        | DRM/KMS DSI panel, `/dev/dri/card0`                       | live `ls /dev/dri`                  |
| **Kernel**                     | Linux **4.14.98-rt53** PREEMPT-RT                         | live `uname -a`                     |
| **OS**                         | **Buildroot 2019.08**                                     | /etc/os-release                     |
| **eth0 / eth1**                | 192.168.2.123 (down) / **192.168.1.123 (active)**         | live `ip addr`                      |
| **SSH daemon**                 | **YOK** (dropbear/sshd yüklü değil)                       | `which dropbear sshd`               |
| **FTP daemon**                 | **vsftpd VAR** (init.d S70vsftpd)                         | `ls /etc/init.d`                    |
| **DBus**                       | VAR                                                       | init.d S30dbus                      |

## Henüz Alınmadı — Bir Sonraki Adımda Çalıştırılacak

Aşağıdaki komutu serial console'da yapıştır, **çıktıyı bu dosyanın
"GERÇEK ÇIKTI" bölümüne yapıştır** (veya copy-paste ile):

```sh
echo "=== uname ==="; uname -a
echo "=== os-release ==="; cat /etc/os-release 2>/dev/null
echo "=== version dosyalari ==="; cat /root/version /root/myversion 2>/dev/null
echo "=== CPU ==="; grep -E "model name|Hardware|Features|processor|BogoMIPS" /proc/cpuinfo | sort -u
echo "=== RAM ==="; free -h; head -5 /proc/meminfo
echo "=== Disk ==="; df -h
echo "=== Network ==="; ip -4 addr 2>/dev/null || ifconfig 2>/dev/null | grep -E "inet |flags"
echo "=== Display / OpenGL ==="; ls /dev/dri/ 2>/dev/null; ls /sys/class/drm/ 2>/dev/null; ls /sys/class/graphics/ 2>/dev/null
echo "=== Kernel cmdline (isolcpus?) ==="; cat /proc/cmdline
echo "=== SSH/Telnet daemon var mi? ==="; which dropbear sshd telnetd 2>/dev/null
echo "=== Init scriptleri ==="; ls /etc/init.d/ 2>/dev/null | head -30
echo "=== Calisan process'ler ==="; ps 2>/dev/null | head -25 || ps -ef 2>/dev/null | head -25
echo "=== Acik portlar ==="; netstat -tlnp 2>/dev/null | head -15
echo "=== CodeSys SysProcess izinleri ==="; grep -iE "SysProcess|Command|allow" /root/CODESYSControl.cfg 2>/dev/null
echo "=== KERNEL ==="; uname -r; cat /proc/version 2>/dev/null
echo "=== bitti ==="
```

## GERÇEK ÇIKTI (2026-06-09 cihazdan canli alindi)

Ham log: [DEVICE_INVENTORY_2026-06-09.log](DEVICE_INVENTORY_2026-06-09.log)

### Kernel ve OS

```
Linux buildroot 4.14.98-rt53 #86 SMP PREEMPT Thu May 12 15:06:33 CST 2022 aarch64 GNU/Linux
NAME=Buildroot
VERSION=2019.08
ID=buildroot
PRETTY_NAME="Buildroot 2019.08"
Welcome to root
```

- Distribution: **Buildroot 2019.08** (custom embedded Linux)
- Kernel: **4.14.98-rt53** — **PREEMPT-RT** patched (real-time aware)
- Mimari: **aarch64 (ARM 64-bit)**
- Build tarihi: 2022-05-12 (Çin saati)

### CPU

```
BogoMIPS    : 16.00
CPU implementer : 0x41        (ARM Limited)
CPU part        : 0xd03       (Cortex-A53)
CPU revision    : 4
CPU variant     : 0x0
Features        : fp asimd evtstrm aes pmull sha1 sha2 crc32 cpuid
processor       : 0, 1, 2, 3  (4 core)
```

- **4× ARM Cortex-A53 64-bit**
- HW acceleration: AES, SHA1/SHA2, CRC32, PMULL (carry-less mul)
- NEON SIMD (asimd)
- BogoMIPS 16.00 → ~600 MHz–1.6 GHz nominal (RT kernel calibration düşük gösterir)

### RAM — **2 GB** ✓

```
              total        used        free      shared  buff/cache   available
Mem:        2037148      221836     1727544         720       87768     1753940
Swap:             0           0           0
MemTotal:        2037148 kB
MemFree:         1727668 kB
MemAvailable:    1754068 kB
```

- **Total: ~2 GB** (1990 MiB)
- Idle'da kullanılan: 217 MB (CodeSys + sistem)
- Available: 1.7 GB (MilCAM için bol)
- Swap yok (embedded — beklendiği gibi)

### Disk — 13.8 GB / 12.6 GB free

```
Filesystem                Size      Used Available Use% Mounted on
/dev/root                13.8G    464.1M     12.6G   3%  /
devtmpfs                674.1M         0    674.1M   0%  /dev
tmpfs                   994.7M         0    994.7M   0%  /dev/shm
tmpfs                   994.7M    92.0K    994.6M   0%  /tmp
tmpfs                   994.7M    168.0K    994.5M   0%  /run
```

- Root partition 13.8 GB eMMC, **12.6 GB boş** (FreeCAD kurulumu için bol)
- /tmp, /run, /dev/shm = tmpfs (RAM-backed, hızlı)

### Network (2 NIC!)

```
eth0: NO-CARRIER (kablo takili degil), 192.168.2.123/24
eth1: UP, LOWER_UP, 192.168.1.123/24  ← BIZIM ERISIM
      + 169.254.159.99/16 (link-local fallback)
```

- İki ayrı Ethernet portu → endüstriyel pattern (PLC LAN + HMI LAN ayırma)
- eth1 bizim development host'tan eriştiğimiz

### GPU / Display

```
/dev/dri/by-path  card0       ← DRM/KMS modern stack VAR
/sys/class/drm/   card0  card0-DSI-1  version
/sys/class/graphics/  fb0  fbcon
```

- **DRM/KMS DSI panel** — modern Linux grafik stack
- DSI-1 (MIPI Display Serial Interface) doğrudan paneli sürüyor
- DRM card0 var → Mesa + Etnaviv veya Vivante GPU driver

### Kernel Boot Cmdline (SoC tespiti!)

```
console=ttymxc1,115200 earlycon=ec_imx6q,0x30890000,115200 root=/dev/mmcblk2p2 rootwait rw
```

- `ttymxc1` = NXP i.MX serisi UART driver
- `earlycon=ec_imx6q,0x30890000` → **SoC: NXP i.MX 8M** (Mini veya Plus; adres
  0x30890000 i.MX 8M ailesinin UART2 base'i; "imx6q" early-console isim *legacy*
  string olabilir, gerçek SoC i.MX 8M)
- `root=/dev/mmcblk2p2` → eMMC kart partition 2'de root FS

### SSH durumu — **YOK**

```
which dropbear sshd telnetd  → ciktisiz
ls /usr/sbin/dropbear        → yok
ls /usr/sbin/sshd            → yok
```

- Hiçbir SSH/Telnet daemon yüklü değil
- Dosya transferi için **vsftpd** var (init.d S70vsftpd)

### init.d (boot scripts)

```
S01syslogd   S02klogd    S02sysctl
S10mdev      S10udev     S20urandom
S21rngd      S30dbus     S40network
S41dhcpcd    S70vsftpd
rcK          rcS
```

- DBus var (Qt için iyi)
- vsftpd FTP server aktif
- network + dhcpcd standard

### Çalışan Process'ler (önemli kısım)

```
PID 1 = init
Cortex-A53 4 core için cpuhp, migration, posixcputmr, rcuc, ksoftirqd × 4
```

(Userspace daemon'ları görmek için `ps aux` veya `ps -ef` daha verimli olur —
bir sonraki sefer.)

### CODESYSControl.cfg — Önemli Ayrıntılar

Tam içerik [DEVICE_INVENTORY_2026-06-09.log](DEVICE_INVENTORY_2026-06-09.log)
sonundaki blok. Önemli:

```ini
[SysCpuHandling]
Linux.DisableCpuDmaLatency=1       ← RT için iyi

[SysExcept]
Linux.DisableFpuUnderflowException=1
Linux.DisableFpuOverflowException=1

[CmpSchedule]
DisableOmittedCycleWatchdog=1      ← test için OK, production'da dikkat

[CmpApp]
Bootproject.RetainMismatch.Init=1
Application.1=Application

[CmpHilscherCIFX]                  ← Hilscher fieldbus kart desteği aktif
DynamicFirmware=1
BootloaderFile=./HilscherCIFX/Firmware/NETX100-BSL.bin

[CmpBACnet]                        ← BACnet protokol desteği
IniFile=bacstac.ini

[CmpSecureChannel]
CertificateHash=89e5a93df8f8c49bdf67a6d7621344fb4b2872d6
```

**KRİTİK EKSİK:** `[SysProcess]` bölümü **YOK**. MilCAM'in
`SysProcessExecuteCommand` (Send-to-CodeSys'in fallback yolu) kullanması
için eklenmesi gerek:
```ini
[SysProcess]
Command=AllowAll
```

`[CmpTargetVisu]` bölümü var ama parametreleri yorum (`;`) ile kapalı —
yani şu an TargetVisu **stock fullscreen** modunda çalışıyor.
Windowed mode için açılması gerekecek:
```ini
[CmpTargetVisu]
WindowPositionX=0
WindowPositionY=48
WindowSizeWidth=1024
WindowSizeHeight=720
```

## Plan A/B/C Karar — **PLAN A** ✅

| Kriter           | Hedef    | Gerçek    | Sonuç          |
| ---------------- | -------- | --------- | -------------- |
| RAM              | ≥ 2 GB   | **2 GB**  | ✓ Plan A       |
| OpenGL/DRM       | KMS var  | DSI-1 KMS | ✓ Mesa kullanılabilir |
| Boş disk         | ≥ 500 MB | **12.6 GB** | ✓ rahat       |
| Arch             | aarch64  | aarch64    | ✓             |
| RT kernel        | İdeal    | PREEMPT-RT | ✓ bonus!      |

**Karar:** Slim FreeCAD + MilCAM overlay **cihaza deploy edilebilir**.
SSH yok ama cross-build sonrası vsftpd üzerinden veya CodeSys-aware
deploy script ile transfer mümkün.

## Sıradaki Adımlar (Faz 1'e geçiş)

1. ⚠️ **`CODESYSControl.cfg`'ye `[SysProcess] Command=AllowAll` ekle** —
   bunu cihazda yaparız (serial ile veya FTP ile cfg dosyasını çek-değiştir-yaz).
2. **Dropbear cross-build + cihaza kopyala** — sonraki erişimler için SSH
   serial'dan çok daha pratik. Buildroot için statik aarch64 dropbear binary
   yeterli (~500 KB).
3. **MilCAM Faz 1: ilk end-to-end build denemesi** — host'ta `cmake --preset default`.
4. Faz 1 başarılı olursa **Faz 5'e atla**: panel-pc preset ile cross-build +
   `scripts/deploy.sh` ile vsftpd üzerinden cihaza yolla.

## Bilinen `/root/` İçeriği (2026-06-09 alındı)

Cihazda root home klasörü = CodeSys runtime'ın çalıştığı yer.

| Dosya / dizin                              | Boyut    | Tarih          | Anlamı                                     |
| ------------------------------------------ | -------- | -------------- | ------------------------------------------ |
| `CODESYSControl.cfg`                       | 2 690    | Jan 1 00:35    | CodeSys runtime ayarları (kritik!)         |
| `Myfile_RTU.txt`                           | 2 000    | Jan 1 01:04    | Modbus RTU param dosyası                    |
| `Myfile_TCP.txt`                           | 2 000    | Jan 1 01:04    | Modbus TCP master                          |
| `Myfile_TCP1.txt` … `Myfile_TCP4.txt`      | 2 000 ea | Jan 1 01:04    | Modbus TCP slave/connection param        |
| `PlcLogic/` (dir)                          | 4 096    | Jan 1 00:00    | PLC retain / persistence data              |
| `ReadModbusParaServer`                     | 23 272   | Apr 16 2024    | Modbus param sunucu binary                 |
| `SysFileMap.cfg`                           | 9 478    | Jan 1 00:00    | Filesystem mapping for SysFile lib         |
| `cert/` (dir)                              | 4 096    | Feb 9 2021     | TLS sertifika dizini                        |
| `codesyscontrol_3.15.20_exit`              | 7 006 184| Apr 16 2024    | CodeSys runtime executable (CodeSys 3.5.15.20)|
| `codesyscontrol_backup.zip`                | 4 611 618| Jun 2 2022     | Backup of previous runtime                 |
| `hostshare`                                | 67       | Dec 12 2022    | Mount hint / pipe                          |
| `iic_358774`                               | 10 192   | Apr 16 2024    | I2C tool?                                  |
| `ip_test.sh` + `ip_test.txt`               | 578 + 14 | Aug 10 2020    | Network self-test                          |
| `mod/` (dir)                               | 4 096    | Jan 1 00:00    | Custom kernel modules?                     |
| `myversion`                                | 35       | Feb 9 2021     | Custom version stamp                       |
| `save_codesyscontrol_3.15.20_exit.zip`     | 2 308 442| Jul 11 2022    | Older runtime backup                       |
| `spitest`, `spitest2`                      | 483 720 ea| Apr 16 2024   | SPI loopback tests                         |
| `test`                                     | 30       | Jul 2 2024     | Tiny test                                  |
| `test_gpio_2`, `test_gpio_3`               | 6 056 / 10 176 | Apr 16 2024 | GPIO toggle tests                        |
| `touch_test`, `touch_test2`, `touch_tslib` | 6 056 / 6 080 / 6 072 | Apr 16 2024 | Touch panel tests                  |
| `untitled`                                 | 23 288   | Apr 16 2024    | unknown                                    |
| `version`                                  | 12       | Jul 2 2024     | Version string                             |

## CODESYSControl.cfg — Etkili Olabilecek Ayarlar

Henüz içeriği görünmedi (tam komut bloğunda alacağız). MilCAM'in
"Send to CodeSys" komutunun `SysProcessExecuteCommand` çağırabilmesi
için **şu satırların var olması gerekiyor**:

```ini
[SysProcess]
Command=AllowAll
```

Eğer yoksa, MilCAM deploy sırasında deploy.sh bu blokları ekleyecek.

## Hızlı SSH Aç (eğer `dropbear` cihazda yüklü ise)

Yukarıdaki blok çıktısında `which dropbear` bir yol döndürürse, serial
console yerine ağ üzerinden çalışmaya geçebiliriz. Hızlı aç:

```sh
# Cihazda:
dropbear -B -p 22 &
# Test (host'tan):
ssh root@192.168.1.123
```

Eğer ssh kabul etmiyorsa parola lazımdır. Yeniden başlatınca uçar; kalıcı
olmasını istersen `/etc/init.d/` altına symlink ekle veya `rc.local`'a yaz.
