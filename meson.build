project('evol-mod-game', 'c',
version : '0.1',
default_options : ['warning_level=3', 'c_std=c11'])

src = subproject('evol').get_variable('evolmod_subdir') + '/meson.build'
dst = meson.current_source_dir() + '/import/'
run_command( 'python3', meson.source_root() + '/subprojects/evol/buildscripts/copy.py', src, dst)
subdir('import')

mod_src = [
  'src/mod.c',
]

mod_incdir = [
  '..',
]

mod_deps = [
  evmod_deps,
]

module = shared_module(
  'game', mod_src,
  include_directories: mod_incdir,
  dependencies: mod_deps,
  name_prefix: evmod_name_prefix,
  name_suffix: evmod_name_suffix,
)

mod_dep = declare_dependency(
  include_directories: mod_incdir,
)

meson.override_dependency('evmod_game', mod_dep)

configure_file(input: 'src/lua/script_api.lua', output: 'script_api.lua', copy:true)
