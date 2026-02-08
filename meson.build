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
hidapi_dep = dependency('hidapi-hidraw')
libdrm_dep = dependency('libdrm')
xrandr_dep = dependency('xrandr')
x11_dep = dependency('x11')

pkexec_path = join_paths(get_option('prefix'), get_option('bindir'), 'kerneldrive-pkexec')
add_project_arguments('-DPKEXEC_PATH="' + pkexec_path + '"', language: 'cpp')

sources = files(
  'src/main.cpp',
  'src/ui/window.cpp',
  'src/ui/pages/power_page.cpp',
  'src/ui/pages/input_page.cpp',
  'src/ui/pages/display_page.cpp',
  'src/ui/pages/dependencies_page.cpp',
  'src/ui/pages/store_page.cpp',
  'src/ui/pages/socials_page.cpp',
  'src/core/sysfs_writer.cpp',
  'src/core/plugin_manager.cpp',
  'src/core/config_manager.cpp',
  'src/core/display/display_manager.cpp',
  'src/core/display/backends/hyprland_backend.cpp',
  'src/core/display/backends/gnome_backend.cpp',
  'src/core/display/backends/x11_backend.cpp',
  'src/core/input/input_manager.cpp',
  'src/core/input/backends/hyprland_input.cpp',
  'src/core/input/backends/gnome_input.cpp',
  'src/core/input/backends/x11_input.cpp',
  'src/core/power/power_manager.cpp',
  'src/core/power/backends/linux_power.cpp',
  'src/core/dependency_manager.cpp',
  'src/core/plugin_installer.cpp',
)

cc = meson.get_compiler('cpp')
dl_dep = cc.find_library('dl', required: false)

executable('kerneldrive',
  sources,
  dependencies: [gtk_dep, libadwaita_dep, dl_dep, libdrm_dep, xrandr_dep, x11_dep],
  install: true,
  export_dynamic: true,
)

# Plugins are now built separately as standalone projects.
# Their .so files should be placed in ~/.config/kerneldrive/plugins/

executable('optimus_helper',
  'tools/optimus_helper.cpp',
  install: true,
  install_dir: get_option('bindir')
)

install_data('data/kerneldrive.desktop',
  install_dir: get_option('datadir') / 'applications'
)

install_data('data/registry.json',
  install_dir: get_option('datadir') / 'kerneldrive' / 'data'
)

conf_data = configuration_data()
conf_data.set('bindir', join_paths(get_option('prefix'), get_option('bindir')))

configure_file(
  input: 'data/com.acedmicabhishek.kerneldrive.policy',
  output: 'com.acedmicabhishek.kerneldrive.policy',
  configuration: conf_data,
  install: true,
  install_dir: get_option('datadir') / 'polkit-1' / 'actions'
)

install_data('data/kerneldrive-pkexec',
  install_dir: get_option('bindir'),
  install_mode: 'rwxr-xr-x'
)
