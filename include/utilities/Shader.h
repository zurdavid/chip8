#ifndef GL_LION_SHADER_H
#define GL_LION_SHADER_H

#include <string>
#include <glm/glm.hpp>

class Shader {
public:
    Shader(const char* vertexPath, const char* fragmentPath);
    [[nodiscard]] unsigned int Id() const;
    void use() const;

    void setBool(const std::string &name, bool value) const;
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
    void setMatrix4(const std::string &name, const glm::mat4* transformation) const;
    void setVec3(const std::string &name, const float v1, const float v2, const float v3) const;
    void setVec3(const std::string &name, const glm::vec3 value) const;

private:
    unsigned int _id;
};


#endif //GL_LION_SHADER_H
