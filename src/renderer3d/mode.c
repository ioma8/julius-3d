#include "renderer3d/mode.h"

#include "core/config.h"

static renderer3d_camera camera = {
    0,
    45.0f,
    55.0f,
    1.0f
};

static float clamp_float(float value, float min, float max)
{
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}

void renderer3d_mode_init(void)
{
    camera.enabled = config_get(CONFIG_UI_3D_VIEW) ? 1 : 0;
    renderer3d_camera_reset();
}

int renderer3d_mode_is_enabled(void)
{
    return camera.enabled;
}

void renderer3d_mode_set_enabled(int enabled)
{
    camera.enabled = enabled ? 1 : 0;
    config_set(CONFIG_UI_3D_VIEW, camera.enabled);
}

void renderer3d_mode_toggle(void)
{
    renderer3d_mode_set_enabled(!camera.enabled);
}

const renderer3d_camera *renderer3d_camera_get(void)
{
    return &camera;
}

void renderer3d_camera_rotate_yaw(float delta_degrees)
{
    camera.yaw_degrees += delta_degrees;
    while (camera.yaw_degrees >= 360.0f) {
        camera.yaw_degrees -= 360.0f;
    }
    while (camera.yaw_degrees < 0.0f) {
        camera.yaw_degrees += 360.0f;
    }
}

void renderer3d_camera_zoom(float delta)
{
    camera.zoom = clamp_float(camera.zoom + delta, 0.5f, 4.0f);
}

void renderer3d_camera_reset(void)
{
    camera.yaw_degrees = 45.0f;
    camera.pitch_degrees = 55.0f;
    camera.zoom = 1.0f;
}
