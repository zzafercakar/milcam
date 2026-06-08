# CodesysBridge Protokolu

`milcam/CodesysBridge/` — MilCAM ↔ CodeSys 3.5 runtime arasi iletisim
katmani. Pure C++ core + CPython binding. FreeCAD'in MilCAM workbench'i
icinden `import CodesysBridge` ile cagrilir.

## Iki Kanal

### Kanal 1: G-code Drop Folder (gercek)

**Konum:** `/var/cnc/jobs/` (default, `Bridge.drop_folder` ile override).

**Yazma protokolu (atomic):**

1. MilCAM `dropGCode(jobId, gcode)` cagirir.
2. `<dropFolder>/<jobId>.gcode.tmp` dosyasina yazar.
3. POSIX `rename()` ile `.tmp` → `.gcode` (atomic).
4. PLC her zaman tam veya hic dosya gorur, yarim gormez.

**PLC tarafi (CodeSys ST):**

```iecst
VAR
    smcReadFile : SMC_ReadNCFile2;
    smcInterp   : SMC_NCInterpreter;
    sFile       : STRING(256);
    xLoadJob    : BOOL;
END_VAR

(* HMI butonu xLoadJob set eder *)
smcReadFile(
    bExecute  := xLoadJob,
    sFileName := sFile,
    ...
);
```

`sFile`, OPC UA `MilCAM.JobReadyPath` sembolunden okunur.

### Kanal 2: OPC UA (Faz 4'te gercek, su an stub)

**Endpoint:** `opc.tcp://192.168.1.123:4840` (varsayilan).

**Bridge.connect():** Faz 4'te open62541 client_connect. Su an no-op.

**Sembol sozlesmesi** (PLC tarafinda kurulacak):

| Sembol                       | Tip     | Yon          | Aciklama                              |
|------------------------------|---------|--------------|---------------------------------------|
| `MilCAM.JobReadyPath`        | STRING  | MilCAM → PLC | Yeni G-code dosya yolu                |
| `MilCAM.JobId`               | STRING  | MilCAM → PLC | Job kimligi (insan-okur)              |
| `MilCAM.RunRequest`          | BOOL    | MilCAM → PLC | True = simdi basla                   |
| `MilCAM.CancelRequest`       | BOOL    | MilCAM → PLC | True = durdur                         |
| `MilCAM.AppHeartbeat`        | UDINT   | MilCAM → PLC | Saniyede 1 artar (watchdog)           |
| `PLC.MachineState`           | UDINT   | PLC → MilCAM | 0=Idle 1=Running 2=Paused 3=Error 4=EStop |
| `PLC.CurrentLine`            | UDINT   | PLC → MilCAM | Calisan G-code satir no               |
| `PLC.CurrentBlock`           | STRING  | PLC → MilCAM | Aktif blok (debug)                    |
| `PLC.ErrorCode`              | UDINT   | PLC → MilCAM | 0=OK                                  |
| `PLC.ErrorMessage`           | STRING  | PLC → MilCAM | Insan-okur                            |
| `PLC.SpindleSpeed`           | REAL    | PLC → MilCAM | RPM gerçek                            |
| `PLC.FeedOverride`           | REAL    | PLC → MilCAM | 0.0-2.0                               |

### Watchdog

MilCAM saniyede `MilCAM.AppHeartbeat++` yazar. PLC 10 saniye gormezse
"MilCAM koptu" alarmi.

## API Yuzeyi (Python)

```python
import CodesysBridge

# Module-level
CodesysBridge.raise_window("TargetVisu")        # wmctrl raise

# Bridge instance — bir tane FreeCAD oturumunda
bridge = CodesysBridge.Bridge()
bridge.endpoint_url = "opc.tcp://192.168.1.123:4840"
bridge.drop_folder  = "/var/cnc/jobs"

# Drop folder (her zaman calisir)
path = bridge.drop_gcode("job_42", gcode_text)   # → "/var/cnc/jobs/job_42.gcode"
removed = bridge.prune_old_jobs(30)              # son 30 gunden eskileri sil

# OPC UA (Faz 4'te gercek)
ok = bridge.connect()                            # False (stub)
if bridge.is_connected():
    bridge.notify_job_ready(path)
state = bridge.machine_state()                   # int (STATE_* sabitleri)
line  = bridge.current_line()
bridge.disconnect()

# Sabit enum'lar
CodesysBridge.STATE_IDLE    # 1
CodesysBridge.STATE_RUNNING # 2
CodesysBridge.STATE_ESTOP   # 5
```

## ABI Disiplini

CodesysBridge **CPython stable ABI** ile yazildi (`#define Py_LIMITED_API
0x030A0000` *eklenmedi* — ama API kullanimi stable subset icinde). Bu
sayede:
- Bir kere derlenen `.so` Python 3.10+ ile calisir.
- FreeCAD versiyonlari arasindaki Python farklilarinda kirilmaz.
- Cross-build sirasinda Python version uyumu tek koşul.

## Window Switching

`raise_window(title_substring)` shell exec ile `wmctrl -a "<title>"`
cagirir. `wmctrl` cihazda yok ise `Bridge::raiseWindow` False doner ve
SwitchToHmi komutu Console'a warning yazar.

CodeSys tarafindan da symmetric: TargetVisu "Open CAM" butonu:
```iecst
SysProcessExecuteCommand2('wmctrl -a MilCAM', ADR(result));
```

`/etc/codesys*/CODESYSControl.cfg` icinde `[SysProcess] Command=AllowAll`
gerek.

## E-Stop Davranisi

PLC `PLC.EStop = TRUE` set ettiginde (Faz 4):
1. Subscription notification gelir
2. Python callback FreeCAD `MainWindow`'a QMessageBox.critical gosterir
3. Yeni Send-to-CodeSys submit'lerini engelle (komut IsActive false)
4. EStop reset edilince banner kalkar

## Test Stratejisi

- **Unit (pytest):**
  - `tests/python/test_codesys_bridge.py` — drop folder atomic, sembolik temizlik
  - `tests/python/test_codesys_post.py` — postprocessor argumanlari, header
- **Integration (Faz 4):**
  - Yerel `open62541` mock server ile subscription test
  - 10 sn watchdog timeout dogrulamasi
- **Saha (Faz 5):**
  - Gerçek CodeSys runtime, 24 saat sürekli
  - Ethernet kablo cikar/tak — reconnect logic
  - EStop fiziksel test
