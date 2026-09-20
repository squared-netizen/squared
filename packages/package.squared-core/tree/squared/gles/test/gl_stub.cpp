// A fake GLES 3.0 driver for host testing.
//
// Defined against the real NDK headers, so every signature is checked by the
// compiler rather than by me. It records what was called, which is what lets a
// test assert things a real driver cannot be asked about - such as "nothing
// was deleted after invalidate()".
#include <GLES3/gl3.h>

#include <cstring>
#include <string>
#include <vector>

namespace glstub {

struct Call { std::string name; unsigned int a{0}; unsigned int b{0}; };

std::vector<Call> calls;
GLuint next_name = 1;
GLenum pending_error = GL_NO_ERROR;
bool fail_compile = false;
bool fail_link = false;
GLint unpack_alignment = 4;

void reset()
{
    calls.clear();
    next_name = 1;
    pending_error = GL_NO_ERROR;
    fail_compile = false;
    fail_link = false;
    unpack_alignment = 4;
}

int count(const char* name)
{
    int total = 0;
    for (const Call& call : calls) if (call.name == name) ++total;
    return total;
}

bool deleted(const char* name, unsigned int object)
{
    for (const Call& call : calls) {
        if (call.name == name && call.a == object) return true;
    }
    return false;
}

void record(const char* name, unsigned int a = 0, unsigned int b = 0)
{
    calls.push_back(Call{name, a, b});
}

}  // namespace glstub

using glstub::record;

