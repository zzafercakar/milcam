# Dropbear SSH — Cross-build + Deploy Notes (2026-06-09)

VEC-VE'ye SSH erişim açma süreci. **Faz 0.8'in birinci alt-adımı tamamlandı.**
Bu belge cross-build, transfer ve install adımlarının her birinin tuzaklarını
ve nihai çalışan reçetesini saklıyor.

## Sonuç

```bash
ssh -i ~/.ssh/milcam_id root@192.168.1.123
# →  full shell, key-only auth, glibc 2.29 uyumlu static binary
```

- Cihazda **dropbear 2024.86** aarch64 statik binary
- `/etc/init.d/S60dropbear` boot'ta otomatik daemon başlatır + ilk açılışta
  host key generate eder
- Auth modeli: **public key only** (parola disabled — host glibc'de
  `crypt()` yok zaten, configure auto-detect etti)
- Host key türleri: RSA 2048 + ED25519
- Şu an çalışan daemon: PID 2997, `0.0.0.0:22` LISTEN

## Cross-build Reçetesi (calismayinca tekrar bakacak version)

### Toolchain (host = Ubuntu 24.04)

```bash
# Zaten yuklu:
aarch64-linux-gnu-gcc 13.3.0
make, wget, curl, tar, bzip2
```

### Source

```bash
cd /tmp
curl -sL -o dropbear-2024.86.tar.bz2 \
    https://matt.ucc.asn.au/dropbear/releases/dropbear-2024.86.tar.bz2
tar xjf dropbear-2024.86.tar.bz2
mkdir -p dropbear-build && cd dropbear-build
```

### Configure (TUZAK var, dikkat)

```bash
../dropbear-2024.86/configure \
    --host=aarch64-linux-gnu \
    --disable-zlib \
    --disable-pam \
    --enable-bundled-libtom \
    --disable-utmp --disable-utmpx \
    --disable-wtmp --disable-wtmpx \
    CC=aarch64-linux-gnu-gcc \
    CFLAGS="-Os -no-pie -fno-pie -static" \
    LDFLAGS="-static -no-pie -static-libgcc"
```

**Configure uyarisi:** "crypt() not available, dropbear server will not have
password authentication" — BU IYI, biz key-only istiyoruz.

### localoptions.h (zorunlu)

`configure` sonrasi build hatasiyla karsilasmamak icin `localoptions.h`
dosyasini build dizininde yaratmak zorunlu (configure DROPBEAR_SVR_PASSWORD_AUTH'i
ON birakir, password auth gerektirir, crypt() olmadigi icin compile hatasi):

```c
/* MilCAM deployment — password auth disabled */
#define DROPBEAR_SVR_PASSWORD_AUTH 0
#define DROPBEAR_SVR_PUBKEY_AUTH 1
#define DROPBEAR_CLI_PASSWORD_AUTH 1
#define DROPBEAR_CLI_PUBKEY_AUTH 1

/* Slim binary */
#define DROPBEAR_3DES 0
#define DROPBEAR_BLOWFISH 0
#define DROPBEAR_TWOFISH 0

/* Anahtar tipleri */
#define DROPBEAR_ECDSA 1
#define DROPBEAR_ED25519 1
#define DROPBEAR_RSA 1

#define DO_HOST_LOOKUP 0
```

### Build

```bash
make -j$(nproc) PROGRAMS="dropbear dropbearkey" \
     LDFLAGS="-static -no-pie -static-libgcc"
aarch64-linux-gnu-strip dropbear dropbearkey
```

### Sonuc

```
dropbear:    1 123 640 byte, ELF aarch64 statically linked, NOT pie
dropbearkey:   991 384 byte, ELF aarch64 statically linked, NOT pie
```

**KRITIK DOGRULAMA:**
```
file dropbear
→ "statically linked"  ✓ (eger "dynamically linked" goruyorsan TEKRAR yap)
→ NOT "pie executable"  ✓ (eger "pie" goruyorsan -no-pie eksik)
```

## Cihaza Transfer

Cihaz network'i (192.168.1.0/24) host'tan goruluyor ama host'a (192.168.181.0/24)
**ters yon route yok** (VMware NAT). Bu yuzden initial transfer **serial console**
uzerinden:

### Paketle

```bash
cd /tmp/dropbear-deploy   # dropbear + dropbearkey + S60dropbear
tar czf /tmp/dropbear-bundle.tar.gz dropbear dropbearkey S60dropbear
base64 -w 76 /tmp/dropbear-bundle.tar.gz > /tmp/dropbear-bundle.b64
# ~1 MB tar.gz → ~1.3 MB base64
```

### Stream (yakl. 2 dakika @ 115200 baud)

