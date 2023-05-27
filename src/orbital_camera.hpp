#pragma once

#include <glm/glm.hpp>

namespace g3d {
    struct Window;
    
    struct OrbitalCameraSettings {
        explicit OrbitalCameraSettings() = default;
        explicit OrbitalCameraSettings(float fovydeg) : fovydeg{fovydeg} {}
        explicit OrbitalCameraSettings(float fovydeg, float near, float far)
            : fovydeg{fovydeg}, near{near}, far{far} {}

        float fovydeg{90.0f};
        float near{0.01f}, far{100.0f};
    };

    class OrbitalCamera {
      public:
        explicit OrbitalCamera() = default;
        explicit OrbitalCamera(Window *window, OrbitalCameraSettings settings);

        void update();
        void on_window_resize(uint32_t width, uint32_t height);
        void set_mouse_state(bool locked = false) { _mouse_locked = locked; }
        void set_wheel_state(bool locked = false) { _wheel_locked = locked; }
        glm::mat4 view_matrix() const;
        glm::mat4 projection_matrix() const;

        OrbitalCameraSettings settings;

      private:
        void _calculate_projection_matrix(Window *);

        bool _mouse_locked = false, _wheel_locked = false;
        float _distance{2.0f}, _theta{1.0f}, _phi{0.0f};
        glm::mat4 _projection_matrix;
    };
} // namespace g3d
