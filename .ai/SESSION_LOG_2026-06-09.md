# Session Log — 2026-06-09

Bu belge, bu oturumda yapilan tum ilerlemenin ozet kaydidir. MilCAM projesi
acildiginda buradan baslayip kaldigimiz yerden devam edilebilir.

## Bu Oturumda Ne Yapildi

### 1. CADNC oturumunda VEC-VE feasibility sorusu

Kullanici sordu: VEC-VE panel PC'de MilCAM (veya hafifletilmis FreeCAD)
ile CodeSys HMI birlikte calisabilir mi, geçis butonu olsa?

Iki paralel arastirma yapildi (Explore agent + general-purpose agent):
- **Doc deep-dive:** VEC-VE manueli, datasheet, "How to Run FreeCAD.docx"
- **Web research:** CodeSys 3.5 TargetVisu mimarisi, SysProcess library,
  WebVisu vs TargetVisu, Qt VirtualKeyboard, wmctrl-kiosk pattern

Sonuc: **Teknik olarak mumkun**, hatta FreeCAD 1.0.2 AppImage bu cihazda
zaten basariyla calistirilmis (.docx kaniti).

### 2. MilCAM proje iskeleti olusturuldu

Ilk attemt: CADNC tarzi mimari (custom Qt6 QML UI + cam/ + adapter/).
Kullanici "yeni UI yapilmasin, FreeCAD birebir kullanilsin" diye duzeltti.

### 3. Pivot: slim FreeCAD + Python workbench overlay

Tum custom UI/adapter/cam/viewport silindi. Yeni mimari:

- `CMakeLists.txt` → `add_subdirectory(${FREECAD_SOURCE_DIR})` ile FreeCAD'i
  build et + ~20 module `BUILD_<X>=OFF` flag ile devre disi
- `milcam/CodesysBridge/` → saf C++ + CPython stable ABI binding
  (drop folder + wmctrl + OPC UA stub)
- `milcam/Mod/MilCAM/` → Python workbench overlay
  - InitGui.py: non-CAM workbench'leri gizler, MilCAM default WB yapar
  - Commands: SendToCodesys, SwitchToHmi, AboutMilCAM
  - PostProcessor/codesys_post.py: FreeCAD post pattern'inde CODESYS G-code
  - Resources/icons/*.svg

`.ai/` belge seti rewritten: START_HERE, context.yaml, WORKPLAN,
ARCHITECTURE, FREECAD_SUBSET, CODESYS_BRIDGE, VEC_VE_TARGET,
REFERENCE_SOURCES, ENGINEERING_LOG, NEXT_SESSION_PROMPT, HISTORY_PIVOT.

Git: 3 commit (ccc6922 init, e860f21 doc prune, ef716a2 pivot).

### 4. VEC-VE serial console kurulumu

Kullanici VMware seri portu uzerinden RS232 dönüşturucu ile cihaza
takmaya basladi.

**Yapilan adimlar:**

1. `/dev/ttyS0` host tarafinda VMware'in expose ettigi port olarak
   tespit edildi (uartclk=1843200, type=16550A, /sys/class/tty/ttyS*/
   ile sudo'suz da bulundu).
2. SecureCRT screenshot'undan baud=115200, 8N1, no flow control teyit edildi.
3. Ilk denemede `sudo screen /dev/ttyS0 115200,cs8,-ixon,-ixoff,-crtscts`
   ile baglandi ama **garbage karakter akti** (eski dönüşturücü kalitesiz).
4. Yeni MOXA UPort dönüşturücu denendi → **bos ekran**, hat sessiz.
5. Network probe yapildi paralelde:
   - ping 192.168.1.123 = 3 ms RTT ✓
   - Port 11740 (CodeSys gateway) AÇIK ✓
   - SSH/Telnet/HTTP/WebVisu/OPC UA HEPSI KAPALI
6. Teshis: pinout uyumsuzlugu (.docx pin 4=RX 8=TX vs standart 2/3/5)
7. Kullanici **dogru kabloyu bagladi**, baglanti basarili oldu.
8. `ls -l /root` cikti — CodeSys 3.15.20 runtime, Modbus dosyalari,
   GPIO/touch/SPI test binary'leri gorulebildi.

### 5. Full envanter cihazdan otomatik cekildi (aksam seans)

Engeller asildi:
- **`dialout` grubunda degildim** → kullanici `sudo chmod o+rw /dev/ttyS0`
- **ModemManager** seri portu probe ediyordu → `sudo systemctl stop ModemManager`
- **Zombi root SCREEN process** (PID 31899) → `sudo killall screen` + VMware
  port disconnect/connect

