#include "renderer3d/software.h"

#include <math.h>

#include "building/building.h"
#include "building/construction.h"
#include "city/view.h"
#include "figure/figure.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "map/building.h"
#include "map/grid.h"
#include "map/figure.h"
#include "map/image.h"
#include "map/property.h"
#include "renderer3d/mode.h"
#include "widget/city_building_ghost.h"
#include "widget/city_figure.h"

#define TILE_WIDTH 60
#define TILE_HEIGHT 30
#define MAX_PRISM_SIZE 5
#define DEGREES_TO_RADIANS 0.01745329252f
#define CAMERA_YAW_BASE_OFFSET 45.0f

static const map_tile *selected_tile_for_draw;

static int viewport_x;
static int viewport_y;
static int viewport_width;
static int viewport_height;
static int view_center_x;
static int view_center_y;
static float view_cos = 1.0f;
static float view_sin = 0.0f;
static float view_zoom = 1.0f;

typedef struct {
    int x;
    int y;
} point2d;

static int clamp_int(int value, int min, int max)
{
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}

static int round_float_to_int(float value)
{
    return (int) ((value >= 0.0f) ? (value + 0.5f) : (value - 0.5f));
}

static void setup_view_transform(void)
{
    city_view_get_viewport(&viewport_x, &viewport_y, &viewport_width, &viewport_height);
    const renderer3d_camera *camera = renderer3d_camera_get();
    float yaw = (camera->yaw_degrees - CAMERA_YAW_BASE_OFFSET) * DEGREES_TO_RADIANS;
    view_cos = cosf(yaw);
    view_sin = sinf(yaw);
    view_zoom = camera->zoom;
    if (view_zoom < 0.35f) {
        view_zoom = 0.35f;
    }
    view_center_x = viewport_x + viewport_width / 2;
    view_center_y = viewport_y + viewport_height / 2;
}

static point2d transform_point(int x, int y)
{
    float dx = (float) x - (float) view_center_x;
    float dy = (float) y - (float) view_center_y;
    dx *= view_zoom;
    dy *= view_zoom;
    point2d p;
    p.x = view_center_x + round_float_to_int(dx * view_cos - dy * view_sin);
    p.y = view_center_y + round_float_to_int(dx * view_sin + dy * view_cos);
    return p;
}

static color_t shade_color(color_t color, int delta)
{
    int r = clamp_int((int) ((color >> 16) & 0xff) + delta, 0, 255);
    int g = clamp_int((int) ((color >> 8) & 0xff) + delta, 0, 255);
    int b = clamp_int((int) (color & 0xff) + delta, 0, 255);
    return (color_t) ((r << 16) | (g << 8) | b);
}

static void draw_line(point2d a, point2d b, color_t color)
{
    int dx = b.x > a.x ? b.x - a.x : a.x - b.x;
    int sx = a.x < b.x ? 1 : -1;
    int dy = b.y > a.y ? a.y - b.y : b.y - a.y;
    int sy = a.y < b.y ? 1 : -1;
    int err = dx + dy;

    for (;;) {
        graphics_draw_horizontal_line(a.x, a.x, a.y, color);
        if (a.x == b.x && a.y == b.y) {
            break;
        }
        int err2 = 2 * err;
        if (err2 >= dy) {
            err += dy;
            a.x += sx;
        }
        if (err2 <= dx) {
            err += dx;
            a.y += sy;
        }
    }
}

static void draw_filled_quad(point2d p0, point2d p1, point2d p2, point2d p3, color_t color)
{
    point2d points[4] = {p0, p1, p2, p3};
    int min_y = p0.y;
    int max_y = p0.y;
    for (int i = 1; i < 4; ++i) {
        if (points[i].y < min_y) {
            min_y = points[i].y;
        }
        if (points[i].y > max_y) {
            max_y = points[i].y;
        }
    }

    for (int y = min_y; y <= max_y; ++y) {
        int intersections[4];
        int count = 0;
        for (int i = 0; i < 4; ++i) {
            point2d a = points[i];
            point2d b = points[(i + 1) % 4];
            if (a.y == b.y) {
                continue;
            }
            if ((y >= a.y && y < b.y) || (y >= b.y && y < a.y)) {
                intersections[count++] = a.x + (y - a.y) * (b.x - a.x) / (b.y - a.y);
            }
        }
        if (count >= 2) {
            int x1 = intersections[0];
            int x2 = intersections[1];
            if (x1 > x2) {
                int tmp = x1;
                x1 = x2;
                x2 = tmp;
            }
            graphics_draw_horizontal_line(x1, x2, y, color);
        }
    }
}

