project('KernelDrive', 'cpp',
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
)

executable('kerneldrive',
  sources,
  dependencies: [gtk_dep, libadwaita_dep],
  install: true,
)
