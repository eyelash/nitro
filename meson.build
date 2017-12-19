project('nitro', 'cpp', default_options: ['cpp_std=c++11'])

sources = [
	'nitro.cpp',
	'gles2.cpp',
	'animation.cpp',
	'text.cpp',
	'window_x11.cpp',
	'widgets.cpp',
]
shaders = [
	'shaders/color.fs.glsl',
	'shaders/color.vs.glsl',
	'shaders/texture.fs.glsl',
	'shaders/texture.vs.glsl',
	'shaders/color_mask.fs.glsl',
	'shaders/color_mask.vs.glsl',
	'shaders/texture_mask.fs.glsl',
	'shaders/texture_mask.vs.glsl',
]
dependencies = [
	dependency('epoxy'),
	dependency('x11'),
	dependency('libpng', version: '>= 1.5.4'),
	dependency('icu-uc'),
	dependency('harfbuzz-icu'),
	dependency('freetype2'),
	dependency('fontconfig'),
]

glsl2h = generator(executable('glsl2h', 'glsl2h.cpp'), output: '@PLAINNAME@.h', arguments: ['@INPUT@', '@OUTPUT@'])
nitro = library('nitro', sources, glsl2h.process(shaders), dependencies: dependencies)
nitro_dep = declare_dependency(link_with: nitro, dependencies: dependencies, include_directories: include_directories('.'))

executable('demo', 'demo.cpp', dependencies: nitro_dep)