static void fill_viewport_background(void)
{
    int width = 0;
    int height = 0;
    city_view_get_viewport(&viewport_x, &viewport_y, &width, &height);
    graphics_fill_rect(viewport_x, viewport_y, width, height, 0xff20293a);
}

static int building_prism_height(const building *b)
{
    if (!b || !b->id) {
        return 0;
    }
    if (building_is_house((building_type) b->type)) {
        return clamp_int(14 + b->subtype.house_level * 3, 14, 54);
    }

    switch (b->type) {
        case BUILDING_ROAD:
        case BUILDING_PLAZA:
        case BUILDING_GARDENS:
        case BUILDING_LOW_BRIDGE:
        case BUILDING_SHIP_BRIDGE:
        case BUILDING_FORT_GROUND:
            return 0;
        case BUILDING_WALL:
            return 18;
        case BUILDING_GATEHOUSE:
            return 34;
        case BUILDING_TOWER:
            return 46;
        case BUILDING_SMALL_STATUE:
            return 20;
        case BUILDING_MEDIUM_STATUE:
            return 30;
        case BUILDING_LARGE_STATUE:
        case BUILDING_TRIUMPHAL_ARCH:
            return 42;
        case BUILDING_SMALL_TEMPLE_CERES:
        case BUILDING_SMALL_TEMPLE_NEPTUNE:
        case BUILDING_SMALL_TEMPLE_MERCURY:
        case BUILDING_SMALL_TEMPLE_MARS:
        case BUILDING_SMALL_TEMPLE_VENUS:
            return 32;
        case BUILDING_LARGE_TEMPLE_CERES:
        case BUILDING_LARGE_TEMPLE_NEPTUNE:
        case BUILDING_LARGE_TEMPLE_MERCURY:
        case BUILDING_LARGE_TEMPLE_MARS:
        case BUILDING_LARGE_TEMPLE_VENUS:
        case BUILDING_ORACLE:
            return 48;
        case BUILDING_GRANARY:
        case BUILDING_WAREHOUSE:
        case BUILDING_MARKET:
        case BUILDING_SENATE:
        case BUILDING_FORUM:
            return 34;
        case BUILDING_GOVERNORS_HOUSE:
        case BUILDING_GOVERNORS_VILLA:
        case BUILDING_GOVERNORS_PALACE:
            return 44;
        case BUILDING_WHEAT_FARM:
        case BUILDING_VEGETABLE_FARM:
        case BUILDING_FRUIT_FARM:
        case BUILDING_OLIVE_FARM:
        case BUILDING_VINES_FARM:
        case BUILDING_PIG_FARM:
            return 18;
        case BUILDING_MARBLE_QUARRY:
        case BUILDING_IRON_MINE:
        case BUILDING_TIMBER_YARD:
        case BUILDING_CLAY_PIT:
        case BUILDING_WINE_WORKSHOP:
        case BUILDING_OIL_WORKSHOP:
        case BUILDING_WEAPONS_WORKSHOP:
        case BUILDING_FURNITURE_WORKSHOP:
        case BUILDING_POTTERY_WORKSHOP:
            return 28;
        default:
            return 30;
    }
}

