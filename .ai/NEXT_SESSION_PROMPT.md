# MilCAM — Sonraki Session Prompt (2026-06-11)

> Bu dosyayı yeni session'ın ilk mesajı olarak yapıştır.

---

## YAPIŞTIRILACAK PROMPT

```
MilCAM projesine devam ediyoruz. Önce şunları oku, sonra göreve başla:
1. CLAUDE.md, .ai/START_HERE.md, .ai/context.yaml
2. .ai/ENGINEERING_LOG.md (en üstteki 2026-06-10/11 girişleri — mimari + switch)
3. .ai/TOOLCHAIN_NOTES.md (aarch64 cross-build reçetesi)
4. .ai/DEVICE_DEPLOY_PROCEDURE.md + .ai/CODESYS_RETURN_BUTTON.md
5. docs/superpowers/specs/2026-06-10-milcam-ui-v2-and-hmi-switch-design.md
6. docs/superpowers/plans/2026-06-10-milcam-ui-v2.md

GÖREV (tek odak): HMI⇄MilCAM geçişini cihazda TEMİZ çalışır hale getir.
Şu an: relaunch düzeldi, UI v2 güzel görünüyor; ama CNC'ye basınca CNC HMI
temiz gelmiyor (snapshot bozuk kareler yakalamıştı). Aşağıdaki "Sıradaki
adımlar"ı uygula. Sonra CAM özelliklerine geç (P1 DXF+canvas, P3 Profile).

Cihaz: ssh -i ~/.ssh/milcam_id root@192.168.1.123  (Türkçe konuş, kod İngilizce)
Build host sysroot/toolchain: ~/milcam-sysroot/ (Bootlin glibc-2.27, qt-desktop, vec-ve, aqtenv)
Cross-build: bash scripts/build-app-arm64.sh ; deploy: bash scripts/run-on-device.sh
GitHub: github.com/zzafercakar/milcam (main); push key ~/.ssh/id_ed25519_zzafercakar
```

---

## DURUM ÖZETİ (2026-06-11)

**MilCAM = standalone Qt 5.12 Widgets 2.5B CAM**, VEC-VE panelinde **linuxfb**'de
çalışır (GL yok). Cihazda canlı ve görünüyor.

### TAMAMLANAN
- **UI v2 (modern açık/mavi CAM kabuğu):** üst bar (New/Open/Save/Post + sağ-üst
  `CNC` butonu), sol ikon şeridi, grid canvas, JOB/PARAMETERS panelleri, alt
  durum çubuğu + SEND G-CODE. Cihazda fb0 capture ile doğrulandı. (Placeholder —
  gerçek CAM işlevi P1/P3'te.)
- **Toolchain:** Bootlin **glibc-2.27** aarch64 (gcc7.3) + desktop **Qt 5.12.4**
  header (aqt) + cihazın gerçek `.so.5`'leri, `-static-libstdc++`. `scripts/
  build-app-arm64.sh`. (Host gcc-13 glibc-2.39 → cihaz glibc-2.29'da çalışmaz.)
- **P2 CODESYS DIN 66025 post** (host TDD, 5+ test) + cihazda headless `ALL PASS`.
- **HMI⇄MilCAM switch = "launch/exit" modeli:**
  - CodeSYS "MilCAM" butonu → `setsid /root/milcam/run.sh &` (SysProcess). **Edge-
    triggered MINIMAL program PLC'ye İNDİRİLDİ → relaunch DURDU** (run.sh artık 1×).
  - MilCAM "CNC" butonu → açılışta yakaladığı fb0 karesini geri basar + `qApp->quit()`.
  - run.sh **idempotent** (çalışıyorsa atla) + **10sn cooldown** (CNC sonrası
    hemen yeniden açılmasın). İkisi de cihazda test edildi.
- `[SysProcess] Command=AllowAll` cfg'ye eklendi (yedek: `.bak-milcam`). Hepsi
  GitHub main'de (son: `5c32c7e`).

### DOĞRULANMIŞ MİMARİ GERÇEKLER (tekrar deneme!)
1. Tek `/dev/fb0`, tüm VT'ler paylaşır → **ekranda görünen = fb0'a en son yazan** (aktif VT değil).
2. **`chvt`/VT-switch ÖLÜ** (chvt 2 askıda kalır; CodeSYS tty1'i bırakmaz).
3. **CodeSYS kendiliğinden repaint YAPMAZ** (MilCAM çıkınca ekran donuk kalır).
4. **Cihaz MilCAM'i kendiliğinden başlatmaz** (relaunch hep CodeSYS'tendi → düzeldi).

### KALAN TEK SORUN
CNC'ye basınca **temiz CNC HMI gelmiyor**. MilCAM açılışta fb0'ı yakalayıp çıkışta
geri basıyor; ama önceki relaunch-kaosu döneminde **bozuk kareler** (beyaz/kısmi)
yakalanmıştı → geri basınca bozuk görünüyor. Relaunch düzeldiğine göre **temiz
bootstrap** ile düzelmeli.

### SIRADAKI ADIMLAR (öncelik sırası)
1. **Temiz bootstrap testi:** Cihazı **restart** et → boot'ta CodeSYS CNC HMI'si
   temiz çizilir. Sonra: CNC HMI görünürken → **MilCAM** butonu (MilCAM temiz CNC
   karesini yakalar) → **CNC** butonu → temiz CNC geri gelmeli ve **kalmalı**.
2. **Hâlâ bozuksa — known-good kareyi dosyaya sabitle:** CNC HMI tam görünürken
   bir kez `dd if=/dev/fb0 ... > /root/milcam/cnc_bg.raw` ile kaydet; MilCAM'i
   `CncBackdrop`'u **her zaman bu dosyadan** yükleyecek şekilde değiştir (launch-
   capture'ı sadece dosya yoksa kullan). Böylece asla bozuk kare yakalanmaz.
   İlgili kod: `milcam/app/src/main.cpp` (captureCncBackdrop), `gui/HmiSwitch.cpp`
   (returnToCnc), `gui/CncBackdrop.h`.
3. **Round-trip doğrula:** CodeSYS CNC → MilCAM → CNC, fb0 capture + foto.
4. **Sonra CAM özellikleri:** P1 (DXF import + canvas), P3 (Profile op) — kendi
   planları docs/superpowers/.

### CİHAZ DURUMU (oturum sonu)
- MilCAM çalışıyor (panel responsive). `[SysProcess]` aktif, minimal launch
  programı PLC'de. Touch: Weida `/dev/input/event1` (MilCAM açıkken MilCAM'de,
  kapalıyken CodeSYS'te). Dosyalar: `/root/milcam/{milcam,run.sh}`.
- Bootstrap için bir restart, temiz CNC HMI verir.
```
