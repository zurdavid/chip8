#ifndef CHIP8_CANVAS_H
#define CHIP8_CANVAS_H

class Canvas {
  public:
    Canvas();
    void draw() const;

  private:
    unsigned int VBO;
    unsigned int EBO;
    unsigned int VAO;
};

#endif// CHIP8_CANVAS_H