extern "C" {

GLenum glGetError() { GLenum e = glstub::pending_error; glstub::pending_error = GL_NO_ERROR; return e; }

void glGenBuffers(GLsizei n, GLuint* out) { for (GLsizei i = 0; i < n; ++i) out[i] = glstub::next_name++; record("glGenBuffers"); }
void glBindBuffer(GLenum t, GLuint b) { record("glBindBuffer", t, b); }
void glBufferData(GLenum t, GLsizeiptr, const void*, GLenum) { record("glBufferData", t); }
void glBufferSubData(GLenum t, GLintptr, GLsizeiptr, const void*) { record("glBufferSubData", t); }
void glDeleteBuffers(GLsizei n, const GLuint* b) { for (GLsizei i = 0; i < n; ++i) record("glDeleteBuffers", b[i]); }

GLuint glCreateShader(GLenum) { record("glCreateShader"); return glstub::next_name++; }
void glShaderSource(GLuint, GLsizei, const GLchar* const*, const GLint*) { record("glShaderSource"); }
void glCompileShader(GLuint s) { record("glCompileShader", s); }
void glGetShaderiv(GLuint, GLenum p, GLint* out)
{
    if (p == GL_COMPILE_STATUS) *out = glstub::fail_compile ? GL_FALSE : GL_TRUE;
    else if (p == GL_INFO_LOG_LENGTH) *out = glstub::fail_compile ? 21 : 0;
    else *out = 0;
}
void glGetShaderInfoLog(GLuint, GLsizei max, GLsizei* written, GLchar* log)
{
    const char* text = "0:3: syntax error\n";
    GLsizei n = static_cast<GLsizei>(std::strlen(text));
    if (n > max - 1) n = max - 1;
    std::memcpy(log, text, static_cast<std::size_t>(n));
    log[n] = '\0';
    if (written) *written = n;
}
void glDeleteShader(GLuint s) { record("glDeleteShader", s); }

GLuint glCreateProgram() { record("glCreateProgram"); return glstub::next_name++; }
void glAttachShader(GLuint p, GLuint s) { record("glAttachShader", p, s); }
void glDetachShader(GLuint p, GLuint s) { record("glDetachShader", p, s); }
void glLinkProgram(GLuint p) { record("glLinkProgram", p); }
void glGetProgramiv(GLuint, GLenum p, GLint* out)
{
    if (p == GL_LINK_STATUS) *out = glstub::fail_link ? GL_FALSE : GL_TRUE;
    else if (p == GL_INFO_LOG_LENGTH) *out = glstub::fail_link ? 21 : 0;
    else if (p == GL_ACTIVE_ATTRIBUTES) *out = 1;
    else if (p == GL_ACTIVE_UNIFORMS) *out = 1;
    else if (p == GL_ACTIVE_ATTRIBUTE_MAX_LENGTH) *out = 32;
    else if (p == GL_ACTIVE_UNIFORM_MAX_LENGTH) *out = 32;
    else *out = 0;
}
void glGetProgramInfoLog(GLuint, GLsizei max, GLsizei* written, GLchar* log)
{
    const char* text = "link error\n";
    GLsizei n = static_cast<GLsizei>(std::strlen(text));
    if (n > max - 1) n = max - 1;
    std::memcpy(log, text, static_cast<std::size_t>(n));
    log[n] = '\0';
    if (written) *written = n;
}
void glUseProgram(GLuint p) { record("glUseProgram", p); }
void glDeleteProgram(GLuint p) { record("glDeleteProgram", p); }
GLint glGetUniformLocation(GLuint, const GLchar* name)
{
    return std::strcmp(name, "u_projection") == 0 ? 7 : -1;
}
GLint glGetAttribLocation(GLuint, const GLchar* name)
{
    return std::strcmp(name, "a_position") == 0 ? 0 : -1;
}
void glGetActiveAttrib(GLuint, GLuint, GLsizei max, GLsizei* len, GLint* size, GLenum* type, GLchar* name)
{
    const char* text = "a_position";
    GLsizei n = static_cast<GLsizei>(std::strlen(text));
    if (n > max - 1) n = max - 1;
    std::memcpy(name, text, static_cast<std::size_t>(n));
    name[n] = '\0';
    if (len) *len = n;
    *size = 1;
    *type = GL_FLOAT_VEC2;
}
void glGetActiveUniform(GLuint, GLuint, GLsizei max, GLsizei* len, GLint* size, GLenum* type, GLchar* name)
{
    const char* text = "u_projection";
    GLsizei n = static_cast<GLsizei>(std::strlen(text));
    if (n > max - 1) n = max - 1;
    std::memcpy(name, text, static_cast<std::size_t>(n));
    name[n] = '\0';
    if (len) *len = n;
    *size = 1;
    *type = GL_FLOAT_MAT4;
}
void glUniform1f(GLint, GLfloat) { record("glUniform1f"); }
void glUniform1i(GLint, GLint) { record("glUniform1i"); }
void glUniform2f(GLint, GLfloat, GLfloat) { record("glUniform2f"); }
void glUniform4f(GLint, GLfloat, GLfloat, GLfloat, GLfloat) { record("glUniform4f"); }
void glUniformMatrix4fv(GLint, GLsizei, GLboolean t, const GLfloat*) { record("glUniformMatrix4fv", t); }

void glGenVertexArrays(GLsizei n, GLuint* out) { for (GLsizei i = 0; i < n; ++i) out[i] = glstub::next_name++; record("glGenVertexArrays"); }
void glBindVertexArray(GLuint a) { record("glBindVertexArray", a); }
void glDeleteVertexArrays(GLsizei n, const GLuint* a) { for (GLsizei i = 0; i < n; ++i) record("glDeleteVertexArrays", a[i]); }
void glEnableVertexAttribArray(GLuint i) { record("glEnableVertexAttribArray", i); }
void glVertexAttribPointer(GLuint i, GLint, GLenum, GLboolean, GLsizei, const void*) { record("glVertexAttribPointer", i); }
void glVertexAttribDivisor(GLuint i, GLuint d) { record("glVertexAttribDivisor", i, d); }

void glGenTextures(GLsizei n, GLuint* out) { for (GLsizei i = 0; i < n; ++i) out[i] = glstub::next_name++; record("glGenTextures"); }
void glBindTexture(GLenum t, GLuint x) { record("glBindTexture", t, x); }
void glTexImage2D(GLenum, GLint, GLint internal, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*) { record("glTexImage2D", static_cast<unsigned>(internal)); }
void glTexSubImage2D(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void*) { record("glTexSubImage2D"); }
void glTexParameteri(GLenum, GLenum p, GLint v) { record("glTexParameteri", p, static_cast<unsigned>(v)); }
void glDeleteTextures(GLsizei n, const GLuint* t) { for (GLsizei i = 0; i < n; ++i) record("glDeleteTextures", t[i]); }
void glActiveTexture(GLenum u) { record("glActiveTexture", u); }
void glPixelStorei(GLenum p, GLint v) { if (p == GL_UNPACK_ALIGNMENT) glstub::unpack_alignment = v; record("glPixelStorei", p, static_cast<unsigned>(v)); }

void glViewport(GLint, GLint, GLsizei, GLsizei) { record("glViewport"); }
void glClearColor(GLfloat, GLfloat, GLfloat, GLfloat) { record("glClearColor"); }
void glClear(GLbitfield m) { record("glClear", m); }

}  // extern "C"
