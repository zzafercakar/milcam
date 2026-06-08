# Engineering Log

Karşılaşılan teknik sorunlar, root cause analizleri ve çözümler. Yeni en üstte.

---

## 2026-06-08 — Proje doğdu

**Olay:** CADNC'den carve-out ile MilCAM oluşturuldu.

**Kararlar:**
- Sketcher + PartDesign **build'e dahil değil** — CAM-only scope.
- Adapter'da SketchFacade/NestFacade dosyaları fiziksel olarak duruyor ama
  CMake glob filtresi ile build dışında.
- FreeCAD subset: Base, App, Material, Part.
- Top-level `MILCAM_*` CMake option'ları: `ASAN`, `BUILD_TESTS`,
  `ENABLE_FREECAD`, `ENABLE_OPCUA`.

**Açık konular:**
- `app/main.cpp` içinde QSDLApi shim kaldırıldı; Qt 6.5+ `QQuickWindow::setGraphicsApi` çağrısı şu an yok — gerekirse build hatasına bakıp geri ekle.
- `ui/qml/Main.qml` içinde Qt Quick Dialogs / FileDialog stub bir Item olarak
  bırakıldı. Gerçek import dialog Faz 2'de gelecek.

---

## (Boş) — Doldurulacak konular

İlk build denemesinde muhtemelen çıkacak hataların buraya geleceği yer:

- [ ] `find_package(Qt6 ... VirtualKeyboard)` host'ta `qt6-virtualkeyboard-dev`
      paketinin yüklü olmasını gerektirir. Test edilecek.
- [ ] FreeCAD'in `Mod/Material/App` ve `Mod/Part/App` build'leri Sketcher/PartDesign
      olmadan compile ediyor mu? `find_package(Materials)`/`find_package(Part)`
      iç bağımlılıklar açısından doğrulanacak.
- [ ] OCCT `DataExchange` component STEP/IGES importer'larını içeriyor mu?
      Faz 2'de derleme aşamasında ortaya çıkar.
- [ ] `wmctrl` host'ta yoksa CodesysBridge::raiseSelfWindow sessizce başarısız —
      hata mesajı ekle.

---

## Şablon — Yeni Sorun Eklerken

```markdown
## YYYY-MM-DD — Kısa başlık

**Olay:** Ne oldu? Hangi komut/build adımı?

**Belirtiler:** Hata mesajı, log satırı.

**Root cause:** Neden oldu?

**Çözüm:** Ne yaptım?

**Kalıcı önlem (varsa):** CI guard, CMake check, vs.
```
