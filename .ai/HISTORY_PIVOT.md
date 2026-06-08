# Pivot Notesi — 2026-06-08

Bu belge MilCAM'in iki gun icinde geçirdigi mimari kayma bilgisini saklar.
Daha sonra "MilCAM neden boyle?" sorusu cikarsa burayi oku.

## Sabah — Yanlis Iskelet

İlk MilCAM scaffold CADNC'nin mimarisini taklit ediyordu:

```
ui/qml/Main.qml          ← Custom Qt6 QML interface (touch-first)
app/main.cpp             ← Custom main() with VirtualKeyboard, single instance
adapter/inc/...          ← CadDocument, CadEngine, CamFacade, PartFacade
cam/inc/...              ← 8 CAM operations from CADNC
viewport/                ← OCCT V3d_Viewer
util/, resources/        ← Helpers + icons
freecad/Base, App, ...   ← Vendored FreeCAD slim subset (App backend only)
```

**Hatali varsayim:** "MilCAM = lighter MilCAD" → custom app inşa et.

## Aksam — Kullanici Duzeltmesi

> "Şuanki kurguda MilCAM olabildiğince minimal değil mi? CAD değil CAM
> yapılacak. Bunun için de FreeCAD kaynak kodları ve **arayüzü birebir
> kullanılacak** sadece ağır diğer yükleri kullanılmayacak. Yani diğer
> modüller açılmayacak veya hiç olmayacak ama çok daha hafif olacak."
>
> "Dediğim gibi **yeni bir arayüz yapmıyoruz. FreeCAD'i hafifleterek
> kullanıyoruz** sadece."

Mesaj net: FreeCAD'in UI'si birebir + non-CAM modul'leri compile'da
disable + ince overlay = MilCAM.

## Pivot Aksiyonu

Silinen dizinler / dosyalar:
- `ui/` (whole)
- `app/` (whole)
- `viewport/` (whole)
- `util/` (whole)
- `cam/` (whole, CADNC kopyasi)
- `adapter/` (whole, CADNC kopyasi)
- `resources/` (whole, CADNC kopyasi)
- `freecad/` (vendorlanmis slim subset)
- `cmake/` (CADNC inherited)
- `tests/test_codesys_bridge.cpp` (custom Qt-based test)

Yeni dosyalar:
- `milcam/CodesysBridge/` — yeni mimari (C++ + CPython binding)
- `milcam/Mod/MilCAM/` — Python workbench overlay
- `CMakeLists.txt` — tum FreeCAD CMake'i wrap'leyen yeni versiyon
- `tests/python/` — pytest smoke
- Bu pivot belgesi

## Mimari Karsilasstirma

| Konu                     | Eski (silindi)               | Yeni (mevcut)                       |
|--------------------------|------------------------------|-------------------------------------|
| Uygulama                 | Custom Qt6 app               | FreeCAD birebir                     |
| UI                       | QML, custom touch-first      | FreeCAD'in kendi UI'si               |
| 3D viewport              | OCCT V3d_Viewer (custom)     | FreeCAD'in viewport'u                |
| CAM kod tabani          | CADNC'den kopya (cam/)        | FreeCAD'in CAM workbench'i           |
| File import              | Custom ImportFacade          | FreeCAD'in File→Open'i               |
| G-code post              | CADNC's CodesysPostProcessor C++ | codesys_post.py (Python)         |
| CodesysBridge            | Qt + QObject sinifi          | CPython extension (no Qt)            |
| Build sistemi            | Self-contained CMake         | Wraps FreeCAD'in CMake'i             |
| Disk footprint hedefi    | ~50 MB MilCAM binary         | < 1.5 GB FreeCAD install + overlay   |
| Operator deneyimi        | Buton+gpkur QML akisi        | FreeCAD UI, CAM workbench, "Send"    |
| Gelistirme efforti       | Bir yillik UI is             | Birkac haftalik overlay + cms        |

## Onemli Ders

Kullanici "MilCAM" dedigi anda, onun MilCAD'in tasarim hatasini tekrarlamak
istemedigi acikti — sifirdan UI yazarak yine ayni kavanozdan ikinci kabugu
yapmak yerine, FreeCAD'in olgun UI'sini almak. "Hafifleterek" kelimesi
"strip" demek, "yeniden yaz" degil.

Gelecek oturumlarda "MilCAM/MilCAD bir CAD/CAM yazalim" cumlesi gorulurse,
**once kullanici ile mimari teyit et**: custom-app mi, FreeCAD-overlay mi?
