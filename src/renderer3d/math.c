#include "renderer3d/math.h"

#include <math.h>

int renderer3d_screen_to_tile(
    int screen_x, int screen_y,
    int viewport_x, int viewport_y,
    int viewport_width, int viewport_height,
    float yaw_degrees, float zoom,
    int *tile_x, int *tile_y)
{
    if (screen_x < viewport_x || screen_x >= viewport_x + viewport_width ||
            screen_y < viewport_y || screen_y >= viewport_y + viewport_height ||
            zoom <= 0.0f) {
        return 0;
    }

    float yaw = -yaw_degrees * 0.01745329252f;
    float scale = 10.0f * zoom;
    float rx = ((float) screen_x - (float) viewport_x - (float) viewport_width / 2.0f) / scale;
    float ry = ((float) screen_y - (float) viewport_y - (float) viewport_height / 2.0f) / (scale * 0.55f);

    float tx = rx * cosf(yaw) - ry * sinf(yaw);
    float ty = rx * sinf(yaw) + ry * cosf(yaw);

    *tile_x = (int) floorf(tx + 32.0f + 0.5f);
    *tile_y = (int) floorf(ty + 32.0f + 0.5f);
    return 1;
}
