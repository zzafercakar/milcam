# Gelecek Oturum İçin İlk Mesaj

Bu dosya, MilCAM üzerinde yeni bir Claude Code oturumuna başlarken kullanılacak
yönlendirme şablonudur.

---

## Önerilen ilk komut

```
Read .ai/START_HERE.md
Read .ai/context.yaml
Read .ai/WORKPLAN.md
Read .ai/ENGINEERING_LOG.md
Read .ai/VEC_VE_TARGET.md
```

## Sonraki Beklenen Adım (2026-06-08 itibarıyla)

**Faz 0.5 — VEC-VE Hardware Reality Check.** Cihaza SSH ile bağlanıp şu
komutları çalıştır, çıktıyı `ENGINEERING_LOG.md` altına bir log entry olarak
yapıştır:

```bash
uname -a
cat /proc/cpuinfo | grep -E "model name|Hardware|Features" | sort -u
free -h
df -h / /root
glxinfo 2>/dev/null | grep -E "OpenGL (version|renderer)"
ps auxf | head -40
ls /opt/codesys/ /etc/codesys* 2>/dev/null
which wmctrl xdotool
```

Çıktıya göre Plan A / B / C arasında karar ver:
- RAM >= 2 GB + OpenGL 2.0+ → Plan A (MilCAM cihazda)
- RAM 1-2 GB → Slim profili, riskler değerlendir
- RAM < 1 GB veya GL yok → Plan C (MilCAM workstation'da, cihaz drop receiver)

## Eğer kullanıcı "build et" derse

Henüz **end-to-end build doğrulanmadı**. İlk derlemede çıkması muhtemel
problemler (ENGINEERING_LOG.md "Doldurulacak konular" bölümünden):
- `Qt6 VirtualKeyboard` paketi eksik olabilir.
- FreeCAD Material/Part'ın Sketcher olmadan derleneceği doğrulanmadı.
- OCCT DataExchange component'ı eksik olabilir.

Bunları sırayla çöz — root cause ara, sembolik link veya hack yapma.

## Eğer kullanıcı "ImportFacade'i wire et" derse

Şu sırayla:
1. `freecad/Mod/Part/App/Import.h` ve `ImportStep.cpp` API'sini incele.
2. `adapter/src/ImportFacade.cpp` stub'unu STEP path'i için doldur.
3. `CamGeometrySource`'a TopoShape geç.
4. `tests/test_import_step.cpp` yaz — küçük bir STEP dosyası test'i.
5. UI'da `Q_EMIT importFinished` sinyalini dinleyen QML kısmı ekle.

## Eğer kullanıcı "OPC UA bağla" derse

Bu Faz 3. Önce Faz 0.5 ve Faz 2 tamamlanmış olmalı. Hangi sembollerin
PLC tarafında açıldığını **kullanıcıdan yazılı al** — sözlü onaylama
sembol isim değişikliklerini yakalamaz.

## Kritik Bilgi

- Kullanıcı: Türkçe konuş.
- Kod ve yorumlar: İngilizce.
- CAM modülünde değişiklik yaparsan → CADNC'ye de portla.
- `freecad/` altında değişiklik yapma — sadece okuma.
- Donanım denenmemişken büyük tahminler yapma.
