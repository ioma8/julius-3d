#include "renderer3d/pick.h"

#include <math.h>

#include "city/view.h"
#include "map/grid.h"
#include "renderer3d/mode.h"

#define DEGREES_TO_RADIANS 0.01745329252f
#define CAMERA_YAW_BASE_OFFSET 45.0f

static void apply_inverse_view_transform(int screen_x, int screen_y, int *view_x, int *view_y)
{
    int viewport_x;
    int viewport_y;
    int viewport_width;
    int viewport_height;
    city_view_get_viewport(&viewport_x, &viewport_y, &viewport_width, &viewport_height);

    int center_x = viewport_x + viewport_width / 2;
    int center_y = viewport_y + viewport_height / 2;
    const renderer3d_camera *camera = renderer3d_camera_get();
    float zoom = camera->zoom;
    float pitch = camera->pitch_degrees * DEGREES_TO_RADIANS;
    if (pitch < 0.12f) {
        pitch = 0.12f;
    }
    if (pitch > 1.22f) {
        pitch = 1.22f;
    }
    float pitch_cos = cosf(pitch);
    if (pitch_cos <= 0.0f) {
        pitch_cos = 0.01f;
    }
    if (zoom <= 0.0f) {
        zoom = 1.0f;
    }

    float dx = (float) screen_x - (float) center_x;
    float dy = (float) screen_y - (float) center_y;
    dx /= zoom;
    dy /= zoom * pitch_cos;

    float yaw = (camera->yaw_degrees - CAMERA_YAW_BASE_OFFSET) * DEGREES_TO_RADIANS;
    float sin_yaw = sinf(yaw);
    float cos_yaw = cosf(yaw);
    float unrot_x = dx * cos_yaw + dy * sin_yaw;
    float unrot_y = -dx * sin_yaw + dy * cos_yaw;

    *view_x = (int) (unrot_x + center_x);
    *view_y = (int) (unrot_y + center_y);
}

int renderer3d_pick_tile(int screen_x, int screen_y, map_tile *tile)
{
    int picked_x;
    int picked_y;
    apply_inverse_view_transform(screen_x, screen_y, &picked_x, &picked_y);

    view_tile view;
    if (city_view_pixels_to_view_tile(picked_x, picked_y, &view)) {
        tile->grid_offset = city_view_tile_to_grid_offset(&view);
        city_view_set_selected_view_tile(&view);
        tile->x = map_grid_offset_to_x(tile->grid_offset);
        tile->y = map_grid_offset_to_y(tile->grid_offset);
        return tile->grid_offset > 0;
    }
    tile->grid_offset = tile->x = tile->y = 0;
    return 0;
}