static color_t building_prism_color(const building *b)
{
    if (!b || !b->id) {
        return 0x9f8563;
    }
    if (building_is_house((building_type) b->type)) {
        if (b->subtype.house_level >= HOUSE_SMALL_VILLA) {
            return 0xb9a06e;
        }
        if (b->subtype.house_level >= HOUSE_SMALL_INSULA) {
            return 0xbd8b60;
        }
        return 0xa87547;
    }

    switch (b->type) {
        case BUILDING_WALL:
        case BUILDING_GATEHOUSE:
        case BUILDING_TOWER:
        case BUILDING_FORT_LEGIONARIES:
        case BUILDING_FORT_JAVELIN:
        case BUILDING_FORT_MOUNTED:
        case BUILDING_BARRACKS:
        case BUILDING_MILITARY_ACADEMY:
            return 0x85888c;
        case BUILDING_SMALL_TEMPLE_CERES:
        case BUILDING_SMALL_TEMPLE_NEPTUNE:
        case BUILDING_SMALL_TEMPLE_MERCURY:
        case BUILDING_SMALL_TEMPLE_MARS:
        case BUILDING_SMALL_TEMPLE_VENUS:
        case BUILDING_LARGE_TEMPLE_CERES:
        case BUILDING_LARGE_TEMPLE_NEPTUNE:
        case BUILDING_LARGE_TEMPLE_MERCURY:
        case BUILDING_LARGE_TEMPLE_MARS:
        case BUILDING_LARGE_TEMPLE_VENUS:
        case BUILDING_ORACLE:
            return 0xbfb9a6;
        case BUILDING_WHEAT_FARM:
        case BUILDING_VEGETABLE_FARM:
        case BUILDING_FRUIT_FARM:
        case BUILDING_OLIVE_FARM:
        case BUILDING_VINES_FARM:
        case BUILDING_PIG_FARM:
            return 0x7f8f54;
        case BUILDING_MARBLE_QUARRY:
        case BUILDING_IRON_MINE:
        case BUILDING_TIMBER_YARD:
        case BUILDING_CLAY_PIT:
        case BUILDING_WINE_WORKSHOP:
        case BUILDING_OIL_WORKSHOP:
        case BUILDING_WEAPONS_WORKSHOP:
        case BUILDING_FURNITURE_WORKSHOP:
        case BUILDING_POTTERY_WORKSHOP:
            return 0x9b8a70;
        case BUILDING_THEATER:
        case BUILDING_AMPHITHEATER:
        case BUILDING_COLOSSEUM:
        case BUILDING_HIPPODROME:
            return 0xaa9a85;
        case BUILDING_GOVERNORS_HOUSE:
        case BUILDING_GOVERNORS_VILLA:
        case BUILDING_GOVERNORS_PALACE:
        case BUILDING_SENATE:
        case BUILDING_FORUM:
            return 0xb49b79;
        default:
            return 0xa47e55;
    }
}

static void draw_building_shadow(int x, int y, int size, int height)
{
    int width = TILE_WIDTH * size;
    int tile_height = TILE_HEIGHT * size;
    int shadow_y = y + tile_height / 2 + clamp_int(height / 3, 2, 18);
    point2d p0 = {x, shadow_y};
    point2d p1 = {x + width / 2, shadow_y - tile_height / 8};
    point2d p2 = {x + width, shadow_y};
    point2d p3 = {x + width / 2, shadow_y + tile_height / 8};

    point2d t0 = transform_point(p0.x, p0.y);
    point2d t1 = transform_point(p1.x, p1.y);
    point2d t2 = transform_point(p2.x, p2.y);
    point2d t3 = transform_point(p3.x, p3.y);
    draw_filled_quad(t0, t1, t2, t3, shade_color(0x131313, -8));
}

static void draw_building_roof(point2d top, point2d left, point2d right, point2d bottom, color_t color)
{
    point2d roof_left = {
        left.x + ((top.x - left.x) / 3),
        left.y + ((top.y - left.y) / 3)
    };
    point2d roof_right = {
        right.x + ((top.x - right.x) / 3),
        right.y + ((top.y - right.y) / 3)
    };
    point2d roof_bottom = {
        bottom.x + ((top.x - bottom.x) / 3),
        bottom.y + ((top.y - bottom.y) / 3)
    };
    draw_filled_quad(top, roof_right, roof_bottom, roof_left, shade_color(color, 28));
    color_t edge = shade_color(color, 56);
    draw_line(top, roof_right, edge);
    draw_line(roof_right, roof_bottom, edge);
    draw_line(roof_bottom, roof_left, edge);
    draw_line(roof_left, top, edge);
}

