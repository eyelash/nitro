/*

Copyright (c) 2016-2017, Elias Aebi
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#pragma once

#include <GLES2/gl2.h>
#include <cmath>
#include <cstdlib>
#include <cstdio>

namespace GLES2 {

struct vec4 {
	GLfloat x, y, z, w;
};

struct mat4 {
	vec4 values[4];
	vec4& operator [] (int i) {
		return values[i];
	}
	constexpr const vec4& operator [] (int i) const {
		return values[i];
	}
	static constexpr mat4 id () {
		return mat4 {
			vec4 {1.f, 0.f, 0.f, 0.f},
			vec4 {0.f, 1.f, 0.f, 0.f},
			vec4 {0.f, 0.f, 1.f, 0.f},
			vec4 {0.f, 0.f, 0.f, 1.f}
		};
	}
};

inline constexpr vec4 operator * (const mat4& lhs, const vec4& rhs) {
	return vec4 {
		lhs[0].x * rhs.x + lhs[1].x * rhs.y + lhs[2].x * rhs.z + lhs[3].x * rhs.w,
		lhs[0].y * rhs.x + lhs[1].y * rhs.y + lhs[2].y * rhs.z + lhs[3].y * rhs.w,
		lhs[0].z * rhs.x + lhs[1].z * rhs.y + lhs[2].z * rhs.z + lhs[3].z * rhs.w,
		lhs[0].w * rhs.x + lhs[1].w * rhs.y + lhs[2].w * rhs.z + lhs[3].w * rhs.w
	};
}

inline constexpr mat4 operator * (const mat4& lhs, const mat4& rhs) {
	return mat4 {lhs * rhs[0], lhs * rhs[1], lhs * rhs[2], lhs * rhs[3]};
}

inline mat4 glOrtho (float l, float r, float b, float t, float n, float f) {
	return mat4 {
		vec4 {2.f/(r-l), 0.f, 0.f, 0.f},
		vec4 {0.f, 2.f/(t-b), 0.f, 0.f},
		vec4 {0.f, 0.f, -2.f/(f-n), 0.f},
		vec4 {-(r+l)/(r-l), -(t+b)/(t-b), -(f+n)/(f-n), 1.f}
	};
}

inline mat4 project (float w, float h, float d) {
	return mat4 {
		vec4 {2.f/w, 0.f, 0.f, 0.f},
		vec4 {0.f, 2.f/h, 0.f, 0.f},
		vec4 {0.f, 0.f, 0.f, -1.f/d},
		vec4 {-1.f, -1.f, 0.f, 1.f}
	};
}

inline mat4 translate (float x, float y, float z = 0.f) {
	return mat4 {
		vec4 {1.f, 0.f, 0.f, 0.f},
		vec4 {0.f, 1.f, 0.f, 0.f},
		vec4 {0.f, 0.f, 1.f, 0.f},
		vec4 {x, y, z, 1.f}
	};
}

inline mat4 scale (float x, float y, float z = 1.f) {
	return mat4 {
		vec4 {x, 0.f, 0.f, 0.f},
		vec4 {0.f, y, 0.f, 0.f},
		vec4 {0.f, 0.f, z, 0.f},
		vec4 {0.f, 0.f, 0.f, 1.f}
	};
}

inline mat4 rotateX (float a) {
	return mat4 {
		vec4 {1.f, 0.f, 0.f, 0.f},
		vec4 {0.f, cosf(a), sinf(a), 0.f},
		vec4 {0.f, -sinf(a), cosf(a), 0.f},
		vec4 {0.f, 0.f, 0.f, 1.f}
	};
}
inline mat4 rotateY (float a) {
	return mat4 {
		vec4 {cosf(a), 0.f, -sinf(a), 0.f},
		vec4 {0.f, 1.f, 0.f, 0.f},
		vec4 {sinf(a), 0.f, cosf(a), 0.f},
		vec4 {0.f, 0.f, 0.f, 1.f}
	};
}
inline mat4 rotateZ (float a) {
	return mat4 {
		vec4 {cosf(a), sinf(a), 0.f, 0.f},
		vec4 {-sinf(a), cosf(a), 0.f, 0.f},
		vec4 {0.f, 0.f, 1.f, 0.f},
		vec4 {0.f, 0.f, 0.f, 1.f}
	};
}

class Shader {
public:
	GLuint identifier;
	Shader(const char* source, GLenum type);
	Shader(const Shader&) = delete;
	~Shader();
	Shader& operator =(const Shader&) = delete;
};

class Program {
public:
	GLuint identifier;
	Program();
	Program(const Shader& vertex_shader, const Shader& fragment_shader);
	Program(const char* vertex_shader, const char* fragment_shader);
	Program(const Program&) = delete;
	~Program();
	Program& operator =(const Program&) = delete;
	void attach_shader(const Shader& shader);
	void detach_shader(const Shader& shader);
	void link();
	void use();
	GLint get_attribute_location(const char* name);
	GLint get_uniform_location(const char* name);
	void set_uniform(const char* name, int value);
	void set_uniform(const char* name, float value);
	void set_uniform(const char* name, const vec4& value);
	void set_uniform(const char* name, const mat4& value);
};

class Texture {
public:
	GLuint identifier;
	int width, height;
	Texture(int width, int height, int depth, const unsigned char* data);
	Texture(const Texture&) = delete;
	~Texture();
	Texture& operator =(const Texture&) = delete;
	void bind(GLenum texture_unit = GL_TEXTURE0);
	void unbind(GLenum texture_unit = GL_TEXTURE0);
};

class FramebufferObject {
public:
	GLuint identifier;
	int width, height;
	Texture* texture;
	FramebufferObject(int width, int height);
	FramebufferObject(const FramebufferObject&) = delete;
	~FramebufferObject();
	FramebufferObject& operator =(const FramebufferObject&) = delete;
	void use();
	void bind();
	void unbind();
};

class TextureState {
	GLuint identifier;
	GLenum unit;
	GLint location;
public:
	TextureState(Texture* texture, GLenum unit, GLint location): identifier(texture->identifier), unit(unit), location(location) {

	}
	void enable() const {
		glActiveTexture(unit);
		glBindTexture(GL_TEXTURE_2D, identifier);
		glUniform1i(location, unit - GL_TEXTURE0);
	}
	void disable() const {
		glActiveTexture(unit);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
};

class UniformFloat {
	GLint location;
	float value;
public:
	UniformFloat(GLint location, float value): location(location), value(value) {

	}
	void enable() const {
		glUniform1f(location, value);
	}
	void disable() const {

	}
};

class UniformVec4 {
	GLint location;
	vec4 value;
public:
	UniformVec4(GLint location, const vec4& value): location(location), value(value) {

	}
	void enable() const {
		glUniform4f(location, value.x, value.y, value.z, value.w);
	}
	void disable() const {

	}
};

class UniformMat4 {
	GLint location;
	mat4 value;
public:
	UniformMat4(GLint location, const mat4& value): location(location), value(value) {

	}
	void enable() const {
		glUniformMatrix4fv(location, 1, GL_FALSE, &value[0].x);
	}
	void disable() const {

	}
};

class AttributeVec4 {
	GLint location;
	vec4 value;
public:
	AttributeVec4(GLint location, const vec4& value): location(location), value(value) {

	}
	void enable() const {
		glVertexAttrib4f(location, value.x, value.y, value.z, value.w);
	}
	void disable() const {

	}
};

class AttributeArray {
	GLuint index;
	GLint size;
	GLenum type;
	const GLvoid* pointer;
public:
	AttributeArray(GLuint index, GLint size, GLenum type, const GLvoid* pointer): index(index), size(size), type(type), pointer(pointer) {

	}
	void enable() const {
		glVertexAttribPointer(index, size, type, GL_FALSE, 0, pointer);
		glEnableVertexAttribArray(index);
	}
	void disable() const {
		glDisableVertexAttribArray(index);
	}
};

inline void enable_state() {

}
template <class T0, class... T> void enable_state(const T0& state0, const T&... state) {
	state0.enable();
	enable_state(state...);
}
inline void disable_state() {

}
template <class T0, class... T> void disable_state(const T0& state0, const T&... state) {
	disable_state(state...);
	state0.disable();
}

template <class... T> void draw(Program* program, GLenum mode, GLsizei count, const T&... state) {
	glUseProgram(program->identifier);
	enable_state(state...);
	glDrawArrays(mode, 0, count);
	disable_state(state...);
}

}
