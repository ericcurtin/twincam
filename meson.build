project('twincam', 'c', 'cpp',
    meson_version : '>= 0.50',
    default_options : [
        'werror=true',
        'warning_level=3',
        'cpp_std=c++17'
    ])

add_project_arguments(
  '-Wstrict-aliasing',
  '-Wchar-subscripts',
  '-Wformat-security',
  '-Wmissing-declarations',
  '-Wpointer-arith',
  '-Wshadow',
  '-Wsign-compare',
  '-Wtype-limits',
  '-Wunused-function',
  '-Wold-style-cast',
  language: 'cpp'
)

libcamera = dependency('libcamera')
libevent = dependency('libevent_pthreads')
libdrm = dependency('libdrm', required : false)
libsdl2 = dependency('SDL2', required : false)
libjpeg = dependency('libjpeg', required : false)
libudev = dependency('libudev', required : false)

incdir = include_directories('/usr/include/libcamera')

twincam_cpp_args = []

twincam_sources = files([
    'src/camera_session.cpp',
    'src/event_loop.cpp',
    'src/twincam.cpp',
    'src/twncm_fnctl.cpp',
    'src/uptime.cpp',
    'src/frame_sink.cpp',
    'src/image.cpp',
    'src/file_sink.cpp'
])

if libdrm.found()
    twincam_cpp_args += [ '-DHAVE_DRM' ]
    twincam_sources += files([
        'src/drm.cpp',
        'src/kms_sink.cpp'
    ])
endif

if libsdl2.found()
    twincam_cpp_args += ['-DHAVE_SDL']
    twincam_sources += files([
        'src/sdl_sink.cpp',
        'src/sdl_texture.cpp',
        'src/sdl_texture_yuv.cpp',
    ])

    if libjpeg.found()
        twincam_cpp_args += ['-DHAVE_LIBJPEG']
        twincam_sources += files([
            'src/jpeg_error_manager.cpp',
            'src/sdl_texture_mjpg.cpp'
        ])
    endif
endif

if libudev.found()
    twincam_cpp_args += ['-DHAVE_LIBUDEV']
endif

twincam  = executable('twincam', twincam_sources, include_directories : incdir,
                      dependencies : [
                          libcamera,
                          libdrm,
                          libevent,
                          libsdl2,
                          libjpeg,
                          libudev,
                      ],
                      cpp_args : twincam_cpp_args,
                      install : true)