static void draw_prism(int x, int y, int size, int height, color_t color)
{
    int width = TILE_WIDTH * size;
    int tile_height = TILE_HEIGHT * size;
    point2d top = {x + width / 2, y - height};
    point2d right = {x + width, y + tile_height / 2 - height};
    point2d bottom = {x + width / 2, y + tile_height - height};
    point2d left = {x, y + tile_height / 2 - height};
    point2d base_right = {right.x, right.y + height};
    point2d base_bottom = {bottom.x, bottom.y + height};
    point2d base_left = {left.x, left.y + height};

    point2d t_top = transform_point(top.x, top.y);
    point2d t_right = transform_point(right.x, right.y);
    point2d t_bottom = transform_point(bottom.x, bottom.y);
    point2d t_left = transform_point(left.x, left.y);
    point2d t_base_right = transform_point(base_right.x, base_right.y);
    point2d t_base_bottom = transform_point(base_bottom.x, base_bottom.y);
    point2d t_base_left = transform_point(base_left.x, base_left.y);

    draw_filled_quad(t_left, t_bottom, t_base_bottom, t_base_left, shade_color(color, -32));
    draw_filled_quad(t_bottom, t_right, t_base_right, t_base_bottom, shade_color(color, -46));
    draw_filled_quad(t_top, t_right, t_bottom, t_left, shade_color(color, 24));
    draw_building_roof(t_top, t_left, t_right, t_bottom, color);

    color_t edge = shade_color(color, -72);
    draw_line(t_top, t_right, edge);
    draw_line(t_right, t_base_right, edge);
    draw_line(t_right, t_bottom, edge);
    draw_line(t_bottom, t_base_bottom, edge);
    draw_line(t_bottom, t_left, edge);
    draw_line(t_left, t_base_left, edge);
    draw_line(t_left, t_top, edge);
}

static void draw_terrain_footprint(int x, int y, int grid_offset)
{
    building_construction_record_view_position(x, y, grid_offset);
    if (grid_offset < 0) {
        point2d transformed = transform_point(x, y);
        image_draw_isometric_footprint_from_draw_tile(image_group(GROUP_TERRAIN_BLACK), transformed.x, transformed.y, 0);
        return;
    }
    if (!map_property_is_draw_tile(grid_offset)) {
        return;
    }

    int image_id = map_image_at(grid_offset);
    if (map_property_is_constructing(grid_offset)) {
        image_id = image_group(GROUP_TERRAIN_OVERLAY);
    }
    point2d transformed = transform_point(x, y);
    image_draw_isometric_footprint_from_draw_tile(image_id, transformed.x, transformed.y, 0);
}

static void draw_building_prism(int x, int y, int grid_offset)
{
    if (!map_property_is_draw_tile(grid_offset)) {
        return;
    }

    int building_id = map_building_at(grid_offset);
    if (!building_id) {
        return;
    }

    building *b = building_main(building_get(building_id));
    if (map_grid_offset_to_x(grid_offset) != b->x || map_grid_offset_to_y(grid_offset) != b->y) {
        return;
    }

    int height = building_prism_height(b);
    if (height <= 0) {
        return;
    }

    int size = b->size ? b->size : map_property_multi_tile_size(grid_offset);
    size = clamp_int(size, 1, MAX_PRISM_SIZE);
    if (height > 4) {
        draw_building_shadow(x, y, size, height);
    }
    draw_prism(x, y, size, height, building_prism_color(b));
}

static void draw_figures(int x, int y, int grid_offset)
{
    int figure_id = map_figure_at(grid_offset);
    while (figure_id) {
        figure *f = figure_get(figure_id);
        if (!f->is_ghost) {
            point2d transformed = transform_point(x, y);
            city_draw_figure(f, transformed.x, transformed.y, 0);
        }
        figure_id = f->next_figure_id_on_same_tile;
    }
}

void renderer3d_software_draw_viewport(const map_tile *selected_tile)
{
    selected_tile_for_draw = selected_tile;
    setup_view_transform();
    int should_mark_deleting = city_building_ghost_mark_deleting(selected_tile_for_draw);
    fill_viewport_background();

    city_view_foreach_map_tile(draw_terrain_footprint);
    city_view_foreach_valid_map_tile_row(draw_building_prism, draw_figures, 0);
    if (!should_mark_deleting) {
        city_building_ghost_draw(selected_tile_for_draw);
    }
}