Cihazda alici hazirla:
```sh
cd /tmp && head -c <SIZE> > /tmp/bundle.b64
```

Host'tan portu doldur:
```bash
cat /tmp/dropbear-bundle.b64 > /dev/ttyS0
```

`head -c` tam BOYUT byte aldiktan sonra otomatik biter, cihaz prompt'a doner.

### Cihazda decode + extract (busybox tar TUZAK!)

```sh
# Busybox tar gzip flag ('-z') DESTEKLEMEZ — gunzip + tar zincirleme:
cd /tmp
base64 -d bundle.b64 > bundle.tar.gz
md5sum bundle.tar.gz                     # host md5 ile esit olmali
gunzip -c bundle.tar.gz | tar x          # tar xzf calismaz
```

### Install

```sh
mv -f /tmp/dropbear /tmp/dropbearkey /usr/sbin/
chmod +x /usr/sbin/dropbear /usr/sbin/dropbearkey
mv -f /tmp/S60dropbear /etc/init.d/
chmod +x /etc/init.d/S60dropbear

mkdir -p /etc/dropbear
/usr/sbin/dropbearkey -t rsa     -f /etc/dropbear/dropbear_rsa_host_key
/usr/sbin/dropbearkey -t ed25519 -f /etc/dropbear/dropbear_ed25519_host_key

/usr/sbin/dropbear -p 22 -E
```

## SSH Key Auth Setup

Host'ta:
```bash
ssh-keygen -t ed25519 -f ~/.ssh/milcam_id -N '' -C "milcam@$(hostname)"
```

Cihaza public key yolla (serial veya scp ile):
```sh
# Cihazda:
mkdir -p /root/.ssh && chmod 700 /root/.ssh
echo 'ssh-ed25519 AAAA... milcam@td1' > /root/.ssh/authorized_keys
chmod 600 /root/.ssh/authorized_keys
```

Host'tan test:
```bash
ssh -i ~/.ssh/milcam_id root@192.168.1.123 'echo OK; uname -a'
```

## Init.d Script

`/etc/init.d/S60dropbear` (boot'ta otomatik calisir, host keys eksikse uretir):

```sh
#!/bin/sh
DBKEYDIR=/etc/dropbear
mkdir -p $DBKEYDIR
[ -f $DBKEYDIR/dropbear_rsa_host_key     ] || /usr/sbin/dropbearkey -t rsa     -f $DBKEYDIR/dropbear_rsa_host_key
[ -f $DBKEYDIR/dropbear_ed25519_host_key ] || /usr/sbin/dropbearkey -t ed25519 -f $DBKEYDIR/dropbear_ed25519_host_key

case "$1" in
    start)   /usr/sbin/dropbear -p 22 ;;
    stop)    killall dropbear ;;
    restart) $0 stop; sleep 1; $0 start ;;
esac
```

## Bilinen Tuzaklar

| Belirti | Sebep | Cozum |
|---|---|---|
| Compile error `crypt()` | password auth ON, crypt() yok | `localoptions.h`'ye `DROPBEAR_SVR_PASSWORD_AUTH 0` |
| `file dropbear` `dynamically linked` | -static flag yanlislikla override edildi | `LDFLAGS="-static ..."` make sırasında da geçir |
| Cihazda `Segmentation fault` | host glibc 2.39 vs cihaz glibc 2.29, dynamic link | Statik build zorunlu |
| `tar: invalid option -- 'z'` | busybox tar gzip flag desteklemez | `gunzip -c file.tar.gz \| tar x` |
| `Failed loading dropbear_ecdsa_host_key` | sadece RSA+ED25519 ureттim | Sorun degil, dropbear devam eder |
| `ssh: Permission denied` | authorized_keys izinleri | `chmod 700 ~/.ssh && chmod 600 ~/.ssh/authorized_keys` |
| `head -c` cihaz tarafinda stuck | base64 dosya boyutu yanlis hesaplandi | host'taki `wc -c` ile tam degeri al |

## Gelecek Iyilestirmeler

- [ ] **Realtek USB Ethernet uzerinden** transfer: cihazin r8152 dongle'i
      taktiginda host network'une yakin bir IP alabilir, scp dogrudan
      calisabilir. Bu durum NETWORK_PROBE'da ayri belge.
- [ ] CodeSys runtime restart sirasinda dropbear etkilenmemeli (ayri pid).
      Test: `pidof dropbear` CodeSys restart oncesi/sonrasi ayni mi.
- [ ] **Drop client ABI tester yaz**: glibc surum mismatch'i debug etmek
      icin kucuk bir "hello world" cross-build ile statik link
      dogrulamasi.
