#pragma once
#include <cstdint>
#include <string>

struct GLFWwindow;

namespace g3d {
/* Forward Declarations */
    struct Window;

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

        GLFWwindow *pglfw_window{nullptr};
        uint32_t width{0}, height{0};
        char title[64];
    };
    struct RenderState {
        HandleVao vao;
        HandleProgram program;
    };
    struct Renderer {
        void render();

        private:
    };

    enum class RenderFlag {
        CULL_FACE  = 1 << 0,
        DEPTH_TEST = 1 << 1,
    };
} // namespace g3d