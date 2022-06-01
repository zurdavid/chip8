#include <fstream>

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <spdlog/spdlog.h>

#include "utilities/Shader.h"

std::string getShaderSource(const char *shaderPath) {
    std::stringstream ss;

    std::ifstream file;
    // turn on exceptions for ifstream
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        file.open(shaderPath);
        ss << file.rdbuf();
    } catch (std::system_error &e) {
        spdlog::error("Could not open file {} [ {} ]", shaderPath, e.code().message());
    };
    std::string source(ss.str());
    return source;
}


unsigned int createVertexShader(const char* vertexShaderSource) {
    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        spdlog::error("ERROR::SHADER::VERTEX::LINKING_FAILED\n {}", infoLog);
    }
    return vertexShader;
}


unsigned int createFragmentShader(const char* fragmentShaderSource) {
    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    int success;
    char infoLog[512];
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        spdlog::error("ERROR::SHADER::FRAGMENT::LINKING_FAILED\n {}", infoLog);
    }
    return fragmentShader;
}


unsigned int createProgram(unsigned int vertexShader, unsigned int fragmentShader) {
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        spdlog::error("ERROR::SHADER::PROGRAM::LINKING_FAILED\n {}", infoLog);
    }
    return shaderProgram;
}

Shader::Shader(const char *vertexPath, const char *fragmentPath) {
    const std::string vertexSource = getShaderSource(vertexPath);
    unsigned int vertexShader = createVertexShader(vertexSource.c_str());
    const std::string fragmentSource = getShaderSource(fragmentPath);
    unsigned int fragmentShader = createFragmentShader(fragmentSource.c_str());

    _id = createProgram(vertexShader, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

unsigned int Shader::Id() const {
    return _id;
}

void Shader::use() const {
    glUseProgram(_id);
}

void Shader::setBool(const std::string &name, bool value) const {
    glUniform1i(glGetUniformLocation(_id, name.c_str()), static_cast<int>(value));
}

void Shader::setInt(const std::string &name, int value) const {
    glUniform1i(glGetUniformLocation(_id, name.c_str()), value);
}

void Shader::setFloat(const std::string &name, float value) const {
    glUniform1f(glGetUniformLocation(_id, name.c_str()), value);
}

void Shader::setMatrix4(const std::string &name, const glm::mat4* transformation) const {
    glUniformMatrix4fv(glGetUniformLocation(_id, name.c_str()), 1, GL_FALSE, glm::value_ptr(*transformation));
}

void Shader::setVec3(const std::string &name, const float v1, const float v2, const float v3) const {
    glUniform3fv(glGetUniformLocation(_id, name.c_str()), 1, glm::value_ptr(glm::vec3(v1, v2, v3)));
}

void Shader::setVec3(const std::string &name, const glm::vec3 value) const {
    glUniform3fv(glGetUniformLocation(_id, name.c_str()), 1, glm::value_ptr(value));
}
