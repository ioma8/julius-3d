#include "renderer3d/software.h"

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

void renderer3d_software_draw_viewport(void)
{
    int vx, vy, vw, vh;
    city_view_get_viewport(&vx, &vy, &vw, &vh);
    const renderer3d_camera *camera = renderer3d_camera_get();

    fill_rect_clipped(vx, vy, vw, vh, 0xff182028);

    int spacing = (int) (24.0f * camera->zoom);
    if (spacing < 8) {
        spacing = 8;
    }
    int yaw_shift = ((int) camera->yaw_degrees) % spacing;

    for (int y = vy + yaw_shift; y < vy + vh; y += spacing) {
        graphics_draw_horizontal_line(vx, vx + vw - 1, y, 0xff405060);
    }
    for (int x = vx + spacing - yaw_shift; x < vx + vw; x += spacing) {
        graphics_draw_vertical_line(x, vy, vy + vh - 1, 0xff405060);
    }

    fill_rect_clipped(vx + 8, vy + 8, 180, 18, 0xff202830);

    renderer3d_scene scene;
    renderer3d_scene_build(&scene);

    for (int i = 0; i < scene.terrain_count; i++) {
        const renderer3d_terrain_item *item = &scene.terrain[i];
        int sx = vx + (item->x % 64) * 12;
        int sy = vy + (item->y % 64) * 8;
        fill_rect_clipped(sx, sy, 10, 6, terrain_color(item->terrain));
    }
}
