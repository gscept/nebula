# compressonatorcli for Linux

`bin/linux/compressonatorcli` is the texture compressor the toolkit uses on Linux
(`textureconverter.cc`, Windows uses `bin/win32/texconv.exe`). It is a single executable
built from AMD Compressonator V4.6.0 by the scripts in this folder. Everything except glibc
is linked statically, it runs on any x86-64 Linux with glibc 2.28 or newer
(RHEL/Rocky/Alma 8+, Ubuntu 18.04+, Debian 10+, Fedora 29+).

`THIRD_PARTY_LICENSES.txt` holds the licenses of everything linked into the binary and has
to be distributed along with it.

## Rebuilding

```bash
toolkit/scripts/compressonator/build.sh --install
```

Needs `git` and `docker` (`DOCKER=podman` works too). The script clones the pinned
sources (verifying each commit) into `~/.cache/nebula-compressonatorcli` (override with
`WORK_DIR`), builds them in an AlmaLinux 8 container with GCC 12 (`Dockerfile`), updates
`THIRD_PARTY_LICENSES.txt` and with `--install` copies the result to `bin/linux/`.
Building the same sources again produces a byte-identical binary.

To check a build (needs Pillow, numpy and ImageMagick 7):

```bash
toolkit/scripts/compressonator/test_compressonatorcli.py bin/linux/compressonatorcli
```

## Contents

| Part | Version | Linked |
| --- | --- | --- |
| Compressonator CLI | V4.6.0 | static |
| OpenEXR / Imath (+ vendored libdeflate, OpenJPH) | 3.4.16 / 3.2.2 | static |
| libtiff | 4.7.2 | static |
| zlib | 1.3.2 | static |
| libstdc++ / libgcc | GCC 12 | static |
| glibc | >= 2.28 | system |

Not included: Qt (AMD's Linux CLI doesn't use it either), OpenCV (only needed for the
`-analysis` / PSNR / SSIM features), OpenGL, GPU encoders.

## Input formats

| Format | Loaded by | Precision |
| --- | --- | --- |
| DDS, TGA, KTX | Compressonator plugins | as stored |
| EXR | Compressonator plugin (OpenEXR) | half float |
| TIF, TIFF | TIFF plugin, patch 0005 (libtiff) | 8/16-bit integer, 16/32-bit float |
| PNG, JPEG, BMP, PSD, GIF, HDR, PNM | stb_image fallback | 8-bit RGBA |

TIFF support covers gray, gray+alpha, RGB and RGBA in 8/16-bit integer and 16/32-bit float,
stripped or tiled, contiguous or planar, uncompressed, LZW, Deflate or PackBits with any
predictor, and mirrored orientations. Palette, bilevel, YCbCr and CMYK images are loaded as
8-bit RGBA through libtiff. Alpha is passed on as stored (straight alpha stays straight),
only the first image of a multi-page file is used. JPEG, WebP, ZSTD, LZMA, LERC and JBIG
compressed TIFFs are rejected with an error.

Compressonator tone maps float images when compressing to LDR formats (BC1-BC5, BC7), use
BC6H for float sources. 8-bit sources keep giving exactly the same results as AMD's
prebuilt 4.5.52 CLI.

## sRGB

Compressonator has no sRGB variants of the BC formats and its DDS writer only produced
`*_UNORM` formats (BC1-3 even with legacy FourCC headers that can't express sRGB), so
sRGB textures lost their color space. Patch 0006 adds `-OutputSRGB`: BC1, BC2, BC3, BC7
and 8-bit RGBA are then written with a DX10 header and the `*_SRGB` format, the block
data is unchanged. `textureconverter.cc` passes it for sRGB textures.

Mip levels are still averaged in sRGB space, texconv (Windows) converts to linear
first, which keeps small mips slightly brighter.

## Patches

`patches/` holds the changes to Compressonator V4.6.0 as a `git format-patch` series,
applied in order by `build_compressonatorcli.sh`. They are written to be upstreamable:

| Patch | Change |
| --- | --- |
| 0001 | build the CLI on Linux without OpenGL and OpenCV (`OPTION_CMP_OPENGL/OPENCV` off) |
| 0002 | replace `-march=knl`, which GCC 15 removed |
| 0003 | OpenEXR 3 / Imath 3: include paths from pkg-config, use Imath's `half` in the EXR plugin, rename the clashing `printBits()` |
| 0004 | mip generation for 16-bit integer images (their mip levels were left uninitialized) |
| 0005 | libtiff based TIFF plugin (`OPTION_BUILD_TIFF`, default on Linux) |
| 0006 | `-OutputSRGB` / `CMP_CompressOptions::outputSRGB` for sRGB DDS formats |
| 0007 | missing `<cstdint>` include for GCC 13+ |
| 0008 | link the OpenEXR libraries pkg-config found by full path |

To change them, apply the series to a clone of Compressonator V4.6.0 with `git am`,
edit and commit, then regenerate with
`git format-patch --no-signature -o <this folder>/patches 095a143..HEAD`.

## Known limitations

- No GPU encoding, `-EncodeWith OCL/DXC/GPU` fall back to the CPU (same as AMD's Linux
  build). Windows' texconv uses DirectCompute for BC6H/BC7.
- Mip generation for half float images (EXR) averages the raw half bit patterns, which
  is inaccurate across exponent ranges. Unchanged from upstream.
- Uncompressed float output (`-fd ARGB_32F`) from 16-bit integer sources is broken
  upstream. Block compressed output is correct.
- BC6H from DDS files with 32-bit float pixels (`R32G32B32A32_FLOAT`, e.g. cubes assembled
  by texassemble from HDR faces) decodes to wrong colors, 16-bit float DDS and EXR sources
  are correct. The asset batcher converts these and `.hdr` files (8-bit through stb_image)
  to `R16G16B16A16_FLOAT` DDS with texconv first.
- Cube maps can only be block compressed ("Transcoding Cube Maps is not supported"), the
  asset batcher converts the uncompressed targets (RGBA8, R16) with texconv instead.
