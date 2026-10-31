# Build environment for a portable compressonatorcli: glibc 2.28 baseline
# (RHEL/Rocky/Alma 8+, Ubuntu 18.04+, Debian 10+, Fedora 29+)
# libglvnd-devel only provides KHR/khrplatform.h for the KTX plugin, nothing links GL
FROM almalinux:8
RUN dnf install -y dnf-plugins-core \
 && dnf config-manager --set-enabled powertools \
 && dnf install -y gcc-toolset-12 cmake ninja-build python3 file patch libglvnd-devel \
 && dnf clean all
ENV PATH=/opt/rh/gcc-toolset-12/root/usr/bin:$PATH
