#ifndef GLSHADERPROGRAM_H
#define GLSHADERPROGRAM_H

#include <Graphics/opengl.h>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <initializer_list>
#include <unordered_map>

class GLShaderProgram {
public:
    GLShaderProgram();
    GLShaderProgram(std::initializer_list<const std::string> shaderFiles);
    ~GLShaderProgram();

    GLShaderProgram &attachShader(const std::string &shaderFile);
    GLShaderProgram &attachShader(GLenum shaderType, const std::string &shaderFile);
    void linkProgram();
    void attachAndLink(std::initializer_list<const std::string> shaderFiles);
    GLuint getHandle() const;

    void bind() const;
    void unbind() const;

    GLint uniformLocation(const GLchar *name);

    void setUniform1f(const GLchar *name, GLfloat v) { glUniform1f(uniformLocation(name), v); }
    void setUniform1i(const GLchar *name, GLint v) { glUniform1i(uniformLocation(name), v); }
    void setUniform1ui(const GLchar *name, GLuint v) { glUniform1ui(uniformLocation(name), v); }
    void setUniform3fv(const GLchar *name, const glm::vec3 &v) { glUniform3fv(uniformLocation(name), 1, glm::value_ptr(v)); }
    void setUniformMatrix4fv(const GLchar *name, const glm::mat4 &v) { glUniformMatrix4fv(uniformLocation(name), 1, GL_FALSE, glm::value_ptr(v)); }

private:
    GLuint handle;
    std::unordered_map<std::string, GLint> uniforms;
};

#endif