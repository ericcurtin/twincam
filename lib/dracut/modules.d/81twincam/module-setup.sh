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
        inst_libdir_file "libevent*.so*"
        inst_libdir_file "libstdc++.so*"
}

