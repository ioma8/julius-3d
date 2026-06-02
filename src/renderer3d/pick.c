#include "renderer3d/pick.h"

#include "city/view.h"
#include "map/grid.h"
#include "renderer3d/math.h"
#include "renderer3d/mode.h"

int renderer3d_pick_tile(int screen_x, int screen_y, map_tile *tile)
{
    int vx, vy, vw, vh;
    int picked_x;
    int picked_y;
    city_view_get_viewport(&vx, &vy, &vw, &vh);

    const renderer3d_camera *camera = renderer3d_camera_get();
    if (!renderer3d_screen_to_tile(screen_x, screen_y, vx, vy, vw, vh,
            camera->yaw_degrees, camera->zoom, &picked_x, &picked_y)) {
        tile->grid_offset = tile->x = tile->y = 0;
        return 0;
    }

    if (!map_grid_is_inside(picked_x, picked_y, 1)) {
        tile->grid_offset = tile->x = tile->y = 0;
        return 0;
    }

    tile->x = picked_x;
    tile->y = picked_y;
    tile->grid_offset = map_grid_offset(picked_x, picked_y);
    return tile->grid_offset > 0;
}
