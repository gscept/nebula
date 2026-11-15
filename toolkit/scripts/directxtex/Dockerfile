# Build environment for portable texconv/texassemble: glibc 2.28 baseline
# (RHEL/Rocky/Alma 8+, Ubuntu 18.04+, Debian 10+, Fedora 29+)
# git: the image libraries are fetched by Texconv-Custom-DLL's CMake during the build
# nasm: SIMD extension of libjpeg-turbo
FROM almalinux:8
RUN dnf install -y dnf-plugins-core \
 && dnf config-manager --set-enabled powertools \
 && dnf install -y gcc-toolset-12 cmake ninja-build git nasm patch \
 && dnf clean all
ENV PATH=/opt/rh/gcc-toolset-12/root/usr/bin:$PATH
