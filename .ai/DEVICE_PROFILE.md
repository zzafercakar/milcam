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
| **Mimari (kanıtlı)**           | **aarch64 (ARM 64-bit)** — FreeCAD AppImage'tan          | aarch64 string in filename          |

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

## GERÇEK ÇIKTI (cihazdan)

```
{TODO — kullanıcı yukarıdaki bloğu çalıştırıp çıktıyı buraya yapıştıracak}
```

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
