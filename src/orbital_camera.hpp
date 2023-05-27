#pragma once

// #include <glm/glm.hpp>

// struct GLFWwindow;

// namespace eng {
//     struct Window;
// }

// struct OrbitalCameraSettings {
//     explicit OrbitalCameraSettings(float fovydeg) : fovydeg{fovydeg} {}
//     explicit OrbitalCameraSettings(float fovydeg, float near, float far)
//         : fovydeg{fovydeg}, near{near}, far{far} {}

//     float fovydeg{90.0f};
//     float near{0.01f}, far{100.0f};
// };

// class OrbitalCamera {
//   public:
//     explicit OrbitalCamera(eng::Window *window, OrbitalCameraSettings settings);

//     void update();
//     void set_mouse_state(bool locked = false) {_mouse_locked = locked;}
//     void set_wheel_state(bool locked = false) {_wheel_locked = locked;}
//     glm::mat4 view_matrix() const;
//     glm::mat4 projection_matrix() const;

//     OrbitalCameraSettings settings;

//   private:
//     void _calculate_projection_matrix(eng::Window *);

//     bool _mouse_locked = false, _wheel_locked = false;
//     float _distance{2.0f}, _theta{1.0f}, _phi{0.0f};
//     glm::mat4 _projection_matrix;

//     friend void scroll_callback(GLFWwindow *, double, double);
// };
