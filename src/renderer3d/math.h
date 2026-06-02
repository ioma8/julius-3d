#ifndef RENDERER3D_MATH_H
#define RENDERER3D_MATH_H

typedef struct {
    float x;
    float y;
} renderer3d_vec2;

int renderer3d_screen_to_tile(
    int screen_x, int screen_y,
    int viewport_x, int viewport_y,
    int viewport_width, int viewport_height,
    float yaw_degrees, float zoom,
    int *tile_x, int *tile_y);

#endif // RENDERER3D_MATH_H
