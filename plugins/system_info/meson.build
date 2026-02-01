shared_library('system_info',
  sources: [
    'system_info_plugin.cpp',
    'backend/sys_stats.cpp',
    'ui/info_page.cpp',
  ],
  dependencies: [
    dependency('gtk4'),
    dependency('libadwaita-1'),
  ],
  install: true,
  install_dir: get_option('libdir') / 'kerneldrive/plugins',
  name_prefix: ''
)
