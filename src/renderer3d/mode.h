#ifndef RENDERER3D_MODE_H
#define RENDERER3D_MODE_H

typedef struct {
    int enabled;
    float yaw_degrees;
    float pitch_degrees;
    float zoom;
} renderer3d_camera;

void renderer3d_mode_init(void);
int renderer3d_mode_is_enabled(void);
void renderer3d_mode_set_enabled(int enabled);
void renderer3d_mode_toggle(void);

const renderer3d_camera *renderer3d_camera_get(void);
void renderer3d_camera_rotate_yaw(float delta_degrees);
void renderer3d_camera_zoom(float delta);
void renderer3d_camera_reset(void);

#endif // RENDERER3D_MODE_H
