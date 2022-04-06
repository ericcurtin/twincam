project('twincam', 'c', 'cpp',
    meson_version : '>= 0.50',
    default_options : [
        'werror=true',
        'warning_level=3',
        'cpp_std=c++17'
    ])

libcamera = dependency('libcamera')
libevent = dependency('libevent_pthreads')
libdrm = dependency('libdrm')
libsdl2 = dependency('SDL2', required : false)

incdir = include_directories('/usr/include/libcamera')

twincam_sources = files([
    'src/camera_session.cpp',
    'src/event_loop.cpp',
    'src/image.cpp',
    'src/sdl_sink.cpp',
    'src/twincam.cpp',
])

twincam  = executable('twincam', twincam_sources, include_directories : incdir,
                  dependencies : [
                      libcamera,
                      libsdl2,
                      libevent,
                  ],
                  install : true)

