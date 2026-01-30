project('KernelDrive', ['cpp', 'c'],
  version: '0.1.0',
  default_options: [
    'warning_level=3',
    'cpp_std=c++23',
  ],
)

gnome = import('gnome')

gtk_dep = dependency('gtk4')
libadwaita_dep = dependency('libadwaita-1')

sources = files(
  'src/main.cpp',
  'src/ui/window.cpp',
  'src/ui/pages/power_page.cpp',
  'src/ui/pages/input_page.cpp',
  'src/ui/pages/display_page.cpp',
  'src/ui/pages/store_page.cpp',
  'src/core/sysfs_writer.cpp',
  'src/core/plugin_manager.cpp',
)

cc = meson.get_compiler('cpp')
dl_dep = cc.find_library('dl', required: false)

executable('kerneldrive',
  sources,
  dependencies: [gtk_dep, libadwaita_dep, dl_dep],
  install: true,
  export_dynamic: true,
)

# Plugins
shared_module('system_info',
  'plugins/system_info.cpp',
  dependencies: [gtk_dep, libadwaita_dep],
  install: true,
  install_dir: get_option('libdir') / 'kerneldrive/plugins'
)

shared_module('asus_armoury',
  [
    'plugins/asus_armoury/asus_plugin.cpp', 
    'plugins/asus_armoury/backend/modes.cpp',
    'plugins/asus_armoury/backend/core.cpp',
    'plugins/asus_armoury/backend/battery.cpp',
    'plugins/asus_armoury/backend/display.cpp',
    'plugins/asus_armoury/backend/stress.cpp',
    'plugins/asus_armoury/backend/monitor.cpp',
    'plugins/asus_armoury/backend/brightness.cpp',
    'plugins/asus_armoury/backend/keyboard.cpp',
    'plugins/asus_armoury/backend/gpu.cpp',
    'plugins/asus_armoury/backend/visuals.cpp'
  ],
  dependencies: [gtk_dep, libadwaita_dep],
  install: true,
  install_dir: get_option('libdir') / 'kerneldrive/plugins'
)

shared_module('logitech_ghub',
  [
    'plugins/logitech_ghub/logitech_plugin.cpp',
    'plugins/logitech_ghub/backend/logitech_peripherals.cpp'
  ],
  dependencies: [gtk_dep, libadwaita_dep],
  install: true,
  install_dir: get_option('libdir') / 'kerneldrive/plugins'
)
install_data('data/kerneldrive.desktop',
  install_dir: get_option('datadir') / 'applications'
)
