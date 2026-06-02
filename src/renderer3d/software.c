#include "renderer3d/software.h"

#include "city/view.h"
#include "graphics/color.h"
#include "graphics/graphics.h"
#include "graphics/screen.h"
#include "renderer3d/mode.h"

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
}
