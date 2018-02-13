#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "GLHelper.h"
#include <iostream>
#include <string>
#include <vector>

static void APIENTRY glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);

void GLHelper::printGLInfo() {
    const GLubyte *vendor = glGetString(GL_VENDOR);
    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *version = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    std::cout << "OpenGL Info" << std::endl
        << "Vendor: " << vendor << std::endl
        << "Renderer: " << renderer << std::endl
        << "Version: " << version << std::endl
        << "GLSL Version: " << glslversion << std::endl << std::endl;
}

void GLHelper::printGLExtensions() {
    const GLubyte *extension;
    std::cout << "Extensions: ";
    for (GLuint index = 0; (extension = glGetStringi(GL_EXTENSIONS, index++)) != nullptr;) {
        std::cout << extension << " ";
    }
    std::cout << std::endl << std::endl;
}

void GLHelper::registerDebugOutputCallback() { 
    GLint flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        std::cout << "OpenGL Debug Context Created" << std::endl;
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
    }
    else {
        std::cout << "OpenGL Debug Context NOT Created" << std::endl;
    }
}

void GLHelper::printUniformInfo(GLuint program) {
    GLint uniformCount;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &uniformCount);
    
	std::vector<GLuint> uniformIndices{ (GLuint)uniformCount };
    std::vector<std::string> uniformNames{ (GLuint)uniformCount };
    for (int i = 0; i < uniformCount; i++) {
        uniformIndices[i] = i;

		const GLsizei bufSize = 1024;
		GLsizei length;
        char uniformName[bufSize];
        glGetActiveUniformName(program, i, bufSize, &length, uniformName);
        uniformNames[i] = std::string(uniformName);
    }

	std::vector<GLint> uniformType{ uniformCount };
    std::vector<GLint> uniformSize{ uniformCount };
    std::vector<GLint> uniformBlockIndex{ uniformCount };
    std::vector<GLint> uniformOffset{ uniformCount };
    std::vector<GLint> uniformArrayStride{ uniformCount };
    std::vector<GLint> uniformMatrixStride{ uniformCount };

    glGetActiveUniformsiv(program, uniformCount, uniformIndices.data(), GL_UNIFORM_TYPE, uniformType.data());
    glGetActiveUniformsiv(program, uniformCount, uniformIndices.data(), GL_UNIFORM_SIZE, uniformSize.data());
    glGetActiveUniformsiv(program, uniformCount, uniformIndices.data(), GL_UNIFORM_BLOCK_INDEX, uniformBlockIndex.data());
    glGetActiveUniformsiv(program, uniformCount, uniformIndices.data(), GL_UNIFORM_OFFSET, uniformOffset.data());
    glGetActiveUniformsiv(program, uniformCount, uniformIndices.data(), GL_UNIFORM_ARRAY_STRIDE, uniformArrayStride.data());
    glGetActiveUniformsiv(program, uniformCount, uniformIndices.data(), GL_UNIFORM_MATRIX_STRIDE, uniformMatrixStride.data());

    std::cout << "Uniform Info <type name (size, blockIndex, offset, arrayStride, matrixStride)>" << std::endl;
    GLint arrayCount = 0;
    for (int i = 0; i < uniformCount; i++) {
        std::cout
            << uniformType[i] << "\t" << uniformNames[i] << "\t\t"
            << "(" << uniformSize[i] << ", " << uniformBlockIndex[i] << ", " << uniformOffset[i] << ", "
            << uniformArrayStride[i] << ", " << uniformMatrixStride[i] << ")"
            << std::endl;
    }
}

void GLHelper::getMemoryUsage(GLint &totalMem, GLint &availableMem) {
    if (GLEW_NVX_gpu_memory_info) {
        glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &totalMem);
        glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &availableMem);
    }
    else {
        totalMem = 0;
        availableMem = 0;
    }
}

static void APIENTRY glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) {
	// Ignore "Texture 0 is base level inconsistent" warning
	if (id == 131204) return;
	// Ignore buffer creation info
	if (id == 131185) return;
	// Ignore framebuffer creation info
	if (id == 131169) return;
	// Ignore allocation info
	if (id == 131184) return;

	std::cout << "DEBUG (" << id << "): " << message << std::endl;

	std::cout << "Source: ";
	switch (source) {
		case GL_DEBUG_SOURCE_API: std::cout << "API"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM: std::cout << "Window System"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Shader Compiler"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY: std::cout << "Third Party"; break;
		case GL_DEBUG_SOURCE_APPLICATION: std::cout << "Application"; break;
		case GL_DEBUG_SOURCE_OTHER: std::cout << "Other"; break;
		default: std::cout << "Unkown"; break;
	}

	std::cout << std::endl << "Type: ";
	switch (type) {
		case GL_DEBUG_TYPE_ERROR: std::cout << "Error"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Deprecated Behavior"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: std::cout << "Undefined Behavior"; break;
		case GL_DEBUG_TYPE_PORTABILITY: std::cout << "Portability"; break;
		case GL_DEBUG_TYPE_PERFORMANCE: std::cout << "Performance"; break;
		case GL_DEBUG_TYPE_MARKER: std::cout << "Marker"; break;
		case GL_DEBUG_TYPE_PUSH_GROUP: std::cout << "Push Group"; break;
		case GL_DEBUG_TYPE_POP_GROUP: std::cout << "Pop Group"; break;
		case GL_DEBUG_TYPE_OTHER: std::cout << "Other"; break;
		default: std::cout << "Unknown"; break;
	}

	std::cout << std::endl << "Severity: ";
	switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH: std::cout << "High"; break;
		case GL_DEBUG_SEVERITY_MEDIUM: std::cout << "Medium"; break;
		case GL_DEBUG_SEVERITY_LOW: std::cout << "Low"; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Notification"; break;
		default: std::cout << "Unkown"; break;
	}

	std::cout << std::endl << std::endl;
}
