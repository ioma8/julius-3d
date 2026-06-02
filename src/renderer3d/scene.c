#include "renderer3d/scene.h"

#include "city/view.h"
#include "map/grid.h"
#include "map/terrain.h"

static renderer3d_scene *active_scene;

static void add_tile(int x, int y, int grid_offset)
{
    if (!active_scene || active_scene->terrain_count >= RENDERER3D_MAX_TERRAIN_ITEMS) {
        return;
    }
    renderer3d_terrain_item *item = &active_scene->terrain[active_scene->terrain_count++];
    item->grid_offset = grid_offset;
    item->x = x;
    item->y = y;
    item->terrain = map_terrain_get(grid_offset);
}

void renderer3d_scene_build(renderer3d_scene *scene)
{
    scene->terrain_count = 0;
    active_scene = scene;
    city_view_foreach_valid_map_tile(add_tile);
    active_scene = 0;
}
