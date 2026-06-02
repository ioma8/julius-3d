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

    (void) viewport_width;
    (void) viewport_height;
    (void) yaw_degrees;

    int local_x = (int) (((float) (screen_x - viewport_x)) / zoom);
    int local_y = (int) (((float) (screen_y - viewport_y)) / zoom);

    int odd = (local_x / 30 + local_y / 15) & 1;
    int x_is_odd = (local_x / 30) & 1;
    int y_is_odd = (local_y / 15) & 1;
    int x_mod = (local_x % 30) / 2;
    int y_mod = local_y % 15;
    int x_view_offset = local_x / 60;
    int y_view_offset = local_y / 15;

    if (odd) {
        if (x_mod + y_mod >= 14) {
            y_view_offset++;
            if (x_is_odd && !y_is_odd) {
                x_view_offset++;
            }
        }
    } else {
        if (y_mod > x_mod) {
            y_view_offset++;
        } else if (x_is_odd && y_is_odd) {
            x_view_offset++;
        }
    }

    *tile_x = x_view_offset;
    *tile_y = y_view_offset;
    return 1;
}
