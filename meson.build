project('mirrolink', 'cpp',
  version : '0.1.0',
  default_options : ['cpp_std=c++17', 'warning_level=3']
)

# Dependencies
sdl2_dep = dependency('sdl2')
ffmpeg_dep = dependency('libavcodec')
libusb_dep = dependency('libusb-1.0')

# Core library
mirrolink_core_sources = [
  'src/core/device_manager.cpp',
  'src/core/screen_mirror.cpp',
  'src/core/input_handler.cpp',
  'src/core/audio_forwarder.cpp',
]

mirrolink_core = static_library('mirrolink_core',
  mirrolink_core_sources,
  dependencies : [ffmpeg_dep, libusb_dep],
  include_directories : include_directories('src')
)

# GUI application
mirrolink_gui_sources = [
  'src/gui/main_window.cpp',
  'src/gui/device_view.cpp',
  'src/gui/settings_dialog.cpp',
]

executable('mirrolink',
  mirrolink_gui_sources,
  link_with : mirrolink_core,
  dependencies : [sdl2_dep],
  install : true
)

# Tests
gtest_dep = dependency('gtest', required : false)
if gtest_dep.found()
  test_sources = [
    'tests/unit/device_manager_test.cpp',
    'tests/unit/screen_mirror_test.cpp',
  ]
  
  test_exe = executable('mirrolink_tests',
    test_sources,
    link_with : mirrolink_core,
    dependencies : [gtest_dep]
  )
  
  test('unit tests', test_exe)
endif