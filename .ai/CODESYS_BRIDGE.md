# CodeSys Bridge Protocol

MilCAM ↔ CodeSys 3.5 runtime iletişim sözleşmesi. İki kanal: drop folder ve
OPC UA. Bu dosya kanalların **kontrat seviyesinde** ne yaptığını anlatır.
İmplementasyon: `adapter/src/CodesysBridge.cpp`.

## Kanal 1: G-code Drop Folder

### Yer
`/var/cnc/jobs/`  (cihazda; konfigürablle `CodesysBridge::setDropFolder`).

### Atomic Write Protokolü
1. MilCAM hedef adi `<jobId>.gcode` belirler (örn. `job_42.gcode`).
2. Geçici dosyaya yazar: `<jobId>.gcode.tmp`.
3. `QFile::rename` ile atomic taşır (POSIX `rename` garantili atomicity).
4. Tüketici (PLC) her zaman tam dosya görür veya hiç görmez — yarı yazılmış
   dosya yok.

### PLC tarafı
```iecst
VAR
    smcReadFile : SMC_ReadNCFile2;
    smcInterp   : SMC_NCInterpreter;
    sFile       : STRING := '/var/cnc/jobs/job_42.gcode';
END_VAR

smcReadFile(
    bExecute := xLoadJob,
    sFileName := sFile,
    pStartPosition := pPos,
);
```

### Temizlik
- MilCAM eski jobs'u temizler (`/var/cnc/jobs/` içinde > 30 günlükleri).
- Faz 5'te disk dolma senaryosu — `CodesysBridge::pruneOldJobs(int days)` ekle.

### Sınırlamalar
- Drop folder PLC ve MilCAM'in ortak görebileceği bir mount olmalı.
- NFS mount yapılırsa async write nedeniyle atomic garanti zayıflar; `sync`
  veya local FS tercih edilmeli.

---

## Kanal 2: OPC UA

### Endpoint
`opc.tcp://192.168.1.123:4840` (varsayılan; konfigüre edilebilir).

### Security
- Faz 3 ilk surum: **Anonymous + None** (test için).
- Faz 5: Username/password + Basic256Sha256 zorunlu.

### Symbol Tablosu

PLC tarafında `Application.GVL_MilCAM` veya benzer bir GVL açılmalı.

| Symbol                       | Type    | Read/Write | Niçin                                |
| ---------------------------- | ------- | ---------- | ------------------------------------ |
| `MilCAM.JobReadyPath`        | STRING  | MilCAM → PLC | Yeni G-code dosya yolu             |
| `MilCAM.JobId`               | STRING  | MilCAM → PLC | İnsan-okur job kimliği             |
| `MilCAM.RunRequest`          | BOOL    | MilCAM → PLC | True = "şimdi başlat"              |
| `MilCAM.CancelRequest`       | BOOL    | MilCAM → PLC | True = "durdur"                    |
| `MilCAM.AppHeartbeat`        | UDINT   | MilCAM → PLC | her saniye artar (watchdog)        |
| `PLC.MachineState`           | UDINT   | PLC → MilCAM | enum: 0 Idle, 1 Running, 2 Paused, 3 Error, 4 EStop |
| `PLC.CurrentLine`            | UDINT   | PLC → MilCAM | NC interpretörünün şu anki satırı   |
| `PLC.CurrentBlock`           | STRING  | PLC → MilCAM | Aktif G-code bloğu (debug)         |
| `PLC.ErrorCode`              | UDINT   | PLC → MilCAM | 0 = OK                              |
| `PLC.ErrorMessage`           | STRING  | PLC → MilCAM | İnsan-okur                          |
| `PLC.SpindleSpeed`           | REAL    | PLC → MilCAM | RPM, gerçek                        |
| `PLC.FeedOverride`           | REAL    | PLC → MilCAM | 0.0 - 2.0 (operator override)      |

### MilCAM → PLC: Job Submit Sırası

```
1. dropGCode(jobId, gcode)         → /var/cnc/jobs/<jobId>.gcode
2. setSymbol(MilCAM.JobReadyPath, path)
3. setSymbol(MilCAM.JobId, jobId)
4. setSymbol(MilCAM.RunRequest, TRUE)   ; PLC bunu görüp set FALSE eder
```

### MilCAM ← PLC: State Subscription

```
1. Connect → CreateSubscription(publishingInterval=200ms)
2. MonitorItem: PLC.MachineState, PLC.CurrentLine, PLC.EStop
3. Notification gelince Qt signal emit:
   - machineStateChanged
   - currentLineChanged
   - emergencyStopActivated (special path)
```

### Heartbeat / Watchdog

MilCAM her saniye `MilCAM.AppHeartbeat++` yazar.
PLC bunu görmezse 10 saniye sonra "MilCAM bağlantısı koptu" alarmı verir.

### Bağlantı Kopma Senaryosu

- MilCAM connect kaybederse: UI'da kırmızı banner, "PLC: disconnected".
- 5 saniyede bir reconnect denenir.
- Drop folder yazımı (`dropGCode`) bağımsız çalışır — bağlantı yokken bile
  dosya yazılır, sadece `notifyJobReady` no-op olur.

---

## Window Switching (HMI ↔ CAM)

Aslında bu da CodesysBridge'in sorumluluğu (CodeSys'le iletişim olduğu için).

### CAM → HMI yön
MilCAM `[HMI]` butonu:
```cpp
QProcess::execute("wmctrl", { "-a", "TargetVisu" });
```

### HMI → CAM yön
TargetVisu içinde "Open CAM" butonu — IEC ST kodu:
```iecst
SysProcessExecuteCommand2('wmctrl -a MilCAM', ADR(result));
```

Bunun için CodeSys cihazında:
```ini
; /etc/codesys*/CODESYSControl.cfg
[SysProcess]
Command=AllowAll
```

## E-Stop Davranışı

PLC EStop'a basıldığında:
1. `PLC.EStop` = TRUE
2. MilCAM subscription notification alır
3. UI tam ekran kırmızı banner: "EMERGENCY STOP — Reset on machine"
4. Yeni job submit edilmesi engellenir (MilCAM-side guard)
5. EStop reset edilince banner kaybolur

---

## Test Stratejisi

### Birim
- `tests/test_codesys_bridge.cpp` — dropGCode atomic rename, klasör yokken
  oluşturma, hatalı izin durumu.

### Entegrasyon (Faz 3)
- Yerel `open62541` server ile mock PLC: subscription, write, notification.
- Heartbeat watchdog: 10 saniye yazma kes, mock PLC alarm bekle.

### Saha (Faz 5)
- Gerçek CodeSys runtime, 24 saat sürekli.
- EStop trigger, recover.
- Bağlantı kopma (ethernet kablo çıkar/tak).
