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
| 22    | SSH              | ❌ kapalı     | dropbear/sshd başlatılmamış               |
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

## Sonraki Adım

Cihazın `which dropbear` veya `ls /etc/init.d/` ile SSH daemon var mı
kontrol etmek gerek. Varsa elle başlatıp serial console'u serbest
bırakabiliriz. Yoksa BusyBox httpd üzerinden basit bir komut-execute
proxy kurulabilir veya CodeSys SysProcess ile uzaktan komut çalıştırılabilir
(bkz. WORKPLAN Faz 1.1).
