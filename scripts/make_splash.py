#!/usr/bin/env python3
# Generate the MilCAM boot splash as a raw framebuffer image for the VEC-VE panel.
#
# The device renders graphics through Qt linuxfb on /dev/fb0 (1024x768, 32bpp,
# BGRX, stride 4096). There is NO OpenGL/X on the panel, so the splash is just a
# pre-composed raw framebuffer blit — the boot script does `cat splash.raw >
# /dev/fb0`. No image library is needed on the device.
#
# Usage:
#   ./make_splash.py                       # SMB wordmark placeholder
#   ./make_splash.py --logo smb_logo.png   # center a real logo PNG on the bg
#   ./make_splash.py --bg 14,27,42 --out /tmp/splash.raw
#
# Then deploy:
#   ssh root@192.168.1.123 'cat > /etc/milcam-splash.raw' < splash.raw
#   ssh root@192.168.1.123 /etc/init.d/S95splash start
#
# Why a raw blit and not a PNG on the device: the panel's busybox userland has
# no image decoder we depend on; a raw BGRX dump is trivial and instant.

import argparse

from PIL import Image, ImageDraw, ImageFont

# Panel framebuffer geometry (confirmed live via /sys/class/graphics/fb0).
W, H = 1024, 768
STRIDE = 4096                 # bytes per line (W*4, but read from fb0 to be safe)

_FONT_CANDIDATES = [
    "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
]


def _font(size):
    for path in _FONT_CANDIDATES:
        try:
            return ImageFont.truetype(path, size)
        except OSError:
            continue
    return ImageFont.load_default()


def _parse_rgb(s):
    return tuple(int(x) for x in s.split(","))


def render(logo_path, bg):
    """Compose the 1024x768 splash. If logo_path is given, center it; otherwise
    draw the 'SMB / MilCAM CNC CAM' placeholder wordmark."""
    img = Image.new("RGB", (W, H), bg)
    if logo_path:
        logo = Image.open(logo_path).convert("RGBA")
        # Fit within 70% of the screen, preserve aspect.
        logo.thumbnail((int(W * 0.7), int(H * 0.7)), Image.LANCZOS)
        img.paste(logo, ((W - logo.width) // 2, (H - logo.height) // 2), logo)
        return img

    d = ImageDraw.Draw(img)
    accent, white, muted = (56, 132, 200), (235, 240, 245), (120, 140, 160)
    wm = _font(200)
    bb = d.textbbox((0, 0), "SMB", font=wm)
    tw, th = bb[2] - bb[0], bb[3] - bb[1]
    x, y = (W - tw) // 2 - bb[0], (H - th) // 2 - bb[1] - 40
    d.text((x, y), "SMB", font=wm, fill=white)
    lw, ly = int(tw * 0.9), y + th + 30
    d.rectangle([(W - lw) // 2, ly, (W - lw) // 2 + lw, ly + 6], fill=accent)
    sub = _font(40)
    sb = d.textbbox((0, 0), "MilCAM  ·  CNC CAM", font=sub)
    d.text(((W - (sb[2] - sb[0])) // 2 - sb[0], ly + 30),
           "MilCAM  ·  CNC CAM", font=sub, fill=muted)
    return img


def to_bgrx(img, stride):
    """Pack RGB image into the panel's BGRX framebuffer layout with line padding."""
    px = img.load()
    pad = b"\x00" * (stride - W * 4)
    out = bytearray()
    for yy in range(H):
        row = bytearray()
        for xx in range(W):
            r, g, b = px[xx, yy]
            row += bytes((b, g, r, 0))     # BGRX little-endian (rgba 8/16,8/8,8/0)
        out += row + pad
    return bytes(out)


def main():
    ap = argparse.ArgumentParser(description="Build MilCAM framebuffer splash")
    ap.add_argument("--logo", help="logo PNG to center (else SMB wordmark)")
    ap.add_argument("--bg", default="14,27,42", help="background R,G,B")
    ap.add_argument("--out", default="splash.raw", help="output raw path")
    ap.add_argument("--preview", help="also save a PNG preview here")
    args = ap.parse_args()

    img = render(args.logo, _parse_rgb(args.bg))
    if args.preview:
        img.save(args.preview)
    with open(args.out, "wb") as f:
        f.write(to_bgrx(img, STRIDE))
    print("wrote", args.out, "(%d bytes)" % (STRIDE * H))


if __name__ == "__main__":
    main()
