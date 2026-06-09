# Serial Console Access — VEC-VE Panel PC

Bu belge, VEC-VE cihazına RS232 serial console üzerinden nasıl bağlanılacağını
belgeliyor. **İlk başarılı bağlantı 2026-06-09'da kuruldu**, sürecin tüm
ayrıntıları aşağıda.

## Çalışan Konfigürasyon (Bunu Kullan)

### Donanım

| Bileşen               | Detay                                                 |
| --------------------- | ----------------------------------------------------- |
| USB-RS232 dönüştürücü | **MOXA UPort** serisi (Windows'ta COM1, host'ta `/dev/ttyS0`) |
| Cihaz tarafı          | DB9 dişi konnektör (VEC-VE'nin RS232 portu)            |
| Pinout (VEC-VE)       | Pin 4=RX, Pin 8=TX, Pin 6=GND **(standart DEĞİL!)**     |
| Baud                  | **115200 8N1**                                          |
| Flow control          | **HEPSİ KAPALI** (no DTR/DSR, no RTS/CTS, no XON/XOFF) |

### Yazılım — Linux VM tarafından (önerilen)

```bash
sudo screen /dev/ttyS0 115200,cs8,-ixon,-ixoff,-crtscts
```

Çıkış: `Ctrl+A` → `k` → `y`.

Eğer dialout grubunda değilsen tek-sefer eklenir (logout/login sonrası
sudo gerekmez):
```bash
sudo usermod -aG dialout $USER
```

### Yazılım — Windows tarafından (SecureCRT)

Quick Connect ayarları:
- Protocol: Serial
- Port: COM1 (Device Manager'da MOXA USB Serial Port nereye atanmışsa)
- Baud: 115200, 8N1
- Flow Control: hepsi UNCHECKED

> Eğer hem Windows SecureCRT hem Linux VM'den aynı anda erişmek istersen
> **çakışır** — port kilitli olur. Önce birinden çık, sonra diğerine geç.

## VMware Konfigürasyonu

VM Settings → Serial Port → **Use physical serial port** → Connection: Windows
host'taki MOXA portu (Device Manager'da görüneni seç, COM1/COM6 vs.).

> Önemli: Windows tarafında MOXA dönüştürücü Device Manager'da görünmüyorsa
> önce sürücüyü yükle. CH340/PL2303/CP210x değil, MOXA UPort kendi sürücüsünü
> kullanır (Moxa.com'dan).

## Pinout Tehlikesi — Standart Olmayan!

VEC-VE'nin DB9 pinout'u **standart RS232 değil**. Standart bir DB9-to-DB9 düz
kablo **çalışmaz** — fiziksel olarak doğru hat kurulmaz, sonuç: terminal
bağlanır ama veri akmaz (boş ekran).

| Hat            | Standart DB9 RS232 | VEC-VE     | Sonuç eşleme  |
| -------------- | ------------------ | ---------- | --------------- |
| TX (host→cihaz)| Pin 3              | Pin 4      | **Cross gerek**  |
| RX (cihaz→host)| Pin 2              | Pin 8      | **Cross gerek**  |
| GND            | Pin 5              | Pin 6      | **Cross gerek**  |

### Özel Kablo (eğer yapman gerekirse)

DB9 dişi (host MOXA tarafı) ↔ DB9 erkek (VEC-VE tarafı) custom kablo:

```
Host (DB9 female)        VEC-VE (DB9 male)
    Pin 3 (TX)  ───────────► Pin 4 (RX)
    Pin 2 (RX)  ◄─────────── Pin 8 (TX)
    Pin 5 (GND) ─────────────  Pin 6 (GND)
```

Sadece bu 3 teli bağla, diğer pinleri **takma** (bazı pinler power olabilir,
yanlış bağlanırsa dönüştürücüye zarar verir).

> 2026-06-09 testinde **standart DB9-to-DIN kablo** ile veri akmadı.
> Şüphelendiğimiz durum: kablonun pinout'u standart RS232'ydi, VEC-VE'nin
> beklediği özel pinout'a uymadı.

## Bağlandıktan Sonra

Cihaz prompt'u `#` (root). Hiçbir parola istemiyor. Çalışma dizini `/root/`.

İlk komutlar:

```sh
ls -l           # CodeSys runtime + Modbus config + test binary'leri
cat /root/version /root/myversion       # versiyon damgaları
free -h         # RAM (BİLİNMEYEN — kritik)
df -h           # disk
uname -a        # kernel + arch
```

> Tam donanım envanter komutu için: [DEVICE_PROFILE.md](DEVICE_PROFILE.md)
> son bölümündeki tek-blok komut.

## Eğer Bağlanamıyorsan — Karar Ağacı

```
Boş ekran (hiç karakter gelmiyor) ?
  ├── Cihaz açık mı? (LCD açık, network ping atıyor mu)
  ├── Kablo doğru pinout mu? (.docx pinout vs standart RS232)
  ├── MOXA dönüştürücü Windows'ta görünüyor mu? (Device Manager → COM)
  └── VMware "Use physical serial port" + doğru COM seçili mi?

Çöp karakter (sürekli akan |••÷ vb) ?
  ├── Baud yanlıştır → 9600, 38400, 57600 sırasıyla dene
  ├── Veya pinout yanlıştır → cross/null-modem dene
  └── `sudo /tmp/baud_scan.sh` otomatik tarama scripti yardımcı olur

Prompt geliyor ama yazdığım görünmüyor ?
  └── `screen` local echo'yu açmadı. Ctrl+A → E ile local echo'yu aç
      veya `stty echo` cihaz tarafında

Çıkış komutu (Ctrl+A → k → y) çalışmıyor ?
  └── Başka terminal'den: `sudo killall screen` veya
      `sudo screen -ls` → `sudo screen -X -S <id> quit`
```

## Ek Notlar (.docx'ten)

`doc/How to Run FreeCAD(2).docx` belgesi serial console üzerinden Linux'a
girip NFS mount + FreeCAD AppImage çalıştırma sürecini anlatıyor. Aynı
yöntemle MilCAM AppImage'ı da deploy edebiliriz — bkz. WORKPLAN.md Faz 5.

Cihaz boot tamamlandıktan sonra üzerinde `ip_test.sh` çalıştırılırsa
network config'i hızlıca görülebilir (`/root/ip_test.txt`'e yazıyor olabilir).
