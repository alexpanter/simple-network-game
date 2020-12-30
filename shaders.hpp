//
// Shader Library
//
// Functions to read shader files,
// compile and load them together.
// Also contains a wrapper class.

#pragma once

// GLEW
#ifndef GLEW_STATIC
#define GLEW_STATIC
#endif
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// CUSTOM
#include "fileIO.hpp"

// STANDARD
#include <string>
#include <string.h>
#include <fstream>
#include <iostream>
#include <vector>


namespace Shaders
{
    // --- UNIFORM VARIABLES --- //

    // valid types of uniform variables - each type *must*
    // correspond to a method in the `ShaderWrapper' class.
    typedef enum {
        UNIFORM_BOOL,
        UNIFORM_FLOAT,
        UNIFORM_INT,
        UNIFORM_UINT,
        UNIFORM_MAT4,
        UNIFORM_VEC2,
        UNIFORM_VEC3,
		UNIFORM_VEC4,
        UNIFORM_TEXTURE
    } _uniform_t;

    const char* matchUniformType(_uniform_t type)
    {
        switch(type) {
        case UNIFORM_BOOL:
            return "bool";
        case UNIFORM_FLOAT:
            return "float";
        case UNIFORM_INT:
            return "int";
        case UNIFORM_UINT:
            return "unsigned int";
        case UNIFORM_MAT4:
            return "mat4";
        case UNIFORM_VEC2:
            return "vec2";
        case UNIFORM_VEC3:
            return "vec3";
		case UNIFORM_VEC4:
            return "vec4";
        case UNIFORM_TEXTURE:
            return "texture (aka int)";
        default:
            return "unrecognized type";
        }
    }

    // return a useful error message if the uniform variable
    // could not be found in the compiled shader program
    void checkUniformVariable(GLint location, const char* name, _uniform_t type)
    {
        if(location == -1) {
            std::cerr << "'" << name << "' of type '" << matchUniformType(type)
                      << "' does not correspond to an active uniform variable,"
                      << " or it is associated with a named uniform block."
                      << std::endl;
        }
    }

    // return the maximum of permitted uniform locations within
    // a single shader program. This is guaranteed to be at least
    // 1024 locations.
    inline int getMaxUniformLocations()
    {
        return GL_MAX_UNIFORM_LOCATIONS;
    }



    // --- SHADER COMPILATION --- //

    // load, compile, and return a shader of the specified `type`
    GLuint loadShader(GLenum type, const char* path)
    {
        // load shader file
        std::string shader_str = FileIO::readFileContents(path);
        const char* shader_src = shader_str.c_str();

        // create shader
        GLuint shader = glCreateShader(type);

        // Compile vertex shader
        switch (type)
        {
        case GL_VERTEX_SHADER:
            std::cout << "Compiling vertex shader" << std::endl;
            break;
        case GL_GEOMETRY_SHADER:
            std::cout << "Compiling geometry shader" << std::endl;
            break;
        case GL_FRAGMENT_SHADER:
            std::cout << "Compiling fragment shader" << std::endl;
            break;
        case GL_COMPUTE_SHADER:
        case GL_TESS_CONTROL_SHADER:
        case GL_TESS_EVALUATION_SHADER:
        default:
            std::cout << "Error: Unrecognized shader type" << std::endl;
            return 0;
        }
        glShaderSource(shader, 1, &shader_src, NULL);
        glCompileShader(shader);

        // check if compilation was successful
        GLint result = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &result);

        // if compilation did not succeed, print the error message
        if(!result)
        {
            int logLength;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
            std::vector<GLchar> shader_err((logLength > 1) ? logLength : 1);
            glGetShaderInfoLog(shader, logLength, NULL, &shader_err[0]);

            std::cout << &shader_err[0] << std::endl;
        }

