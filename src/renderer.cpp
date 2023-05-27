#include "renderer.hpp"

#include <cstring>
#include <cstdlib>

namespace g3d {

    Window::Window() { memset(title, 0, 64); }
    Window::Window(const char* title, uint32_t width, uint32_t height) {
        this->width = width;
        this->height = height;
        
        memset(this->title, 0, 64);
        size_t len = strlen(title);
        len = std::min(len, 63llu);
        memcpy(this->title, title, len);
    }

}