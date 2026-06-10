# MilCAM — Sonraki Session Başlangıç Prompt'u

> Bu dosyayı yeni session'ın ilk mesajı olarak kullan (veya Claude'a "bunu oku
> ve başla" de). Bu plan 2026-06-10'da yazıldı.

---

## Yapıştırılacak prompt

```
MilCAM projesine devam ediyoruz. Önce şunları oku, sonra plana göre başla:

1. CLAUDE.md
2. .ai/START_HERE.md, .ai/context.yaml
3. .ai/DISPLAY_ARCHITECTURE_2026-06-09.md  ← cihaz gerçeği (X yok, GL yok,
   linuxfb; FreeCAD 3B çalışmaz → standalone Qt 2.5B CAM kararı)
4. docs/superpowers/specs/2026-06-10-milcam-standalone-cam-design.md  ← tasarım
5. docs/superpowers/plans/2026-06-10-milcam-v1-p0-p2.md  ← UYGULAMA PLANI
6. CODESYS_CNC_CAM_Arastirma_Raporu.txt (kullanıcıda) ← post spec'i

Planı superpowers:subagent-driven-development (önerilen) veya
superpowers:executing-plans ile görev-görev uygula. Checkbox'ları işaretle.

BAŞLANGIÇ ÖNERİSİ: Phase 2 (CODESYS post) ile başla — saf C++17 core,
CİHAZA İHTİYAÇ YOK, host'ta TDD ile yazılır, en yüksek değerli/risksiz parça.
Görev 2.1'den başla. Phase 0 (toolchain) paralel/sonraki; en büyük risk orada
(glibc 2.29 uyumlu aarch64 Qt5.12 cross-build — önce SMB-Q6R'ı mayınla).

Cihaz erişimi (gerektiğinde): ssh -i ~/.ssh/milcam_id root@192.168.1.123
Kullanıcı ile Türkçe konuş; kod/yorum İngilizce.
```

---

## Hızlı bağlam (Claude için özet)

**Karar:** MilCAM = standalone Qt **Widgets** (GL yok → QML değil) 2.5B CAM,
cihazda **linuxfb**'de çalışır, **CODESYS DIN 66025** G-code üretir. FreeCAD
bu donanımda 3B render edemiyor (GLX/EGL/GL yok), terk edildi.

**v1 kapsamı:** DXF + dahili şekiller; Profile + Pocket + Drill; `.cnc` →
drop folder. OPC UA ve PLC receiver sonraya.

**Plan iki yürütülebilir parça + yol haritası:**
- **P2 (post)** — host-only, TDD, görev 2.1–2.6 tam kodlu + golden örnekler.
  **Buradan başla.** Cihaz/toolchain gerekmez.
- **P0 (toolchain + iskelet)** — runbook; SMB-Q6R cross-build'ini incele
  (`/home/embed/Dev/QT6/TeachPendant/SMB-Q6R`, Qt5.12 + open62541, deploy
  script'leri). glibc 2.29 uyumu = en büyük risk (0.1 karar kapısı).
- **P1/P3/P4/P5** — yol haritası; her biri sırası gelince kendi planını alır.

**Tekrar kullanılacaklar:**
- OPC UA: `SMB-Q6R/src/plc_link.{h,cpp}` (open62541, Qt5.12 — uyumlu).
- `milcam/CodesysBridge/` (saf C++ drop folder + OPC UA) repoda duruyor.

**Cihaz durumu (kalıcı):**
- Xorg devre dışı (`/etc/init.d/_S40xorg`); linuxfb kullanılıyor.
- Boot splash: SMB Technics logosu (`/etc/init.d/S95splash` + `/etc/milcam-splash.raw`).
  Kaynak: `scripts/make_splash.py` + `scripts/smb_logo.png`.
- Yedekler cihazda: `/root/usr-stock-backup.tgz`, `/root/dropbear[key]`.
- FreeCAD test kalıntısı `/root/FreeCAD/` (~3.6 GB) — silinebilir.
- IP 192.168.1.123 (eth1), CODESYS gateway 11740. Windows'tan bağlanırken
  eth1 portu + 192.168.1.x subnet şart.

**fb0 görsel doğrulama hilesi:** linuxfb'de `/dev/fb0` = ekran. Yakala:
`ssh ... 'dd if=/dev/fb0 bs=4096 count=768' > fb.raw` →
PIL `Image.frombytes('RGB',(1024,768),data,'raw','BGRX',4096)`. (Xorg açıkken
DRM kullandığından bu çalışmaz — şu an Xorg KAPALI, çalışır.)

**GitHub:** github.com/zzafercakar/milcam (main). Push: SSH key
`~/.ssh/id_ed25519_zzafercakar`, remote `git@github.com:zzafercakar/milcam.git`.
