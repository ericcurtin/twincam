# twincam

[![GitHub Build Status](https://github.com/inotify-tools/inotify-tools/workflows/build/badge.svg)](https://github.com/ericcurtin/twincam/actions)
[![Bugs](https://sonarcloud.io/api/project_badges/measure?project=ericcurtin_twincam&metric=bugs)](https://sonarcloud.io/summary/new_code?id=ericcurtin_twincam)
[![Code Smells](https://sonarcloud.io/api/project_badges/measure?project=ericcurtin_twincam&metric=code_smells)](https://sonarcloud.io/summary/new_code?id=ericcurtin_twincam)

A lightweight camera application, designed to start quickly in a bare environment

# To build, install and run twincam

On fedora (steps should be similar on other platforms):

```
sudo dnf install -y git gcc g++ libevent libevent-devel openssl openssl-devel \
  gnutls gnutls-devel meson boost boost-devel python3-jinja2 python3-ply \
  python3-yaml libdrm libdrm-devel systemd-udev doxygen cmake graphviz \
  libatomic texlive-latex
git clone https://git.libcamera.org/libcamera/libcamera.git
cd libcamera
meson build --prefix=/usr && ninja -v -C build && sudo ninja -v -C build install
cd -
git clone https://github.com/ericcurtin/twincam
cd twincam
meson build --prefix=/usr && ninja -v -C build && sudo ninja -v -C build install
```

Then switch to a tty without a Desktop Environment (Gnome, XFCE, etc.) running via
Ctrl+Alt+F? (whichever tty does not have DE running F1, F2, F3, etc.) and run:

```
twincam
```

