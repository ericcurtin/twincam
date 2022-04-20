#!/usr/bin/bash

# called by dracut
check() {
    require_binaries twincam
}

# called by dracut
depends() {
    echo drm
}

# Install kernel module(s).
installkernel() {
    instmods '=drivers/media'
}

# called by dracut
install() {
    inst /usr/bin/twincam
    # install libs
    inst_libdir_file "libcamera*.so*"
    inst_libdir_file "libcamera/ipa_*.so*"
    inst_libdir_file "libevent*.so*"
    inst_libdir_file "libstdc++.so*"

    # Required if using Fedora SDL2
    inst_libdir_file "libSDL2*.so*"
    inst_libdir_file "libgbm*.so*"
    inst_libdir_file "dri/*.so*"
    inst_libdir_file "libOpenGL*.so*"
    inst_libdir_file "libEGL*.so*"
    inst_libdir_file "egl_vendor.d/*.json"

    inst_dir "/usr/share/glvnd/egl_vendor.d"
    inst_multiple "/usr/share/glvnd/egl_vendor.d/*.json"
}

