#include "renderer3d/scene.h"

#include "building/building.h"
#include "building/construction.h"
#include "city/view.h"
#include "map/building.h"
#include "map/grid.h"
#include "map/terrain.h"

static renderer3d_scene *active_scene;

static void add_tile(int x, int y, int grid_offset)
{
    if (!active_scene) {
        return;
    }
    if (active_scene->terrain_count < RENDERER3D_MAX_TERRAIN_ITEMS) {
        renderer3d_terrain_item *item = &active_scene->terrain[active_scene->terrain_count++];
        item->grid_offset = grid_offset;
        item->x = x;
        item->y = y;
        item->terrain = map_terrain_get(grid_offset);
    }

    int building_id = map_building_at(grid_offset);
    if (building_id > 0 && active_scene->building_count < RENDERER3D_MAX_BUILDING_ITEMS) {
        const building *b = building_get(building_id);
        if (b->grid_offset == grid_offset) {
            renderer3d_building_item *item = &active_scene->buildings[active_scene->building_count++];
            item->building_id = building_id;
            item->grid_offset = grid_offset;
            item->x = b->x;
            item->y = b->y;
            item->size = b->size;
            item->type = b->type;
            item->height = b->size * 3;
            if (building_is_house(b->type)) {
                item->height += b->subtype.house_level;
            }
        }
    }
}

void renderer3d_scene_build(renderer3d_scene *scene)
{
    scene->terrain_count = 0;
    scene->building_count = 0;
    active_scene = scene;
    city_view_foreach_valid_map_tile(add_tile);
    active_scene = 0;

    scene->ghost.active = 0;
    int size_x = 0;
    int size_y = 0;
    if (building_construction_type() && building_construction_size(&size_x, &size_y)) {
        scene->ghost.active = 1;
        scene->ghost.size_x = size_x;
        scene->ghost.size_y = size_y;
        int grid_offset = building_construction_get_start_grid_offset();
        if (grid_offset > 0) {
            scene->ghost.x = map_grid_offset_to_x(grid_offset);
            scene->ghost.y = map_grid_offset_to_y(grid_offset);
        }
    }
}
