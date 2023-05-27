#include "orbital_camera.hpp"
            
#include <iostream>

#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <glm/gtc/matrix_transform.hpp>

#include <renderer.hpp>

namespace g3d {
    OrbitalCamera::OrbitalCamera(Window *window, OrbitalCameraSettings settings)
        : settings{settings} {

        _calculate_projection_matrix(window);
    }

    void OrbitalCamera::update() {
        if (_wheel_locked == false) {
            _distance += ImGui::GetIO().MouseWheel;
            _distance = glm::clamp(_distance, 1.0f, 1000.0f);
        }
        if (_mouse_locked) { return; }

        auto [x, y] = ImGui::GetMouseDragDelta(0, 1.0);
        ImGui::ResetMouseDragDelta();
        x *= 0.5f;
        y *= 0.5f;

        _theta -= y;
        _phi -= x;
        _theta = glm::clamp(_theta, 0.01f, 179.9f);
    }

    glm::mat4 OrbitalCamera::view_matrix() const {
        glm::vec3 position;

        position.x = _distance * glm::sin(glm::radians(_theta)) * glm::cos(glm::radians(_phi));
        position.y = _distance * glm::cos(glm::radians(_theta));
        position.z = _distance * glm::sin(glm::radians(_theta)) * glm::sin(glm::radians(_phi));

        return glm::lookAt(position, glm::normalize(-position), glm::vec3{0, -1, 0});
    }

    glm::mat4 OrbitalCamera::projection_matrix() const { return _projection_matrix; }

    void OrbitalCamera::on_window_resize(uint32_t width, uint32_t height) {
        _projection_matrix = glm::perspectiveFov(glm::radians(settings.fovydeg),
                                                 static_cast<float>(width),
                                                 static_cast<float>(height),
                                                 settings.near,
                                                 settings.far);
    }

    void OrbitalCamera::_calculate_projection_matrix(Window *window) {
        _projection_matrix = glm::perspectiveFov(glm::radians(settings.fovydeg),
                                                 static_cast<float>(window->width),
                                                 static_cast<float>(window->height),
                                                 settings.near,
                                                 settings.far);
    }
} // namespace g3d