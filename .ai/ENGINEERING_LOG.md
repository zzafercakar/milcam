# Engineering Log

Karsilasilan teknik sorunlar, root cause, cozumler. Yeni en uste.

---

## 2026-06-09 (aksam) — Full envanter cekildi, Plan A onaylandi

**Olay:** Serial console acildiktan sonra, cihaza host'tan otomasyonlu komut
gondererek tum donanim envanteri toplanmasi gerekiyordu.

**Engeller (sirayla cozuldu):**

1. **Yetki:** `/dev/ttyS0` `dialout` grubunda; embed user `dialout`'ta degildi
   ve sudo passwordless yoktu. Cozum: kullanici `sudo chmod o+rw /dev/ttyS0`
   ile gecici dunya-yazilabilir yapti.

2. **ModemManager kilidi:** stty "Device or resource busy" verdi. Sistemde
   `ModemManager.service` aktifti ve seri port'u probe ediyordu. Cozum:
   kullanici `sudo systemctl stop ModemManager`.

3. **Zombi screen process:** Onceki seans'tan kalan root altinda
   detached SCREEN process (PID 31899) port'u tutuyordu. fuser/lsof onu
   gormedi (kullanicidan sudo gerek), ama `ps -ef | grep SCREEN` ortaya
   cikardi. Cozum: `sudo killall screen` + VMware Settings → Removable
   Devices → Serial → Disconnect/Connect (kernel-level port reset).

**Cekim metodu:**

Yetkiler temizlendikten sonra otomatik script ile:
```bash
stty -F /dev/ttyS0 115200 cs8 -cstopb -parenb -ixon -ixoff -crtscts raw -echo
timeout 55 cat /dev/ttyS0 > /tmp/serial.log &
# Komutlari send() helper'i ile 1-3 sn uyku ile gonder
```

Sentinel marker pattern: her komut oncesi `echo "--- isim ---"` ile
bolum ayraci. Bu pattern busybox shell uzerinde calisti ve log
parse edilebilir kaldi.

**Sonuclar (ozet):**

