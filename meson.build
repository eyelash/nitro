project('atmosphere', 'cpp', 'c', default_options: ['cpp_std=c++11'])

sources = [
	'scene_graph.cpp',
	'gles2.cpp',
	'animation.cpp',
	'text.cpp',
	'window_x11.cpp',
	'3rdparty/3rdparty.c',
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
	dependency('glesv2'),
	dependency('egl'),
	dependency('x11'),
	dependency('harfbuzz'),
	dependency('freetype2'),
	dependency('fontconfig'),
]

glsl2h = generator(executable('glsl2h', 'glsl2h.c'), output: '@PLAINNAME@.h', arguments: ['@INPUT@', '@OUTPUT@'])
atmosphere = library('atmosphere', sources, glsl2h.process(shaders), dependencies: dependencies, include_directories: include_directories('3rdparty'))

executable('demo', 'demo.cpp', link_with: atmosphere, dependencies: dependencies)
