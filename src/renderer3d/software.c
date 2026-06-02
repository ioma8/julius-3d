#include "renderer3d/software.h"

#include <math.h>

#include "city/view.h"
#include "graphics/color.h"
#include "graphics/graphics.h"
#include "graphics/screen.h"
#include "map/terrain.h"
#include "renderer3d/mode.h"
#include "renderer3d/scene.h"

static void fill_rect_clipped(int x, int y, int width, int height, color_t color)
{
    if (x < 0) {
        width += x;
        x = 0;
    }
    if (y < 0) {
        height += y;
        y = 0;
    }
    if (x + width > screen_width()) {
        width = screen_width() - x;
    }
    if (y + height > screen_height()) {
        height = screen_height() - y;
    }
    if (width <= 0 || height <= 0) {
        return;
    }
    graphics_fill_rect(x, y, width, height, color);
}

static color_t terrain_color(int terrain)
{
    if (terrain & TERRAIN_WATER) {
        return 0xff2d5f8a;
    }
    if (terrain & TERRAIN_ROAD) {
        return 0xff303030;
    }
    if (terrain & TERRAIN_MEADOW) {
        return 0xff476c3a;
    }
    if (terrain & TERRAIN_TREE) {
        return 0xff315a32;
    }
    return 0xff5f6548;
}

static color_t building_color(int type)
{
    (void) type;
    return 0xff9b7a4a;
}

static void project_tile(int tile_x, int tile_y, const renderer3d_camera *camera, int *screen_x, int *screen_y)
{
    float yaw = camera->yaw_degrees * 0.01745329252f;
    float scale = 10.0f * camera->zoom;
    float cx = (float) tile_x - 32.0f;
    float cy = (float) tile_y - 32.0f;
    float rx = cx * cosf(yaw) - cy * sinf(yaw);
    float ry = cx * sinf(yaw) + cy * cosf(yaw);
    *screen_x = (int) (rx * scale);
    *screen_y = (int) (ry * scale * 0.55f);
}

void renderer3d_software_draw_viewport(void)
{
    int vx, vy, vw, vh;
    city_view_get_viewport(&vx, &vy, &vw, &vh);
    const renderer3d_camera *camera = renderer3d_camera_get();

    fill_rect_clipped(vx, vy, vw, vh, 0xff182028);

    renderer3d_scene scene;
    renderer3d_scene_build(&scene);

    for (int i = 0; i < scene.terrain_count; i++) {
        const renderer3d_terrain_item *item = &scene.terrain[i];
        int px, py;
        project_tile(item->x, item->y, camera, &px, &py);
        int sx = vx + vw / 2 + px;
        int sy = vy + vh / 2 + py;
        fill_rect_clipped(sx, sy, 10, 6, terrain_color(item->terrain));
    }

    for (int i = 0; i < scene.building_count; i++) {
        const renderer3d_building_item *item = &scene.buildings[i];
        int px, py;
        project_tile(item->x, item->y, camera, &px, &py);
        int sx = vx + vw / 2 + px;
        int sy = vy + vh / 2 + py - item->height;
        fill_rect_clipped(sx, sy, item->size * 12, item->size * 8 + item->height, building_color(item->type));
        graphics_draw_horizontal_line(sx, sx + item->size * 12, sy, 0xffd0b080);
    }

    if (scene.ghost.active) {
        int px, py;
        project_tile(scene.ghost.x, scene.ghost.y, camera, &px, &py);
        fill_rect_clipped(vx + vw / 2 + px, vy + vh / 2 + py,
            scene.ghost.size_x * 12, scene.ghost.size_y * 8, 0x99ffff00);
    }

    for (int i = 0; i < scene.figure_count; i++) {
        const renderer3d_figure_item *item = &scene.figures[i];
        int px, py;
        project_tile(item->x, item->y, camera, &px, &py);
        fill_rect_clipped(vx + vw / 2 + px, vy + vh / 2 + py - 12, 4, 12, 0xffe8d8b0);
    }
}
