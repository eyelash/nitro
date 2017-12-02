/*

Copyright (c) 2016-2017, Elias Aebi
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "gles2.hpp"
#include <cstring>

namespace gles2 {

// Shader
Shader::Shader(const char* source, GLenum type) {
	identifier = glCreateShader(type);
	int length = strlen(source);
	glShaderSource(identifier, 1, &source, &length);
	glCompileShader(identifier);

	GLint compile_status;
	glGetShaderiv(identifier, GL_COMPILE_STATUS, &compile_status);
	if (compile_status == GL_FALSE) {
		GLint log_length;
		glGetShaderiv(identifier, GL_INFO_LOG_LENGTH, &log_length);
		char* log = new char[log_length];
		glGetShaderInfoLog(identifier, log_length, &log_length, log);
		fprintf(stderr, "shader compilation error:\n%s\n", log);
		delete[] log;
	}
}
Shader::~Shader() {
	glDeleteShader(identifier);
}

// Program
Program::Program() {
	identifier = glCreateProgram();
}
Program::Program(const Shader& vertex_shader, const Shader& fragment_shader): Program() {
	attach_shader(vertex_shader);
	attach_shader(fragment_shader);
	link();
	detach_shader(vertex_shader);
	detach_shader(fragment_shader);
}
Program::Program(const char* vertex_shader, const char* fragment_shader): Program(Shader(vertex_shader, GL_VERTEX_SHADER), Shader(fragment_shader, GL_FRAGMENT_SHADER)) {

}
Program::~Program() {
	glDeleteProgram(identifier);
}
void Program::link() {
	glLinkProgram(identifier);

	GLint link_status;
	glGetProgramiv(identifier, GL_LINK_STATUS, &link_status);
	if (link_status == GL_FALSE) {
		GLint log_length;
		glGetProgramiv(identifier, GL_INFO_LOG_LENGTH, &log_length);
		char* log = new char[log_length];
		glGetProgramInfoLog(identifier, log_length, &log_length, log);
		fprintf(stderr, "program link error:\n%s\n", log);
		delete[] log;
	}
}
GLint Program::get_attribute_location(const char* name) {
	return glGetAttribLocation(identifier, name);
}
GLint Program::get_uniform_location(const char* name) {
	return glGetUniformLocation(identifier, name);
}

// Texture
Texture::Texture(int width, int height, int depth, const unsigned char* data) {
	glGenTextures(1, &identifier);
	glBindTexture(GL_TEXTURE_2D, identifier);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	if (depth == 1)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);
	else if (depth == 3)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	else if (depth == 4)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glBindTexture(GL_TEXTURE_2D, 0);
}
Texture::~Texture() {
	glDeleteTextures(1, &identifier);
}
void Texture::bind(GLenum texture_unit) {
	glActiveTexture(texture_unit);
	glBindTexture(GL_TEXTURE_2D, identifier);
}
void Texture::unbind(GLenum texture_unit) {
	glActiveTexture(texture_unit);
	glBindTexture(GL_TEXTURE_2D, 0);
}

// FramebufferObject
FramebufferObject::FramebufferObject(int width, int height): width(width), height(height), texture(new Texture(width, height, 4, nullptr)) {
	glGenFramebuffers(1, &identifier);
	bind();
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->identifier, 0);
	unbind();
}
FramebufferObject::~FramebufferObject() {
	glDeleteFramebuffers(1, &identifier);
}
void FramebufferObject::use() {
	bind();
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT);
}

}
