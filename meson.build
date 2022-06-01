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
  language: 'cpp'
)

libcamera = dependency('libcamera')
libevent = dependency('libevent_pthreads')
libdrm = dependency('libdrm')
libsdl2 = dependency('SDL2', required : false)
libsdl2_image = dependency('SDL2_image', required : false)

incdir = include_directories('/usr/include/libcamera')

cam_cpp_args = []

twincam_sources = files([
    'src/camera_session.cpp',
    'src/event_loop.cpp',
    'src/twincam.cpp',
    'src/twncm_fnctl.cpp',
    'src/drm.cpp',
    'src/kms_sink.cpp',
    'src/uptime.cpp',
    'src/frame_sink.cpp',
    'src/image.cpp',
])

if libsdl2.found()
    cam_cpp_args += ['-DHAVE_SDL']
    twincam_sources += files([
        'src/sdl_sink.cpp',
        'src/sdl_texture.cpp',
        'src/sdl_texture_yuyv.cpp',
    ])

    if libsdl2_image.found()
        cam_cpp_args += ['-DHAVE_SDL_IMAGE']
        twincam_sources += files([
            'src/sdl_texture_mjpg.cpp'
        ])
    endif
endif


twincam  = executable('twincam', twincam_sources, include_directories : incdir,
                      dependencies : [
                          libcamera,
                          libdrm,
                          libevent,
                          libsdl2,
                          libsdl2_image,
                      ],
                      cpp_args : cam_cpp_args,
                      install : true)