        return shader;
    }

    // create shaders, link them together, return the linked program
    GLuint loadShadersVGF(const char* path)
    {
        // retrieve file paths
        std::string shader_dir = FileIO::getPlatformPath(path);
        std::cout << "Checking VGF shader program '"
                  << shader_dir << "'" << std::endl;

        std::string vertex (shader_dir + "vertex.shd");
        std::string geometry (shader_dir + "geometry.shd");
        std::string fragment (shader_dir + "fragment.shd");

        // retrieve all shaders
        GLuint shd_vertex = loadShader(GL_VERTEX_SHADER, vertex.c_str());
        GLuint shd_geometry = loadShader(GL_GEOMETRY_SHADER, geometry.c_str());
        GLuint shd_fragment = loadShader(GL_FRAGMENT_SHADER, fragment.c_str());

        // link the shaders together
        std::cout << "linking shader program" << std::endl;
        GLuint program = glCreateProgram();
        glAttachShader(program, shd_vertex);
        glAttachShader(program, shd_geometry);
        glAttachShader(program, shd_fragment);
        glLinkProgram(program);

        // check if linking was successful
        GLint result = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &result);

        // if linking did not succeed, print the error message
        if(!result)
        {
            int logLength;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
            std::vector<GLchar> programError( (logLength > 1) ? logLength : 1 );
            glGetProgramInfoLog(program, logLength, NULL, &programError[0]);
            std::cout << &programError[0] << std::endl;
        }

        // perform cleanup
        glDeleteShader(shd_vertex);
        glDeleteShader(shd_geometry);
        glDeleteShader(shd_fragment);

        return program;
    }

    // attach a vertex and a fragment shader, and link them together in a program
    GLuint loadShadersVF(const char* path)
    {
        // retrieve file paths
        std::string shader_dir = FileIO::getPlatformPath(path);
        std::cout << "Checking VF shader program '"
                  << shader_dir << "'" << std::endl;

        std::string vertex (shader_dir + "vertex.shd");
        std::string fragment (shader_dir + "fragment.shd");

        // retrieve all shaders
        GLuint shd_vertex = loadShader(GL_VERTEX_SHADER, vertex.c_str());
        GLuint shd_fragment = loadShader(GL_FRAGMENT_SHADER, fragment.c_str());

        // link the shaders together
        std::cout << "linking shader program" << std::endl;
        GLuint program = glCreateProgram();
        glAttachShader(program, shd_vertex);
        glAttachShader(program, shd_fragment);
        glLinkProgram(program);

        // check if linking was successful
        GLint result = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &result);

        // if linking did not succeed, print the error message
        if(!result)
        {
            int logLength;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
            std::vector<GLchar> programError( (logLength > 1) ? logLength : 1 );
            glGetProgramInfoLog(program, logLength, NULL, &programError[0]);
            std::cout << &programError[0] << std::endl;
        }

        // perform cleanup
        glDeleteShader(shd_vertex);
        glDeleteShader(shd_fragment);

        return program;
    }

    // get location for a uniform variable for a given shader program
    GLuint getUniformLocation(GLuint shader_program, const GLchar* uniform_name)
    {
        GLint result = glGetUniformLocation(shader_program, uniform_name);
        if(result < 0)
        {
            std::cout << "Could not find uniform variable '"
                      << uniform_name << "'" << std::endl;
        }
        return result;
    }

    // type of shader, either with or without geometry shader
    typedef enum {
        SHADERS_VF,
        SHADERS_VGF
    } _shaders_t;

    // wrapper class for the functions above
    class ShaderWrapper {
    private:
        GLuint _shader;
    protected:
    public:
        ShaderWrapper(const char* path, _shaders_t type)
        {
            switch(type) {
            case SHADERS_VF:
                _shader = loadShadersVF(path);
                break;
            case SHADERS_VGF:
                _shader = loadShadersVGF(path);
                break;
            default:
                std::cerr << "ShaderWrapper::ctor(): Unrecognized shader type"
                          << std::endl;
                break;
            }
        }
        ~ShaderWrapper()
        {
            glDeleteShader(_shader); // is this needed ?
            glUseProgram(0);
        }

        void Activate()
        {
            glUseProgram(_shader);
        }
        void Deactivate()
        {
            glUseProgram(0);
        }

        // the 'number' is an integer between 0 and
        // GL_MAX_TEXTURE_UNITS (probably 16)
        void SetUniformTexture(const char* name, GLuint number)
        {
            if(!(number < GL_MAX_TEXTURE_UNITS))
            {
                std::cerr << "ShaderWrapper::SetUniformTexture: "
                          << "Invalid texture number (" << number
                          << ")" << std::endl;
            }

            GLint location = glGetUniformLocation(_shader, name);
            glUniform1i(location, number);

            checkUniformVariable(location, name, UNIFORM_TEXTURE);
        }

        void SetUniform(const char* name, const glm::mat4* mat)
        {
            GLint location = glGetUniformLocation(_shader, name);
            glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(*mat));

            checkUniformVariable(location, name, UNIFORM_MAT4);
        }

        // vector is already allocated
        void SetUniform(const char* name, const glm::vec2* vec)
        {
            GLint location = glGetUniformLocation(_shader, name);
            glUniform2fv(location, 1, glm::value_ptr(*vec));

            checkUniformVariable(location, name, UNIFORM_VEC2);
        }

        // vector can be called by value, i.e. not allocated prior to
        // calling this function
        void SetUniform(const char* name, const glm::vec2 &vec)
        {
            GLint location = glGetUniformLocation(_shader, name);
            glUniform2fv(location, 1, glm::value_ptr(vec));

            checkUniformVariable(location, name, UNIFORM_VEC2);
        }

        // vector is already allocated
        void SetUniform(const char* name, const glm::vec3* vec)
        {
            GLint location = glGetUniformLocation(_shader, name);
            glUniform3fv(location, 1, glm::value_ptr(*vec));

            checkUniformVariable(location, name, UNIFORM_VEC3);
        }

        // vector can be called by value, i.e. not allocated prior to
        // calling this function
        void SetUniform(const char* name, const glm::vec3 &vec)
        {
            GLint location = glGetUniformLocation(_shader, name);
            glUniform3fv(location, 1, glm::value_ptr(vec));

            checkUniformVariable(location, name, UNIFORM_VEC3);
        }

		// vector is already allocated
        void SetUniform(const char* name, const glm::vec4* vec)
        {
			printf("SetUniform(const char* name, const glm::vec4* vec)\n");
            GLint location = glGetUniformLocation(_shader, name);
            glUniform4fv(location, 1, glm::value_ptr(*vec));

            checkUniformVariable(location, name, UNIFORM_VEC4);
        }

        // vector can be called by value, i.e. not allocated prior to
        // calling this function
        void SetUniform(const char* name, const glm::vec4 &vec)
        {
			printf("SetUniform(const char* name, const glm::vec4 &vec)\n");
            GLint location = glGetUniformLocation(_shader, name);
            glUniform4fv(location, 1, glm::value_ptr(vec));

            checkUniformVariable(location, name, UNIFORM_VEC4);
        }

        void SetUniform(const char* name, bool b)
        {
            GLint location = glGetUniformLocation(_shader, name);
            glUniform1ui(location, b);

            checkUniformVariable(location, name, UNIFORM_BOOL);
        }

        void SetUniform(const char* name, float f)
        {
            GLint location = glGetUniformLocation(_shader, name);
            glUniform1f(location, f);

            checkUniformVariable(location, name, UNIFORM_FLOAT);
        }

        void SetUniform(const char* name, int i)
        {
            GLint location = glGetUniformLocation(_shader, name);
            glUniform1i(location, i);

            checkUniformVariable(location, name, UNIFORM_INT);
        }

        void SetUniform(const char* name, unsigned int i)
        {
            GLint location = glGetUniformLocation(_shader, name);
            glUniform1ui(location, i);

            checkUniformVariable(location, name, UNIFORM_UINT);
        }
    };
}
