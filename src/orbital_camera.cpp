#include "orbital_camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <imgui/imgui.h>
#include <iostream>
#include <GLFW/glfw3.h>

// extern OrbitalCamera *orb_cam;
// extern float imgui_mouse_wheel;

// OrbitalCamera::OrbitalCamera(eng::Window *window, OrbitalCameraSettings settings)
//     : settings{settings} {

//     _calculate_projection_matrix(window);
//     window->on_resize.connect(
//         [this, window](uint32_t w, uint32_t h) { this->_calculate_projection_matrix(window); });
// }

// void OrbitalCamera::update() {
//     if (_wheel_locked == false) {
//         _distance += imgui_mouse_wheel;
//         _distance = glm::clamp(_distance, 1.0f, 1000.0f);
//     }
//     if (_mouse_locked) { return; }

//     auto [x, y] = ImGui::GetMouseDragDelta(0, 1.0);
//     ImGui::ResetMouseDragDelta();
//     x *= 0.5f;
//     y *= 0.5f;

//     _theta -= y;
//     _phi -= x;
//     _theta = glm::clamp(_theta, 0.01f, 179.9f);
// }

// glm::mat4 OrbitalCamera::view_matrix() const {
//     glm::vec3 position;

//     position.x = _distance * glm::sin(glm::radians(_theta)) * glm::cos(glm::radians(_phi));
//     position.y = _distance * glm::cos(glm::radians(_theta));
//     position.z = _distance * glm::sin(glm::radians(_theta)) * glm::sin(glm::radians(_phi));

//     return glm::lookAt(position, glm::normalize(-position), glm::vec3{0, -1, 0});
// }

// glm::mat4 OrbitalCamera::projection_matrix() const { return _projection_matrix; }

// void OrbitalCamera::_calculate_projection_matrix(eng::Window *window) {
//     _projection_matrix = glm::perspectiveFov(glm::radians(settings.fovydeg),
//                                              static_cast<float>(window->width()),
//                                              static_cast<float>(window->height()),
//                                              settings.near,
//                                              settings.far);
// }
