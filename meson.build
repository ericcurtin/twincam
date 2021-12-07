project('twincam', 'c', 'cpp',
    meson_version : '>= 0.50',
    default_options : [
        'werror=true',
        'warning_level=3',
        'cpp_std=c++14',
    ],
    license : 'LGPL 2.1+')

libcamera = dependency('libcamera')
libevent = dependency('libevent_pthreads')
libdrm = dependency('libdrm')

incdir = include_directories('/usr/include/libcamera')

twincam_sources = files([
    'camera_session.cpp',
    'event_loop.cpp',
    'image.cpp',
    'main.cpp',
    'drm.cpp',
    'kms_sink.cpp'
])

twincam  = executable('twincam', twincam_sources, include_directories : incdir,
                  dependencies : [
                      libcamera,
                      libdrm,
                      libevent,
                  ],
                  install : true)

