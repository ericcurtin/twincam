# twincam

A lightweight camera application

# To build, install and run twincam

On fedora (steps should be similar on other platforms):

```
sudo dnf install -y git gcc g++ libevent libevent-devel openssl openssl-devel gnutls \
  gnutls-devel meson boost boost-devel python3-jinja2 python3-ply python3-yaml libdrm \
  libdrm-devel systemd-udev doxygen cmake graphviz
git clone https://github.com/kbingham/libcamera.git
cd libcamera
meson build --prefix=/usr && ninja -v -C build && sudo ninja -v -C build install
cd -
git clone https://github.com/ericcurtin/twincam
cd twincam
meson build --prefix=/usr && ninja -v -C build && sudo ninja -v -C build install
```

Then switch to a tty without a Desktop Environment running via Ctrl+Alt+F? (whichever
tty does not have DE running F1, F2, F3, etc.) and run:

```
twincam
```

