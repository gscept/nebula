# texconv / texassemble for Linux

`bin/linux/texconv` and `bin/linux/texassemble` are Linux builds of DirectXTex's texture
tools, matching the feature level of `bin/win32/texconv.exe` / `texassemble.exe`
(DirectXTex `may2026`, 2026.5.8.1). Everything except glibc is linked statically
(libjpeg-turbo, libpng, zlib, OpenEXR, libstdc++, libgcc, OpenMP), they run on any x86-64
Linux with glibc 2.28 or newer.

Upstream DirectXTex only builds the tools on Windows. The Linux ports come from
[Texconv-Custom-DLL](https://github.com/matyalatte/Texconv-Custom-DLL), whose
command-line front ends are synced up to DirectXTex October 2025. They are built against
the DirectXTex `may2026` library here, and `patches/` brings texassemble's front end up
to May 2026.

`THIRD_PARTY_LICENSES.txt` holds the licenses of everything linked into the binaries and
has to be distributed along with them.

## Rebuilding

```bash
toolkit/scripts/directxtex/build.sh --install
```

Needs `git` and `docker` (`DOCKER=podman` works too). The script clones the pinned
sources (verifying each commit) into `~/.cache/nebula-directxtex` (override with
`WORK_DIR`), builds them in an AlmaLinux 8 container with GCC 12 (`Dockerfile`), updates
`THIRD_PARTY_LICENSES.txt` and with `--install` copies the results to `bin/linux/`.
Texconv-Custom-DLL's CMake fetches libjpeg-turbo 3.1.3, libpng 1.6.54, zlib 1.3.1.2 and
OpenEXR 3.4.4 from GitHub during the build, so the container needs network access.

| Source | Version |
| --- | --- |
| Texconv-Custom-DLL | `40b28a0` (DirectX-Headers, DirectXMath, safestringlib at its submodule commits) |
| DirectXTex | `may2026` (`4feb3e1`), replaces Texconv-Custom-DLL's October 2025 branch |

## Patches

`patches/` holds changes to Texconv-Custom-DLL as a `git format-patch` series, applied in
order by `build_directxtex.sh`:

| Patch | Change |
| --- | --- |
| 0001 | port texassemble's `cube-from-mips`, `-m`/`--mip-levels`, the from-mips validation and `help`/`feedback` from DirectXTex May 2026 |
| 0002 | install libjpeg-turbo to `lib`, linking failed on lib64 distros (RHEL, Fedora, ...) |

Both are candidates for upstreaming to Texconv-Custom-DLL. To change them, apply the
series to a clone at `40b28a0` with `git am`, edit and commit, then regenerate with
`git format-patch --no-signature -o <this folder>/patches 40b28a0..HEAD`.

## Differences to the Windows tools

Verified against the Windows 2026.5.8.1 binaries (run under Wine with `-nogpu`): the
command lines are identical apart from the file types, texconv's BC1/BC3/BC7 output is
byte-identical and BC5 decodes to identical values, texassemble's `cube`, `from-mips`
and `cube-from-mips` produce identical pixels.

- File types come from the platform: Linux reads and writes DDS, TGA, HDR, PNG, JPEG and
  EXR. Windows uses WIC instead (BMP, PNG, JPEG, TIFF, GIF, JPEG XR, HEIF, ...) and has
  no EXR.
- PNG/JPEG are loaded as `R8G8B8A8_UNORM_SRGB` on Linux and `B8G8R8A8_UNORM` by WIC, so
  e.g. a cube assembled from PNGs has a different format. Pass `-srgbi`/`-srgbo`
  explicitly when converting to get the same result on both.
- No GPU codecs, BC6H/BC7 are compressed on the CPU (with OpenMP).
- No `texdiag` (Texconv-Custom-DLL doesn't port it).
