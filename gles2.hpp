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
	const vec4& operator [] (int i) const {
		return values[i];
	}
};

inline vec4 operator * (const mat4& lhs, const vec4& rhs) {
	return vec4 {
		lhs[0].x * rhs.x + lhs[1].x * rhs.y + lhs[2].x * rhs.z + lhs[3].x * rhs.w,
		lhs[0].y * rhs.x + lhs[1].y * rhs.y + lhs[2].y * rhs.z + lhs[3].y * rhs.w,
		lhs[0].z * rhs.x + lhs[1].z * rhs.y + lhs[2].z * rhs.z + lhs[3].z * rhs.w,
		lhs[0].w * rhs.x + lhs[1].w * rhs.y + lhs[2].w * rhs.z + lhs[3].w * rhs.w
	};
}

inline mat4 operator * (const mat4& lhs, const mat4& rhs) {
	return mat4 {lhs * rhs[0], lhs * rhs[1], lhs * rhs[2], lhs * rhs[3]};
}

static mat4 glOrtho (float l, float r, float b, float t, float n, float f) {
	return mat4 {
		vec4 {2.f/(r-l), 0.f, 0.f, 0.f},
		vec4 {0.f, 2.f/(t-b), 0.f, 0.f},
		vec4 {0.f, 0.f, -2.f/(f-n), 0.f},
		vec4 {-(r+l)/(r-l), -(t+b)/(t-b), -(f+n)/(f-n), 1.f}
	};
}

static mat4 translate (float x, float y, float z = 0.f) {
	return mat4 {
		vec4 {1.f, 0.f, 0.f, 0.f},
		vec4 {0.f, 1.f, 0.f, 0.f},
		vec4 {0.f, 0.f, 1.f, 0.f},
		vec4 {x, y, z, 1.f}
	};
}

static mat4 rotateZ (float a) {
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
	Shader (const char* filename, GLenum type);
	Shader (const Shader&) = delete;
	~Shader ();
	Shader& operator = (const Shader&) = delete;
};

class Program {
public:
	GLuint identifier;
	Program (Shader* vertex_shader, Shader* fragment_shader);
	Program (const char* vertex_shader, const char* fragment_shader);
	Program ();
	~Program ();
	void attach_shader (Shader* shader);
	void link ();
	void use ();
	GLint get_attribute_location (const char* name);
	void set_attribute (const char* name, const vec4& value);
	void set_uniform (const char* name, const mat4& value);
};

class Texture {
public:
	GLuint identifier;
	int width, height;
	Texture (int width, int height, int depth, const unsigned char* data);
	~Texture ();
	void bind ();
	void unbind ();
};

class FramebufferObject {
public:
	GLuint identifier;
	int width, height;
	Texture* texture;
	FramebufferObject (int width, int height);
	~FramebufferObject ();
	void use ();
	void bind ();
	void unbind ();
};

}
