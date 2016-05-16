#include "gles2.hpp"

namespace GLES2 {

static void print_error (const char* origin) {
	GLenum error = glGetError ();
	if (error == GL_NO_ERROR)
		return;
	else if (error == GL_INVALID_ENUM)
		fprintf (stderr, "%s GL_INVALID_ENUM\n", origin);
	else if (error == GL_INVALID_VALUE)
		fprintf (stderr, "%s GL_INVALID_VALUE\n", origin);
	else if (error == GL_INVALID_OPERATION)
		fprintf (stderr, "%s GL_INVALID_OPERATION\n", origin);
	else if (error == GL_INVALID_FRAMEBUFFER_OPERATION)
		fprintf (stderr, "%s GL_INVALID_FRAMEBUFFER_OPERATION\n", origin);
	else if (error == GL_OUT_OF_MEMORY)
		fprintf (stderr, "%s GL_OUT_OF_MEMORY\n", origin);
	else
		fprintf (stderr, "%s unknown error\n", origin);
}

static char* read_file (const char* file_name, int* length) {
	FILE* file = fopen (file_name, "r");
	if (!file) return nullptr;
	// determine the length
	fseek (file, 0, SEEK_END);
	*length = ftell (file);
	rewind (file);
	// allocate memory and read the file
	char* content = (char*) malloc (*length);
	fread (content, 1, *length, file);
	fclose (file);
	return content;
}

// Shader
Shader::Shader (const char* file_name, GLenum type) {
	int length;
	GLchar* source = read_file (file_name, &length);
	if (source == nullptr) {
		fprintf (stderr, "Shader::Shader(): could not open the file %s\n", file_name);
		return;
	}

	identifier = glCreateShader (type);
	glShaderSource (identifier, 1, &source, &length);
	glCompileShader (identifier);

	GLint compile_status;
	glGetShaderiv (identifier, GL_COMPILE_STATUS, &compile_status);
	if (compile_status == GL_FALSE) {
		GLint log_length;
		glGetShaderiv (identifier, GL_INFO_LOG_LENGTH, &log_length);
		char* log = (char*) malloc (log_length);
		glGetShaderInfoLog (identifier, log_length, NULL, log);
		printf ("the following errors occurred during the compilation of %s:\n%s\n", file_name, log);
		free (log);
	}
	else {
		printf ("%s successfully compiled\n", file_name);
	}

	free (source);
}
Shader::~Shader () {
	glDeleteShader (identifier);
}

// Program
Program::Program (Shader* vertex_shader, Shader* fragment_shader) {
	identifier = glCreateProgram ();
	glAttachShader (identifier, vertex_shader->identifier);
	glAttachShader (identifier, fragment_shader->identifier);
	glLinkProgram (identifier);
	// add error handling here
}
Program::Program (const char* vertex_shader, const char* fragment_shader) {
	identifier = glCreateProgram ();
	Shader v (vertex_shader, GL_VERTEX_SHADER);
	Shader f (fragment_shader, GL_FRAGMENT_SHADER);
	glAttachShader (identifier, v.identifier);
	glAttachShader (identifier, f.identifier);
	link ();
	//glDetachShader (identifier, v.identifier);
	//glDetachShader (identifier, f.identifier);
}
Program::Program () {
	identifier = glCreateProgram ();
}
Program::~Program () {
	glDeleteProgram (identifier);
}
void Program::attach_shader (Shader* shader) {
	glAttachShader (identifier, shader->identifier);
}
void Program::detach_shader (Shader* shader) {
	glDetachShader (identifier, shader->identifier);
}
void Program::link () {
	glLinkProgram (identifier);
	// add error handling here
}
void Program::use () {
	glUseProgram (identifier);
}
GLint Program::get_attribute_location (const char* name) {
	return glGetAttribLocation (identifier, name);
}
void Program::set_attribute (const char* name, const vec4& value) {
	GLint location = glGetAttribLocation (identifier, name);
	glVertexAttrib4f (location, value.x, value.y, value.z, value.w);
}
void Program::set_uniform (const char* name, int value) {
	GLint location = glGetUniformLocation (identifier, name);
	glUniform1i (location, value);
}
void Program::set_uniform (const char* name, const vec4& value) {
	GLint location = glGetUniformLocation (identifier, name);
	glUniform4f (location, value.x, value.y, value.z, value.w);
}
void Program::set_uniform (const char* name, const mat4& value) {
	GLint location = glGetUniformLocation (identifier, name);
	glUniformMatrix4fv (location, 1, GL_FALSE, &value[0].x);
}

// Texture
Texture::Texture (int width, int height, int depth, const unsigned char* data): width(width), height(height) {
	glGenTextures (1, &identifier);
	bind ();
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
	if (depth == 1)
		glTexImage2D (GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);
	else if (depth == 3)
		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	else if (depth == 4)
		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	unbind ();
}
Texture::~Texture () {
	glDeleteTextures (1, &identifier);
}
void Texture::bind (GLenum texture_unit) {
	glActiveTexture (texture_unit);
	glBindTexture (GL_TEXTURE_2D, identifier);
}
void Texture::unbind (GLenum texture_unit) {
	glActiveTexture (texture_unit);
	glBindTexture (GL_TEXTURE_2D, 0);
}

// FramebufferObject
FramebufferObject::FramebufferObject (int width, int height): width(width), height(height) {
	texture = new Texture (width, height, 4, nullptr);
	glGenFramebuffers (1, &identifier);
	bind ();
	glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->identifier, 0);
	unbind ();
}
FramebufferObject::~FramebufferObject () {
	glDeleteFramebuffers (1, &identifier);
}
void FramebufferObject::use () {
	bind ();
	glViewport (0, 0, width, height);
	glClear (GL_COLOR_BUFFER_BIT);
}
void FramebufferObject::bind () {
	glBindFramebuffer (GL_FRAMEBUFFER, identifier);
}
void FramebufferObject::unbind () {
	glBindFramebuffer (GL_FRAMEBUFFER, 0);
}

}