- **Plan A onaylandi.** RAM 2 GB, Disk 12.6 GB free, DRM/KMS var.
- SoC **NXP i.MX 8M** (cmdline 0x30890000), 4× Cortex-A53 aarch64
- **PREEMPT-RT kernel** (RT-aware — bonus!)
- SSH yok (Faz 0.8'de eklenecek), vsftpd var (FTP transfer hazir)
- CODESYSControl.cfg'de `[SysProcess]` bolumu YOK — eklenmesi gerek

Ham log: [DEVICE_INVENTORY_2026-06-09.log](DEVICE_INVENTORY_2026-06-09.log)
(224 satir, 6422 byte)

**Kalici onlemler:**

- [DEVICE_PROFILE.md](DEVICE_PROFILE.md) tam canli envanter + Plan A
  gerekcesi
- [WORKPLAN.md](WORKPLAN.md) Faz 0.7 ✅, Faz 0.8 (cihaz hazirligi) eklendi
- VS Code workspace: tek-klasor moda gecirildi (user tercihi)
- ModemManager geri baslamasin diye disable kalici cozum:
  ```bash
  sudo systemctl disable ModemManager
  ```
  (kullanici karari)

---

## 2026-06-09 (oglen) — Serial console baglantisi (3 girisim, basarili)

**Olay:** VEC-VE'ye RS232 uzerinden Linux shell erisimi gerekti. Uc adimda
basarildi.

**Adim 1 (basarisiz):** Eski USB-RS232 donusturucu + standart DB9 kablo.
`sudo screen /dev/ttyS0 115200,cs8,-ixon,-ixoff,-crtscts` ile baglanildi
ama **surekli garbage karakter** akti (`|••÷` gibi yuksek-bit semboller
tekrarli). Olas neden: donusturucu kalitesi veya pinout uyumsuzlugu.

**Adim 2 (basarisiz):** Yeni MOXA UPort donusturucu + ayni standart kablo
takildi. Bu sefer **bos ekran**. Cihaz Windows tarafindan COM1'e atandi
(Device Manager dogruladi). VMware'in fiziksel seri portu /dev/ttyS0'a
geciriyor — host'ta uartclk=1843200 type=16550A.

**Network probe** yapildi paralelde: cihaza ping 3 ms RTT,
**port 11740 (CodeSys gateway) acik**, SSH/Telnet/HTTP/WebVisu/OPC UA
kapali. Yani serial console **tek interaktif yol**.

**Root cause analizi:** VEC-VE'nin DB9 pinout'u **standart degil**
(.docx Pin 4=RX, 8=TX, 6=GND vs standart 2=RX, 3=TX, 5=GND). Standart
kablo ile MOXA TX'i cihazin yanlis pinine, MOXA RX'i bos hatta dusuyor.

**Adim 3 (basarili):** Kullanici dogru kabloyu bagladi (muhtemelen vendor
ozel kablosu veya elle crossover yapti). Hem Windows SecureCRT hem Ubuntu
VM screen ile root@/root/ prompt'una erisim saglandi. `ls -l /root` cikti:
CodeSys runtime 3.15.20, Modbus param dosyalari, GPIO/touch/SPI test
binary'leri gorulebildi.

**Cozum (gelecek icin):**

- [SERIAL_CONSOLE_ACCESS.md](SERIAL_CONSOLE_ACCESS.md) — adim adim
  baglanti rehberi + pinout uyari + karar agaci.
- [DEVICE_PROFILE.md](DEVICE_PROFILE.md) — cihazdan alinan bilgiler +
  henuz alinmayan envanter komutu (kullanici bunu calistirip cevap
  doldurulacak).
- [NETWORK_PROBE_2026-06-09.md](NETWORK_PROBE_2026-06-09.md) — port
  taramasi sonuclari + servis baslatma onerileri.

**Acik konu:** Tam donanim envanteri (RAM, CPU model, kernel version,
disk, OpenGL/DRM) henuz alinmadi. Komut blogu DEVICE_PROFILE.md'de hazir,
bir sonraki oturumda ilk is olarak calistirilacak.

---

## 2026-06-08 — Pivot: kendi UI'yi sil, FreeCAD'i kullan

**Olay:** Ilk MilCAM scaffold'unda yanlislikla CADNC'nin mimarisini taklit
edip kendi Qt6 QML UI yaptik. Kullanici "FreeCAD'in arayuzu birebir
kullanilsin" diye duzeltti.

**Root cause:** Ilk yorumda "MilCAM = lighter MilCAD" → custom-app
varsayimi yaptim. Kullanici'nin "MilCAM gibi basit bir program" + "G-code
uretsin" cumlesini, MilCAD kod tabani esinli bagimsiz uygulama olarak
yorumladim. Olmaliydı: "MilCAD'in MilCAM'i" = onun gibi minimal bir CAM
arac, ama mevcut bir CAD framework ustune kurulu.

**Cozum:**

- Tum custom UI sildim: `ui/`, `app/`, `viewport/`, `util/`, `cam/`,
  `adapter/`, `resources/`, vendorlanmis `freecad/` subset.
- Yeni mimari: FreeCAD'i `add_subdirectory(${FREECAD_SOURCE_DIR})` ile
  build et + `BUILD_<X>=OFF` flag'leriyle CAM-only kalsin.
- `milcam/CodesysBridge/` = saf C++ + CPython binding (drop folder + OPC UA).
- `milcam/Mod/MilCAM/` = Python workbench overlay (hide non-CAM WBs, add
  Send-to-CodeSys + Switch-to-HMI commands, codesys postprocessor).

**Kalici onlem:** CLAUDE.md'de "DO NOT build custom UI" kuralı. .ai/
docs'ta workbench overlay yaklasiminin kararı belgelendi.

---

## 2026-06-08 — VEC-VE doc CADNC'den otomatik kopyalandi

**Olay:** Initial scaffold sirasinda `doc/` dizinine CADNC'den
SolidWorks PDF'leri ve CADNC ARCHITECTURE.md/PRD.md aniden geldi.

**Root cause:** Bilinmiyor — `mkdir -p ... doc` yaptigimda bos olmasi
beklenirdi. Belki bir hook copy etti veya disk durumu varsayilanindan
farkli.

**Cozum:** SolidWorks + CADNC docs silindi. Sadece VEC-VE/CodeSys ile
ilgili dosyalar tutuldu. Pivot sonrasi `doc/` aynı kalır.

---

## Bos — Doldurulacak (Faz 1 build denemesinden sonra)

### Beklenen Hata Listesi

- [ ] `find_package(Coin3DDoc)` ya da `Coin3D` host'ta yok ise FreeCAD build sirasinda eksiklik raporu — system package list'e ekle.
- [ ] `add_subdirectory(${FREECAD_SOURCE_DIR})` FreeCAD'in CMake'i `${CMAKE_SOURCE_DIR}` varsayimlari ile karisikilik yapabilir. Belirti: include path'leri yanlis cikar, generated header'lar bulunamaz.
- [ ] `BUILD_HELP=OFF` rağmen FreeCAD bazi modullerden `Help` symbol cagiriyorsa link hatasi.
- [ ] `BUILD_BIM=OFF` ama bir ust modul (`Arch`?) ona link ediyorsa.
- [ ] `BUILD_TEST=OFF` FreeCAD'in kendi self-test'leri icin gerek olabilir.
- [ ] `python3.X-dev` ve FreeCAD'in Python ABI'si esit olmali; cross-build'de sysroot'ta dogru Python.
- [ ] `pytest` yoksa MilCAM_tests target gozukmez (uyari ile gecer; build kirilmez).
- [ ] Qt VirtualKeyboard plugin'i FreeCAD'in Qt build'inde yok ise `QT_IM_MODULE=qtvirtualkeyboard` set edilmesi etkisiz kalir.

### Sahaya Indirilirse

- [ ] aarch64 cross-compile sirasinda Coin3D ve Python sysroot'a kopyalanmis mı?
- [ ] systemd unit'de `Environment=` satirlari saha cihazinda yeterli mi?
- [ ] `wmctrl` AppImage'a paketlenmis mi yoksa cihazda yuklu mu olmali?

---

## Sablon — Yeni sorun eklerken

```markdown
## YYYY-MM-DD — Kisa baslik

**Olay:** Ne oldu? Hangi komut/build adimi?

**Belirtiler:** Hata mesaji, log satiri.

**Root cause:** Niye oldu?

**Cozum:** Ne yaptim?

**Kalici onlem (varsa):** CI guard, CMake check.
```
