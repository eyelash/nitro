project('nitro', 'cpp', default_options: ['cpp_std=c++17'])

sources = [
	'nitro.cpp',
	'gles2.cpp',
	'canvas.cpp',
	'animation.cpp',
	'text.cpp',
	'window_x11.cpp',
	'widgets.cpp',
]
shaders = [
	'shaders/canvas.vs.glsl',
	'shaders/canvas.fs.glsl',
]
dependencies = [
	dependency('epoxy'),
	dependency('x11'),
	dependency('libpng', version: '>= 1.5.4'),
	dependency('harfbuzz'),
	dependency('freetype2'),
	dependency('fontconfig'),
]

glsl2h = generator(executable('glsl2h', 'glsl2h.cpp'), output: '@PLAINNAME@.h', arguments: ['@INPUT@', '@OUTPUT@'])
nitro = library('nitro', sources, glsl2h.process(shaders), dependencies: dependencies)
nitro_dep = declare_dependency(link_with: nitro, dependencies: dependencies, include_directories: include_directories('.'))

executable('demo', 'demo.cpp', dependencies: nitro_dep)
