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
    'src/camera_session.cpp',
    'src/event_loop.cpp',
    'src/twincam.cpp',
    'src/drm.cpp',
    'src/kms_sink.cpp',
    'src/image.cpp'
])

twincam  = executable('twincam', twincam_sources, include_directories : incdir,
                  dependencies : [
                      libcamera,
                      libdrm,
                      libevent,
                  ],
                  install : true)

