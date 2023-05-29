#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct GLFWwindow;

namespace g3d {
/* Forward Declarations */
    struct Window;
    struct Framebuffer;
    struct Texture;
    struct RenderState;
    struct Renderer;

/* Typedefs */
    typedef uint32_t HandleVao;
    typedef uint32_t HandleBuffer;
    typedef uint32_t HandleTexture;
    typedef uint32_t HandleFramebuffer;
    typedef uint32_t HandleShader;
    typedef uint32_t HandleProgram;
    //typedef uint32_t ;
    //typedef uint32_t HandleFramebuffer;

/* Definitions */
    struct Window {
        explicit Window();
        explicit Window(const char* title, uint32_t width, uint32_t height);

        bool has_just_resized{false};
        GLFWwindow *pglfw_window{nullptr};
        uint32_t width{0}, height{0};
        char title[64];
    };
    struct Framebuffer {
        uint32_t handle{0};
        std::vector<std::pair<uint32_t, Texture*>> textures;
    };
    struct Texture {
        uint32_t handle{0};
        uint32_t target{0}, format{0};
        uint32_t width{0}, height{0};
    };
    struct RenderState {
        HandleVao vao;
        HandleProgram program;
        float line_width{1.0};
    };
    struct Renderer {
        void render();

        private:
    };
} // namespace g3d