Otomatik script ile cihazdan envanter cekildi (stty + arka plan cat reader +
`printf '%s\r'` ile komut gonderme + sentinel marker pattern).

**Sonuc: PLAN A onaylandi.**

- RAM: 2 037 148 kB (~2 GB)
- Disk: 13.8 GB eMMC, 12.6 GB free
- CPU: 4× ARM Cortex-A53 aarch64
- SoC: NXP i.MX 8M (cmdline 0x30890000)
- Kernel: Linux 4.14.98-rt53 **PREEMPT-RT** SMP
- OS: Buildroot 2019.08
- GPU: DRM/KMS DSI panel (Mesa kullanilabilir)
- SSH: YOK, vsftpd VAR (FTP transfer hazir)
- CodeSys 3.15.20, CODESYSControl.cfg'de `[SysProcess]` YOK

Ham log: [DEVICE_INVENTORY_2026-06-09.log](DEVICE_INVENTORY_2026-06-09.log).

### 6. Tum bulgular MilCAM belleğine aktarildi

Yeni dosyalar:
- [DEVICE_PROFILE.md](DEVICE_PROFILE.md) — cihaz envanteri
  + henüz alınmayan komut blogu (Faz 0.7'de calistirilacak)
- [SERIAL_CONSOLE_ACCESS.md](SERIAL_CONSOLE_ACCESS.md) — adim adim
  baglanti, pinout uyari, karar agaci
- [NETWORK_PROBE_2026-06-09.md](NETWORK_PROBE_2026-06-09.md) — port
  tarama sonuclari

Güncellenen dosyalar:
- ENGINEERING_LOG.md — 2026-06-09 girisi (serial console kurulum)
- WORKPLAN.md — Faz 0.6 (cihaz erisimi) tamamlandi, Faz 0.7 (donanim
  envanteri) eklendi
- VEC_VE_TARGET.md — erisim yollari tablosu, aarch64 dogrulandi
- START_HERE.md — 2026-06-09 update notu + okuma listesi yenilendi
- NEXT_SESSION_PROMPT.md — Faz 0.7 ilk adim
- context.yaml — target_hardware bolumu cihaz envanteri ile guncellendi

## Bir Sonraki Oturumda (MilCAM Acildiginda) Yapilacaklar

1. **`.ai/START_HERE.md`** oku
2. **`.ai/SESSION_LOG_2026-06-09.md`** (bu dosya) oku
3. **`.ai/DEVICE_PROFILE.md`** sonundaki tek-blok komutu serial console'a yapistir
4. Cihazdan gelen ciktiyi DEVICE_PROFILE.md'nin "GERCEK CIKTI" bolumune
   yapistir
5. WORKPLAN.md Faz 0.7 karar agacina gore Plan A/B/C sec:
   - RAM >= 2 GB → A (slim FreeCAD cihazda)
   - 1-2 GB → B (agresif slim + minimum Gui)
   - < 1 GB → C (CAD ayri workstation, panel sadece receiver)
6. Plan secimi dokumante et, sonra Faz 1 (build dogrulamasi) gec.

## Kritik Notlar (Unutma)

- **DO NOT build custom UI** — FreeCAD'in UI'si THE UI.
- **DO NOT patch FreeCAD source** — sadece CMake flag + Python overlay.
- **`Mod/MilCAM/` pure Python, `CodesysBridge/` plain C++ + stable ABI.**
- **MilCAM CADNC ile kod paylasmiyor.** CADNC ayri proje, ayri mimari.
- **VEC-VE RS232 pinout standart degil** — Pin 4=RX, 8=TX, 6=GND.
  Standart DB9 kablo ile baglanmaz, ozel kablo gerek (vendor ya da elle).
- **VM'den baglanti: `sudo screen /dev/ttyS0 115200,cs8,-ixon,-ixoff,-crtscts`**

## Linkler

- [README.md](../README.md)
- [CLAUDE.md](../CLAUDE.md)
- [CMakeLists.txt](../CMakeLists.txt)
- [milcam/Mod/MilCAM/InitGui.py](../milcam/Mod/MilCAM/InitGui.py)
- [milcam/CodesysBridge/](../milcam/CodesysBridge/)
- [doc/VEC-VE-Controller-Programming-Application-Manual.pdf](../doc/VEC-VE-Controller-Programming-Application-Manual.pdf)
- [doc/How to Run FreeCAD(2).docx](../doc/How%20to%20Run%20FreeCAD%282%29.docx)
