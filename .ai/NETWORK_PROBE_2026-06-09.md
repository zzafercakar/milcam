# Network Probe — 2026-06-09

İlk seri bağlantı sonrası host'tan cihazın network durumu tarandı.

## Sonuçlar

```
Host: 192.168.181.130/24  (geliştirme VM ethernet ens192)
Cihaz: 192.168.1.123       (default vendor IP)
Yol: VMware NAT → router → cihaz LAN
Ping: 192.168.1.123 = 3 ms RTT, %0 packet loss
```

| Port  | Servis           | Durum         | Anlamı                                  |
| ----- | ---------------- | ------------- | ----------------------------------------- |
| 22    | SSH (dropbear)   | ✅ **AÇIK 2026-06-09 17:18** | bizim kurduğumuz dropbear 2024.86 daemon |
| 23    | Telnet           | ❌ kapalı     | telnetd yok                               |
| 80    | HTTP             | ❌ kapalı     | web server yok                            |
| 8080  | CodeSys WebVisu  | ❌ kapalı     | uygulama VisuPage barındırmıyor           |
| 4840  | OPC UA           | ❌ kapalı     | CodeSys OPC UA server kapatılmış          |
| 11740 | CodeSys gateway  | ✅ **AÇIK**   | runtime yönetim portu — CODESYS bağlanıyor |

## Anlamı / Aksiyon

- **Tek interaktif shell yolu serial console.** Network üzerinden Linux'a
  doğrudan giriş YOK.
- **CodeSys gateway açık** → kontrolcü programlama mümkün (CODESYS Ethernet
  üzerinden bağlanmış durumda)
- **WebVisu kapalı** → kullanıcı kararı, açmak istersen Application'da
  Visualization → Visualization Manager → Add WebVisu
- **OPC UA kapalı** → MilCAM Faz 3 için açılması gerekecek. CODESYS'te
  Device → Communication → OPC UA Server → enable

## 2026-06-09 17:18 Update — SSH KURULDU

Dropbear 2024.86 aarch64 statik binary cihaza cross-build edildi (host glibc
2.39 ≠ cihaz glibc 2.29 mismatch nedeniyle dinamik link segfault verdi,
statik link cozdu). Detaylar: [DROPBEAR_INSTALL_2026-06-09.md](DROPBEAR_INSTALL_2026-06-09.md).

Komut:
```bash
ssh -i ~/.ssh/milcam_id root@192.168.1.123
```

Public key + ed25519 + host key initial bootstrap ile sifirdan calisir.
Init.d script (`/etc/init.d/S60dropbear`) boot'ta otomatik dropbear baslatir.

## Gelecek

USB Realtek Ethernet baglantisi cihazi host subnet'ine yaklasstirabilir,
bu durumda host'a giden geri-route da kurulabilir. WORKPLAN Faz 0.8'in
ikinci asamasi.
