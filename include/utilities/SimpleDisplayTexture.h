#ifndef CHIP8_SIMPLEDISPLAYTEXTURE_H
#define CHIP8_SIMPLEDISPLAYTEXTURE_H

#include <GL/glew.h>
#include "chip8/Chip8.h"


class SimpleDisplayTexture {
    using Chip8Texture = std::array<uint8_t, chip8::Chip8::screen_size>;

public:
    SimpleDisplayTexture() {
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    void load_texture(const chip8::Chip8 &chip8) const {
        static constexpr int texture_width = chip8::Chip8::screen_width;
        static constexpr int texture_height = chip8::Chip8::screen_height;
        Chip8Texture texture_data = chip8.get_screen();

        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
                GL_TEXTURE_2D, 0, GL_RED, texture_width, texture_height,
                0, GL_RED, GL_UNSIGNED_BYTE, texture_data.data());
//        glBindTexture(GL_TEXTURE_2D, 0);
    }

    [[nodiscard]] GLuint get_texture() const { return texture; }

private:
    GLuint texture{};
};


class FrameBuffer {
public:
    FrameBuffer(const int width, const int height) {
        glGenFramebuffers(1, &fbo);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        // frame buffer fbo_texture
        glGenTextures(1, &fbo_texture);
        glBindTexture(GL_TEXTURE_2D, fbo_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glBindTexture(GL_TEXTURE_2D, 0);

        // attach it to currently bound framebuffer object
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_texture, 0);

        // test if it works
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            spdlog::error("Framebuffer not complete");
        }
        // bind default
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    ~FrameBuffer() {
        glDeleteFramebuffers(1, &fbo);
    }
    FrameBuffer(FrameBuffer &) = delete;
    FrameBuffer(FrameBuffer &&) = delete;
    FrameBuffer operator=(FrameBuffer &) = delete;
    FrameBuffer operator=(FrameBuffer &&) = delete;

    void bind_buffer() const {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    }

    static void unbind_buffer() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    [[nodiscard]] GLuint get_fbo_texture() const { return fbo_texture; }


private:
    GLuint fbo{};
    GLuint fbo_texture{};
};

#endif //CHIP8_SIMPLEDISPLAYTEXTURE_H
