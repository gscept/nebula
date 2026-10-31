#!/usr/bin/env python3
"""
Checks the image loading of a compressonatorcli build, mainly the TIFF plugin.

Every TIFF variant is decoded by compressonatorcli into an uncompressed DDS and
compared with a reference decoded by Pillow/numpy. Needs Pillow, numpy and
ImageMagick 7 ('magick').

Usage: test_compressonatorcli.py <path to compressonatorcli>
"""
import os
import struct
import subprocess
import sys
import tempfile

import numpy as np
from PIL import Image

SIZE = 512


def make_image():
    """RGBA test image with gradients, noise and an alpha checkerboard"""
    y, x = np.mgrid[0:SIZE, 0:SIZE]
    rng = np.random.default_rng(1)
    r = 127 + 127 * np.sin(x * 0.02) * np.cos(y * 0.013)
    g = (x ^ y) & 0xFF
    b = np.clip(128 + 60 * np.sin((x + y) * 0.05) + rng.integers(-20, 21, x.shape), 0, 255)
    a = np.where(((x // 64 + y // 64) % 2) == 1, 255, 128)
    return np.stack([r, g, b, a], -1).astype(np.uint8)


def magick(src, out, *args):
    subprocess.run(['magick', src, *args, out], check=True, capture_output=True)


def run_cli(cli, src, out, *args):
    if os.path.exists(out):
        os.remove(out)
    r = subprocess.run([cli, '-noprogress', '-silent', *args, src, out], capture_output=True, text=True)
    return r.returncode == 0 and os.path.exists(out), (r.stdout + r.stderr).strip()


def decode_dds(path, mip_levels=1):
    """Decode an uncompressed ARGB_8888 or ARGB_32F DDS into float RGBA levels"""
    d = open(path, 'rb').read()
    fourcc = d[84:88]
    data, levels, w, off = d[128:], [], SIZE, 0
    for _ in range(mip_levels):
        if fourcc == b'\x00\x00\x00\x00':
            n = w * w * 4
            px = np.frombuffer(data[off:off + n], np.uint8).reshape(w, w, 4)
            levels.append(px[..., [2, 1, 0, 3]].astype(np.float64) / 255.0)
        elif fourcc == b't\x00\x00\x00':
            n = w * w * 16
            levels.append(np.frombuffer(data[off:off + n], np.float32).reshape(w, w, 4).astype(np.float64))
        else:
            raise RuntimeError('unexpected DDS format %r' % fourcc)
        off += n
        w //= 2
    return levels


def main():
    cli = os.path.abspath(sys.argv[1])
    tmp = tempfile.mkdtemp(prefix='cmptest_')
    os.chdir(tmp)

    base = make_image()
    Image.fromarray(base, 'RGBA').save('test.png')
    as_float = base.astype(np.float64) / 255.0
    opaque = as_float.copy()
    opaque[..., 3] = 1.0

    failed = 0

    def report(name, ok, detail):
        nonlocal failed
        failed += not ok
        print('%-20s %s  %s' % (name, 'ok  ' if ok else 'FAIL', detail))

    # name, magick options, output format, expected ('pil' = decode the TIFF with Pillow), tolerance
    cases = [
        ('rgba8_none', ['-compress', 'None'], 'ARGB_8888', as_float, 0),
        ('rgba8_lzw_predictor', ['-compress', 'LZW', '-define', 'tiff:predictor=2'], 'ARGB_8888', as_float, 0),
        ('rgba8_deflate', ['-compress', 'Zip'], 'ARGB_8888', as_float, 0),
        ('rgba8_packbits', ['-compress', 'RLE'], 'ARGB_8888', as_float, 0),
        ('rgba8_tiled', ['-compress', 'Zip', '-define', 'tiff:tile-geometry=128x64'], 'ARGB_8888', as_float, 0),
        ('rgba8_planar', ['-interlace', 'Plane', '-compress', 'LZW'], 'ARGB_8888', as_float, 0),
        ('rgba8_bottom_left', ['-flip', '-orient', 'bottom-left'], 'ARGB_8888', as_float, 0),
        ('rgb8', ['-alpha', 'off'], 'ARGB_8888', opaque, 0),
        ('gray8', ['-colorspace', 'Gray', '-alpha', 'off'], 'ARGB_8888', 'pil', 0),
        ('gray8_alpha', ['-colorspace', 'Gray'], 'ARGB_8888', 'pil', 0),
        ('palette', ['-alpha', 'off', '-colors', '200', '-type', 'Palette'], 'ARGB_8888', 'pil', 0),
        ('bilevel', ['-alpha', 'off', '-monochrome'], 'ARGB_8888', 'pil', 0),
        ('rgba32f', ['-define', 'quantum:format=floating-point', '-depth', '32'], 'ARGB_32F', as_float, 1e-6),
        ('rgba32f_predictor', ['-define', 'quantum:format=floating-point', '-depth', '32', '-compress', 'Zip', '-define', 'tiff:predictor=3'], 'ARGB_32F', as_float, 1e-6),
        ('rgba16f', ['-define', 'quantum:format=floating-point', '-depth', '16'], 'ARGB_32F', as_float, 1e-3),
    ]
    for name, options, fmt, expected, tolerance in cases:
        tif = name + '.tif'
        magick('test.png', tif, *options)
        ok, log = run_cli(cli, tif, tif + '.dds', '-nomipmap', '-fd', fmt)
        if not ok:
            report(name, False, 'compressonatorcli failed: ' + log[-200:])
            continue
        if isinstance(expected, str):
            expected = np.array(Image.open(tif).convert('RGBA')).astype(np.float64) / 255.0
        error = np.abs(decode_dds(tif + '.dds')[0] - expected).max()
        report(name, error <= tolerance + 1e-12, 'max error %.3g' % error)

    # unsupported compression has to fail cleanly
    magick('test.png', 'jpeg.tif', '-alpha', 'off', '-compress', 'JPEG')
    ok, log = run_cli(cli, 'jpeg.tif', 'jpeg.dds', '-nomipmap', '-fd', 'BC1')
    report('jpeg_rejected', not ok, log.splitlines()[-1] if log else '')

    # 16-bit integer: compressonatorcli can't write uncompressed output from 16-bit sources,
    # so check through BC7 (level 0 must match the 8-bit PNG exactly, each value is v * 257)
    # and through 8-bit mip levels, which have to stay within ~1 of an exact box filter
    # (truncation when converting to 8-bit, plus a little from the truncating 16-bit filter;
    # mips generated from 8-bit sources drift by up to ~4).
    for name, options in [('rgba16', ['-depth', '16']),
                          ('rgba16_msb_tiled', ['-depth', '16', '-endian', 'MSB', '-compress', 'Zip', '-define', 'tiff:predictor=2', '-define', 'tiff:tile-geometry=64x64'])]:
        tif = name + '.tif'
        magick('test.png', tif, *options)
        ok1, _ = run_cli(cli, tif, tif + '.bc7.dds', '-nomipmap', '-fd', 'BC7', '-Quality', '0.1')
        ok2, _ = run_cli(cli, 'test.png', 'png.bc7.dds', '-nomipmap', '-fd', 'BC7', '-Quality', '0.1')
        same = ok1 and ok2 and open(tif + '.bc7.dds', 'rb').read() == open('png.bc7.dds', 'rb').read()
        report(name + '_bc7', same, 'identical to PNG' if same else 'differs from PNG')

        levels = SIZE.bit_length()
        ok, log = run_cli(cli, tif, tif + '.mips.dds', '-miplevels', '20', '-fd', 'ARGB_8888')
        if not ok:
            report(name + '_mips', False, log[-200:])
            continue
        got = decode_dds(tif + '.mips.dds', levels)
        exact = base.astype(np.float64) * 257.0
        worst = 0.0
        for level in got:
            worst = max(worst, np.abs(level * 255.0 - exact / 257.0).max())
            exact = (exact[0::2, 0::2] + exact[1::2, 0::2] + exact[0::2, 1::2] + exact[1::2, 1::2]) / 4.0
        report(name + '_mips', worst <= 1.05, 'max error %.2f over %d levels' % (worst, levels))

    # -OutputSRGB: DX10 header with the _SRGB format, same block data as without it.
    # Formats without an sRGB variant are written unchanged.
    dxgi_names = {72: 'BC1_UNORM_SRGB', 75: 'BC2_UNORM_SRGB', 78: 'BC3_UNORM_SRGB', 99: 'BC7_UNORM_SRGB', 29: 'R8G8B8A8_UNORM_SRGB'}
    for fmt, expected in [('BC1', 72), ('BC2', 75), ('BC3', 78), ('BC7', 99), ('ARGB_8888', 29), ('BC4', None), ('BC5', None)]:
        ok1, _ = run_cli(cli, 'test.png', 'srgb.dds', '-miplevels', '20', '-fd', fmt, '-Quality', '0.1', '-OutputSRGB')
        ok2, _ = run_cli(cli, 'test.png', 'linear.dds', '-miplevels', '20', '-fd', fmt, '-Quality', '0.1')
        if not (ok1 and ok2):
            report('srgb_' + fmt, False, 'compressonatorcli failed')
            continue
        srgb, linear = open('srgb.dds', 'rb').read(), open('linear.dds', 'rb').read()
        if expected is None:
            report('srgb_' + fmt, srgb == linear, 'unchanged' if srgb == linear else 'changed')
            continue
        dx10 = srgb[84:88] == b'DX10'
        dxgi = struct.unpack('<I', srgb[128:132])[0] if dx10 else None
        data = srgb[148:]
        if fmt == 'ARGB_8888':
            # the legacy header stores B, G, R, A, R8G8B8A8 stores R, G, B, A
            px = np.frombuffer(linear[128:], np.uint8).reshape(-1, 4)[:, [2, 1, 0, 3]]
            same_data = data == px.tobytes()
        else:
            same_data = data == (linear[148:] if linear[84:88] == b'DX10' else linear[128:])
        ok = dx10 and dxgi == expected and same_data
        report('srgb_' + fmt, ok, '%s, data %s' % (dxgi_names.get(dxgi, dxgi), 'identical' if same_data else 'differs'))

    print('FAILED: %d' % failed if failed else 'all ok')
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
