#include "utilities/Canvas.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <array>

Canvas::Canvas() {
    static constexpr std::array<float, 4 * 5> vertices{
      -1.0F,-1.0F,0.0F,1.0F,0.0F,
      1.0F,-1.0F,0.0F,0.0F,0.0F,
      1.0F,1.0F,0.0F,0.0F,1.0F,
      -1.0F,1.0F,0.0F,1.0F,1.0F,
    };
    static constexpr std::array<int, 6> indices{
      0,1,2,// first triangle
      0,2,3,// second triangle
    };

    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void *>(0));
    glEnableVertexAttribArray(0);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), static_cast<void *>(0));
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(
      1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}
void Canvas::draw() const {
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
