# Engineering Log

Karsilasilan teknik sorunlar, root cause, cozumler. Yeni en uste.

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
