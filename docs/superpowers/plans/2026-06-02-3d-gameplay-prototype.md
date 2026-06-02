# 3D Gameplay Prototype Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the fastest runnable Julius 3D city mode: toggleable 3D terrain viewport, basic camera, tile picking, existing construction flow, and safe 2D fallback.

**Architecture:** Preserve Julius simulation, saves, UI, and 2D renderer. Add a small `src/renderer3d/` module that exposes mode state, pure camera/picking math, and a temporary framebuffer-backed 3D viewport path; hook it at `widget_city_draw()` and `widget/city.c` coordinate conversion so existing gameplay code still runs.

**Tech Stack:** C99, CMake, SDL2, existing Julius software framebuffer, OpenGL only after the framebuffer prototype path proves integration.

---

## Scope

This plan prioritizes a runnable prototype over visual completeness.

The first runnable target is complete after Task 7:

1. 3D mode can be toggled with config state.
2. City viewport can render a distinct 3D/debug terrain grid while the rest of UI remains usable.
3. Camera yaw/zoom/pan changes are visible.
4. Mouse picking returns a real map tile.
5. Existing selection and construction code uses that tile.
6. 2D fallback remains available.

Tasks 8-10 make the prototype recognizable with building boxes, construction ghost geometry, and figure placeholders.

## File Structure

- Create `src/renderer3d/mode.h`
  Public API for enabling/disabling 3D mode and camera state.

- Create `src/renderer3d/mode.c`
  Stores runtime mode and camera values. Reads/writes config-backed defaults.

- Create `src/renderer3d/math.h`
  Small vector/matrix and camera structs used by testable picking code.

- Create `src/renderer3d/math.c`
  Pure C projection, ray, and grid intersection helpers.

- Create `src/renderer3d/scene.h`
  Defines render item structs for terrain, buildings, construction ghosts, and figures.

- Create `src/renderer3d/scene.c`
  Converts the current visible city slice into transient render items.

- Create `src/renderer3d/software.h`
  Public renderer API for the fast first viewport renderer.

- Create `src/renderer3d/software.c`
  Draws a top-down/perspective debug 3D-style terrain grid into the existing framebuffer. This avoids SDL/OpenGL composition risk until gameplay integration is proven.

- Create `src/renderer3d/pick.h`
  Public API for converting viewport pixels to `map_tile` data.

- Create `src/renderer3d/pick.c`
  Uses pure math helpers and current camera state to return grid offsets.

- Create `test/renderer3d/pick_test.c`
  CTest executable for camera ray and grid picking.

- Modify `CMakeLists.txt`
  Add renderer3d source files and the pick test executable.

- Modify `src/core/config.h`
  Add `CONFIG_UI_3D_VIEW`.

- Modify `src/core/config.c`
  Add the `ui_3d_view` INI key and default value `0`.

- Modify `src/widget/city.c`
  Switch city viewport rendering and coordinate conversion when 3D mode is active.

- Modify `src/window/city.c`
  Route rotate hotkeys to 3D camera yaw when 3D mode is active.

- Modify `src/window/config.c`
  No first-runnable change. The first runnable toggle uses the existing `julius.ini` config path to avoid translation and UI churn.

## Task 1: Add 3D Mode State And Config

**Files:**
- Create: `src/renderer3d/mode.h`
- Create: `src/renderer3d/mode.c`
- Modify: `src/core/config.h`
- Modify: `src/core/config.c`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add the config key**

In `src/core/config.h`, add `CONFIG_UI_3D_VIEW` immediately before `CONFIG_MAX_ENTRIES`:

```c
typedef enum {
    CONFIG_GP_FIX_IMMIGRATION_BUG,
    CONFIG_GP_FIX_100_YEAR_GHOSTS,
    CONFIG_SCREEN_DISPLAY_SCALE,
    CONFIG_SCREEN_CURSOR_SCALE,
    CONFIG_UI_SIDEBAR_INFO,
    CONFIG_UI_SHOW_INTRO_VIDEO,
    CONFIG_UI_SMOOTH_SCROLLING,
    CONFIG_UI_DISABLE_MOUSE_EDGE_SCROLLING,
    CONFIG_UI_DISABLE_RIGHT_CLICK_MAP_DRAG,
    CONFIG_UI_INVERSE_RIGHT_CLICK_MAP_DRAG,
    CONFIG_UI_VISUAL_FEEDBACK_ON_DELETE,
    CONFIG_UI_ALLOW_CYCLING_TEMPLES,
    CONFIG_UI_SHOW_WATER_STRUCTURE_RANGE,
    CONFIG_UI_SHOW_CONSTRUCTION_SIZE,
    CONFIG_UI_HIGHLIGHT_LEGIONS,
    CONFIG_UI_SHOW_MILITARY_SIDEBAR,
    CONFIG_UI_SHOW_SPEEDRUN_INFO,
    CONFIG_UI_3D_VIEW,
    CONFIG_MAX_ENTRIES
} config_key;
```

In `src/core/config.c`, add the matching INI key at the end of `ini_keys[]`:

```c
static const char *ini_keys[] = {
    "gameplay_fix_immigration",
    "gameplay_fix_100y_ghosts",
    "screen_display_scale",
    "screen_cursor_scale",
    "ui_sidebar_info",
    "ui_show_intro_video",
    "ui_smooth_scrolling",
    "ui_disable_mouse_edge_scrolling",
    "ui_disable_map_drag",
    "ui_inverse_map_drag",
    "ui_visual_feedback_on_delete",
    "ui_allow_cycling_temples",
    "ui_show_water_structure_range",
    "ui_show_construction_size",
    "ui_highlight_legions",
    "ui_show_military_sidebar",
    "ui_show_speedrun_info",
    "ui_3d_view",
};
```

Expected: `CONFIG_UI_3D_VIEW` defaults to `0` because `default_values` is zero-initialized except explicit entries.

- [ ] **Step 2: Add mode API**

Create `src/renderer3d/mode.h`:

```c
#ifndef RENDERER3D_MODE_H
#define RENDERER3D_MODE_H

typedef struct {
    int enabled;
    float yaw_degrees;
    float pitch_degrees;
    float zoom;
} renderer3d_camera;

void renderer3d_mode_init(void);
int renderer3d_mode_is_enabled(void);
void renderer3d_mode_set_enabled(int enabled);
void renderer3d_mode_toggle(void);

const renderer3d_camera *renderer3d_camera_get(void);
void renderer3d_camera_rotate_yaw(float delta_degrees);
void renderer3d_camera_zoom(float delta);
void renderer3d_camera_reset(void);

#endif // RENDERER3D_MODE_H
```

Create `src/renderer3d/mode.c`:

```c
#include "renderer3d/mode.h"

#include "core/config.h"

static renderer3d_camera camera = {
    0,
    45.0f,
    55.0f,
    1.0f
};

static float clamp_float(float value, float min, float max)
{
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}

void renderer3d_mode_init(void)
{
    camera.enabled = config_get(CONFIG_UI_3D_VIEW) ? 1 : 0;
    renderer3d_camera_reset();
}

int renderer3d_mode_is_enabled(void)
{
    return camera.enabled;
}

void renderer3d_mode_set_enabled(int enabled)
{
    camera.enabled = enabled ? 1 : 0;
    config_set(CONFIG_UI_3D_VIEW, camera.enabled);
}

void renderer3d_mode_toggle(void)
{
    renderer3d_mode_set_enabled(!camera.enabled);
}

const renderer3d_camera *renderer3d_camera_get(void)
{
    return &camera;
}

void renderer3d_camera_rotate_yaw(float delta_degrees)
{
    camera.yaw_degrees += delta_degrees;
    while (camera.yaw_degrees >= 360.0f) {
        camera.yaw_degrees -= 360.0f;
    }
    while (camera.yaw_degrees < 0.0f) {
        camera.yaw_degrees += 360.0f;
    }
}

void renderer3d_camera_zoom(float delta)
{
    camera.zoom = clamp_float(camera.zoom + delta, 0.5f, 4.0f);
}

void renderer3d_camera_reset(void)
{
    camera.yaw_degrees = 45.0f;
    camera.pitch_degrees = 55.0f;
    camera.zoom = 1.0f;
}
```

- [ ] **Step 3: Add files to CMake**

In `CMakeLists.txt`, add a renderer3d source group after `GRAPHICS_FILES`:

```cmake
set(RENDERER3D_FILES
    ${PROJECT_SOURCE_DIR}/src/renderer3d/mode.c
)
```

Add `${RENDERER3D_FILES}` to `SOURCE_FILES` between `${GRAPHICS_FILES}` and `${SOUND_FILES}`:

```cmake
    ${GRAPHICS_FILES}
    ${RENDERER3D_FILES}
    ${SOUND_FILES}
```

- [ ] **Step 4: Initialize mode from game startup**

Modify `src/game/game.c` after `config_load();`:

```c
#include "renderer3d/mode.h"
```

and:

```c
config_load();
renderer3d_mode_init();
hotkey_config_load();
```

- [ ] **Step 5: Build**

Run:

```bash
cmake --build build-macos-validate --parallel
```

Expected: build completes with exit code `0`.

- [ ] **Step 6: Commit**

```bash
git add CMakeLists.txt src/core/config.h src/core/config.c src/game/game.c src/renderer3d/mode.h src/renderer3d/mode.c
git commit -m "feat: add 3d mode state"
```

## Task 2: Add A Fast 3D Viewport Placeholder

**Files:**
- Create: `src/renderer3d/software.h`
- Create: `src/renderer3d/software.c`
- Modify: `src/widget/city.c`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add software renderer API**

Create `src/renderer3d/software.h`:

```c
#ifndef RENDERER3D_SOFTWARE_H
#define RENDERER3D_SOFTWARE_H

void renderer3d_software_draw_viewport(void);

#endif // RENDERER3D_SOFTWARE_H
```

Create `src/renderer3d/software.c`:

```c
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
```

- [ ] **Step 2: Add file to CMake**

Update `RENDERER3D_FILES`:

```cmake
set(RENDERER3D_FILES
    ${PROJECT_SOURCE_DIR}/src/renderer3d/mode.c
    ${PROJECT_SOURCE_DIR}/src/renderer3d/software.c
)
```

- [ ] **Step 3: Switch city draw**

Modify `src/widget/city.c` includes:

```c
#include "renderer3d/mode.h"
#include "renderer3d/software.h"
```

Modify `widget_city_draw()`:

```c
void widget_city_draw(void)
{
    set_city_clip_rectangle();

    if (renderer3d_mode_is_enabled()) {
        renderer3d_software_draw_viewport();
    } else if (game_state_overlay()) {
        city_with_overlay_draw(&data.current_tile);
    } else {
        city_without_overlay_draw(0, 0, &data.current_tile);
    }

    graphics_reset_clip_rectangle();
}
```

- [ ] **Step 4: Build**

Run:

```bash
cmake --build build-macos-validate --parallel
```

Expected: build completes with exit code `0`.

- [ ] **Step 5: Manual smoke**

Set `ui_3d_view=1` in `julius.ini`, run the app with a Caesar III data path, and confirm the city viewport shows a dark grid while top menu/sidebar still draw.

- [ ] **Step 6: Commit**

```bash
git add CMakeLists.txt src/widget/city.c src/renderer3d/software.h src/renderer3d/software.c
git commit -m "feat: draw 3d viewport placeholder"
```

## Task 3: Add Terrain Slice Scene Data

**Files:**
- Create: `src/renderer3d/scene.h`
- Create: `src/renderer3d/scene.c`
- Modify: `src/renderer3d/software.c`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add scene structs**

Create `src/renderer3d/scene.h`:

```c
#ifndef RENDERER3D_SCENE_H
#define RENDERER3D_SCENE_H

#define RENDERER3D_MAX_TERRAIN_ITEMS 4096

typedef struct {
    int grid_offset;
    int x;
    int y;
    int terrain;
} renderer3d_terrain_item;

typedef struct {
    renderer3d_terrain_item terrain[RENDERER3D_MAX_TERRAIN_ITEMS];
    int terrain_count;
} renderer3d_scene;

void renderer3d_scene_build(renderer3d_scene *scene);

#endif // RENDERER3D_SCENE_H
```

- [ ] **Step 2: Build visible terrain list**

Create `src/renderer3d/scene.c`:

```c
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
```

- [ ] **Step 3: Color terrain by type**

Modify `src/renderer3d/software.c`:

```c
#include "map/terrain.h"
#include "renderer3d/scene.h"
```

Add:

```c
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
```

Inside `renderer3d_software_draw_viewport()`, after drawing the background, build and draw the scene:

```c
renderer3d_scene scene;
renderer3d_scene_build(&scene);

for (int i = 0; i < scene.terrain_count; i++) {
    const renderer3d_terrain_item *item = &scene.terrain[i];
    int sx = vx + (item->x % 64) * 12;
    int sy = vy + (item->y % 64) * 8;
    fill_rect_clipped(sx, sy, 10, 6, terrain_color(item->terrain));
}
```

- [ ] **Step 4: Add file to CMake**

Update `RENDERER3D_FILES`:

```cmake
set(RENDERER3D_FILES
    ${PROJECT_SOURCE_DIR}/src/renderer3d/mode.c
    ${PROJECT_SOURCE_DIR}/src/renderer3d/scene.c
    ${PROJECT_SOURCE_DIR}/src/renderer3d/software.c
)
```

- [ ] **Step 5: Build and run tests**

Run:

```bash
cmake --build build-macos-validate --parallel
ctest --test-dir build-macos-validate --output-on-failure
```

Expected: build completes; all existing tests pass.

- [ ] **Step 6: Commit**

```bash
git add CMakeLists.txt src/renderer3d/scene.h src/renderer3d/scene.c src/renderer3d/software.c
git commit -m "feat: render visible 3d terrain slice"
```

## Task 4: Add Camera Controls

**Files:**
- Modify: `src/window/city.c`
- Modify: `src/widget/city.c`
- Modify: `src/renderer3d/software.c`

- [ ] **Step 1: Route rotation hotkeys**

Modify `src/window/city.c` includes:

```c
#include "renderer3d/mode.h"
```

Modify the rotate branches in `handle_hotkeys()`:

```c
if (h->rotate_map_left) {
    if (renderer3d_mode_is_enabled()) {
        renderer3d_camera_rotate_yaw(-15.0f);
    } else {
        game_orientation_rotate_left();
    }
    window_invalidate();
}
if (h->rotate_map_right) {
    if (renderer3d_mode_is_enabled()) {
        renderer3d_camera_rotate_yaw(15.0f);
    } else {
        game_orientation_rotate_right();
    }
    window_invalidate();
}
```

- [ ] **Step 2: Add mouse wheel zoom**

Modify `src/widget/city.c` after `scroll_map(m);` in `widget_city_handle_input()`:

```c
if (renderer3d_mode_is_enabled() && input_coords_in_city(m->x, m->y)) {
    if (m->scrolled == SCROLL_UP) {
        renderer3d_camera_zoom(0.1f);
        window_request_refresh();
    } else if (m->scrolled == SCROLL_DOWN) {
        renderer3d_camera_zoom(-0.1f);
        window_request_refresh();
    }
}
```

- [ ] **Step 3: Make terrain projection respond to camera**

In `src/renderer3d/software.c`, replace the simple modulo terrain projection with:

```c
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
```

Add `#include <math.h>`.

Use it in the terrain loop:

```c
int px, py;
project_tile(item->x, item->y, camera, &px, &py);
int sx = vx + vw / 2 + px;
int sy = vy + vh / 2 + py;
fill_rect_clipped(sx, sy, 10, 6, terrain_color(item->terrain));
```

- [ ] **Step 4: Build and smoke**

Run:

```bash
cmake --build build-macos-validate --parallel
```

Expected: build completes. Manual smoke: rotate hotkeys visibly rotate the grid; mouse wheel zoom changes grid spacing.

- [ ] **Step 5: Commit**

```bash
git add src/window/city.c src/widget/city.c src/renderer3d/software.c
git commit -m "feat: add 3d camera controls"
```

## Task 5: Add Pure Picking Math Tests

**Files:**
- Create: `src/renderer3d/math.h`
- Create: `src/renderer3d/math.c`
- Create: `test/renderer3d/pick_test.c`
- Modify: `CMakeLists.txt`
- Modify: `test/CMakeLists.txt`

- [ ] **Step 1: Add math API**

Create `src/renderer3d/math.h`:

```c
#ifndef RENDERER3D_MATH_H
#define RENDERER3D_MATH_H

typedef struct {
    float x;
    float y;
} renderer3d_vec2;

int renderer3d_screen_to_tile(
    int screen_x, int screen_y,
    int viewport_x, int viewport_y,
    int viewport_width, int viewport_height,
    float yaw_degrees, float zoom,
    int *tile_x, int *tile_y);

#endif // RENDERER3D_MATH_H
```

Create `src/renderer3d/math.c`:

```c
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
```

- [ ] **Step 2: Add failing pick test**

Create `test/renderer3d/pick_test.c`:

```c
#include "renderer3d/math.h"

#include <stdio.h>

static int expect_tile(const char *name, int sx, int sy, int expected_x, int expected_y)
{
    int tile_x = -1;
    int tile_y = -1;
    int ok = renderer3d_screen_to_tile(sx, sy, 100, 50, 640, 480, 45.0f, 1.0f, &tile_x, &tile_y);
    if (!ok || tile_x != expected_x || tile_y != expected_y) {
        printf("%s: expected %d,%d got %d,%d ok=%d\n", name, expected_x, expected_y, tile_x, tile_y, ok);
        return 1;
    }
    return 0;
}

int main(void)
{
    int failures = 0;
    failures += expect_tile("center", 420, 290, 32, 32);
    failures += expect_tile("right", 430, 290, 33, 31);
    failures += expect_tile("down", 420, 296, 32, 33);
    return failures ? 1 : 0;
}
```

- [ ] **Step 3: Wire test into CMake**

Update `RENDERER3D_FILES`:

```cmake
set(RENDERER3D_FILES
    ${PROJECT_SOURCE_DIR}/src/renderer3d/math.c
    ${PROJECT_SOURCE_DIR}/src/renderer3d/mode.c
    ${PROJECT_SOURCE_DIR}/src/renderer3d/scene.c
    ${PROJECT_SOURCE_DIR}/src/renderer3d/software.c
)
```

In `test/CMakeLists.txt`, add:

```cmake
add_executable(renderer3d_pick
    renderer3d/pick_test.c
    ${PROJECT_SOURCE_DIR}/src/renderer3d/math.c
)
target_include_directories(renderer3d_pick PRIVATE ${PROJECT_SOURCE_DIR}/src)
if(UNIX AND NOT APPLE)
    target_link_libraries(renderer3d_pick m)
endif()
add_test(NAME renderer3d_pick COMMAND renderer3d_pick)
```

- [ ] **Step 4: Run test**

Run:

```bash
cmake -S . -B build-macos-validate -DCMAKE_BUILD_TYPE=Release -DTARGET_PLATFORM=default
cmake --build build-macos-validate --parallel
ctest --test-dir build-macos-validate -R renderer3d_pick --output-on-failure
```

Expected: `renderer3d_pick` passes.

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt test/CMakeLists.txt test/renderer3d/pick_test.c src/renderer3d/math.h src/renderer3d/math.c
git commit -m "test: cover 3d terrain picking math"
```

## Task 6: Route 3D Picking Into Existing City Input

**Files:**
- Create: `src/renderer3d/pick.h`
- Create: `src/renderer3d/pick.c`
- Modify: `src/widget/city.c`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add pick API**

Create `src/renderer3d/pick.h`:

```c
#ifndef RENDERER3D_PICK_H
#define RENDERER3D_PICK_H

#include "map/grid.h"

int renderer3d_pick_tile(int screen_x, int screen_y, map_tile *tile);

#endif // RENDERER3D_PICK_H
```

Create `src/renderer3d/pick.c`:

```c
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
```

- [ ] **Step 2: Add file to CMake**

Update `RENDERER3D_FILES`:

```cmake
set(RENDERER3D_FILES
    ${PROJECT_SOURCE_DIR}/src/renderer3d/math.c
    ${PROJECT_SOURCE_DIR}/src/renderer3d/mode.c
    ${PROJECT_SOURCE_DIR}/src/renderer3d/pick.c
    ${PROJECT_SOURCE_DIR}/src/renderer3d/scene.c
    ${PROJECT_SOURCE_DIR}/src/renderer3d/software.c
)
```

- [ ] **Step 3: Use 3D picking in city widget**

Modify `src/widget/city.c` includes:

```c
#include "renderer3d/pick.h"
```

Modify `update_city_view_coords()`:

```c
static void update_city_view_coords(int x, int y, map_tile *tile)
{
    if (renderer3d_mode_is_enabled()) {
        if (renderer3d_pick_tile(x, y, tile)) {
            view_tile view;
            city_view_grid_offset_to_xy_view(tile->grid_offset, &view.x, &view.y);
            city_view_set_selected_view_tile(&view);
        }
        return;
    }

    view_tile view;
    if (city_view_pixels_to_view_tile(x, y, &view)) {
        tile->grid_offset = city_view_tile_to_grid_offset(&view);
        city_view_set_selected_view_tile(&view);
        tile->x = map_grid_offset_to_x(tile->grid_offset);
        tile->y = map_grid_offset_to_y(tile->grid_offset);
    } else {
        tile->grid_offset = tile->x = tile->y = 0;
    }
}
```

- [ ] **Step 4: Build and test**

Run:

```bash
cmake --build build-macos-validate --parallel
ctest --test-dir build-macos-validate --output-on-failure
```

Expected: build completes and all tests pass.

- [ ] **Step 5: Manual gameplay smoke**

With `ui_3d_view=1`, load a city and verify:

- Hover changes current tile.
- Left click starts construction when a building type is selected.
- Right click on empty city terrain does not crash.
- Set `ui_3d_view=0` and verify original 2D picking still works.

- [ ] **Step 6: Commit**

```bash
git add CMakeLists.txt src/widget/city.c src/renderer3d/pick.h src/renderer3d/pick.c
git commit -m "feat: route city input through 3d picking"
```

## Task 7: Verify First Runnable 3D Gameplay

**Files:**
- No source changes expected.

- [ ] **Step 1: Build and test**

Run:

```bash
cmake --build build-macos-validate --parallel
ctest --test-dir build-macos-validate --output-on-failure
```

Expected: build completes and all tests pass.

- [ ] **Step 2: Manual smoke with 3D disabled**

Set `ui_3d_view=0` in `julius.ini`, run Julius with valid Caesar III data, and verify the original 2D city view still draws and accepts input.

- [ ] **Step 3: Manual smoke with 3D enabled**

Set `ui_3d_view=1` in `julius.ini`, run Julius with valid Caesar III data, and verify:

- The city viewport draws the 3D terrain grid.
- Top menu and sidebar still draw.
- Rotate hotkeys change camera yaw.
- Mouse wheel changes zoom.
- Hover/click returns tiles.
- Road or house placement uses existing construction behavior.

- [ ] **Step 4: Record verification result**

Run:

```bash
git status --short
```

Expected: no uncommitted source changes from this verification-only task.

## Task 8: Add Building Boxes

**Files:**
- Modify: `src/renderer3d/scene.h`
- Modify: `src/renderer3d/scene.c`
- Modify: `src/renderer3d/software.c`

- [ ] **Step 1: Extend scene with buildings**

In `src/renderer3d/scene.h`, add:

```c
#define RENDERER3D_MAX_BUILDING_ITEMS 1024

typedef struct {
    int building_id;
    int grid_offset;
    int x;
    int y;
    int size;
    int type;
    int height;
} renderer3d_building_item;
```

Add to `renderer3d_scene`:

```c
renderer3d_building_item buildings[RENDERER3D_MAX_BUILDING_ITEMS];
int building_count;
```

- [ ] **Step 2: Collect visible buildings**

In `src/renderer3d/scene.c`, include:

```c
#include "building/building.h"
#include "map/building.h"
```

In `renderer3d_scene_build()`, set `scene->building_count = 0;`.

In `add_tile()`, after terrain add:

```c
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
            item->height += b->house_level;
        }
    }
}
```

- [ ] **Step 3: Draw boxes as raised rectangles**

In `src/renderer3d/software.c`, add:

```c
static color_t building_color(int type)
{
    (void) type;
    return 0xff9b7a4a;
}
```

After terrain loop:

```c
for (int i = 0; i < scene.building_count; i++) {
    const renderer3d_building_item *item = &scene.buildings[i];
    int px, py;
    project_tile(item->x, item->y, camera, &px, &py);
    int sx = vx + vw / 2 + px;
    int sy = vy + vh / 2 + py - item->height;
    fill_rect_clipped(sx, sy, item->size * 12, item->size * 8 + item->height, building_color(item->type));
    graphics_draw_horizontal_line(sx, sx + item->size * 12, sy, 0xffd0b080);
}
```

- [ ] **Step 4: Build and smoke**

Run:

```bash
cmake --build build-macos-validate --parallel
```

Expected: build completes. Manual smoke: buildings appear as raised blocks over terrain.

- [ ] **Step 5: Commit**

```bash
git add src/renderer3d/scene.h src/renderer3d/scene.c src/renderer3d/software.c
git commit -m "feat: render 3d building boxes"
```

## Task 9: Add Construction Ghost

**Files:**
- Modify: `src/renderer3d/scene.h`
- Modify: `src/renderer3d/scene.c`
- Modify: `src/renderer3d/software.c`

- [ ] **Step 1: Add ghost data**

In `src/renderer3d/scene.h`, add:

```c
typedef struct {
    int active;
    int x;
    int y;
    int size_x;
    int size_y;
} renderer3d_construction_ghost;
```

Add to `renderer3d_scene`:

```c
renderer3d_construction_ghost ghost;
```

- [ ] **Step 2: Populate ghost**

In `src/renderer3d/scene.c`, include:

```c
#include "building/construction.h"
#include "building/properties.h"
```

At the end of `renderer3d_scene_build()`:

```c
scene->ghost.active = 0;
int size_x = 0;
int size_y = 0;
if (building_construction_type() && building_construction_size(&size_x, &size_y)) {
    scene->ghost.active = 1;
    scene->ghost.x = 0;
    scene->ghost.y = 0;
    scene->ghost.size_x = size_x;
    scene->ghost.size_y = size_y;
}
```

Use the existing `building_construction_get_start_grid_offset()` function:

```c
int grid_offset = building_construction_get_start_grid_offset();
if (grid_offset > 0) {
    scene->ghost.x = map_grid_offset_to_x(grid_offset);
    scene->ghost.y = map_grid_offset_to_y(grid_offset);
}
```

- [ ] **Step 3: Draw ghost**

In `src/renderer3d/software.c`, after buildings:

```c
if (scene.ghost.active) {
    int px, py;
    project_tile(scene.ghost.x, scene.ghost.y, camera, &px, &py);
    fill_rect_clipped(vx + vw / 2 + px, vy + vh / 2 + py,
        scene.ghost.size_x * 12, scene.ghost.size_y * 8, 0x99ffff00);
}
```

- [ ] **Step 4: Build and smoke**

Run:

```bash
cmake --build build-macos-validate --parallel
```

Expected: build completes. Manual smoke: choose a building and move over 3D terrain; ghost footprint appears.

- [ ] **Step 5: Commit**

```bash
git add src/renderer3d/scene.h src/renderer3d/scene.c src/renderer3d/software.c src/building/construction.h src/building/construction.c
git commit -m "feat: render 3d construction ghost"
```

## Task 10: Add Figure Placeholders

**Files:**
- Modify: `src/renderer3d/scene.h`
- Modify: `src/renderer3d/scene.c`
- Modify: `src/renderer3d/software.c`

- [ ] **Step 1: Add figure data**

In `src/renderer3d/scene.h`, add:

```c
#define RENDERER3D_MAX_FIGURE_ITEMS 512

typedef struct {
    int figure_id;
    int x;
    int y;
    int type;
} renderer3d_figure_item;
```

Add to `renderer3d_scene`:

```c
renderer3d_figure_item figures[RENDERER3D_MAX_FIGURE_ITEMS];
int figure_count;
```

- [ ] **Step 2: Collect figures**

In `src/renderer3d/scene.c`, include:

```c
#include "figure/figure.h"
```

In `renderer3d_scene_build()`, set `scene->figure_count = 0;`.

Add a loop after terrain collection:

```c
for (int i = 1; i < MAX_FIGURES && scene->figure_count < RENDERER3D_MAX_FIGURE_ITEMS; i++) {
    const figure *f = figure_get(i);
    if (!f->id) {
        continue;
    }
    renderer3d_figure_item *item = &scene->figures[scene->figure_count++];
    item->figure_id = i;
    item->x = f->x;
    item->y = f->y;
    item->type = f->type;
}
```

- [ ] **Step 3: Draw capsules as small vertical bars**

In `src/renderer3d/software.c`, after ghost:

```c
for (int i = 0; i < scene.figure_count; i++) {
    const renderer3d_figure_item *item = &scene.figures[i];
    int px, py;
    project_tile(item->x, item->y, camera, &px, &py);
    fill_rect_clipped(vx + vw / 2 + px, vy + vh / 2 + py - 12, 4, 12, 0xffe8d8b0);
}
```

- [ ] **Step 4: Build and smoke**

Run:

```bash
cmake --build build-macos-validate --parallel
```

Expected: build completes. Manual smoke: visible walkers appear as small bars.

- [ ] **Step 5: Commit**

```bash
git add src/renderer3d/scene.h src/renderer3d/scene.c src/renderer3d/software.c
git commit -m "feat: render 3d figure placeholders"
```

## Task 11: Final Verification

**Files:**
- No source changes expected.

- [ ] **Step 1: Reconfigure**

Run:

```bash
cmake -S . -B build-macos-validate -DCMAKE_BUILD_TYPE=Release -DTARGET_PLATFORM=default
```

Expected: configure completes with exit code `0`.

- [ ] **Step 2: Build**

Run:

```bash
cmake --build build-macos-validate --parallel
```

Expected: build completes with exit code `0`.

- [ ] **Step 3: Run tests**

Run:

```bash
ctest --test-dir build-macos-validate --output-on-failure
```

Expected: all tests pass.

- [ ] **Step 4: Manual smoke checklist**

Run Julius with valid Caesar III data and verify:

- `ui_3d_view=0` gives the original 2D city view.
- `ui_3d_view=1` gives the 3D city view.
- Top menu, sidebar, minimap, pause, and dialogs still draw.
- Rotate hotkeys rotate 3D yaw.
- Mouse wheel zooms 3D view.
- Pan/scroll still moves the visible city area.
- Hovering/clicking terrain returns valid tiles.
- Road or house placement works in 3D mode.
- Switching back to 2D preserves gameplay state.

- [ ] **Step 5: Record final status**

Run:

```bash
git status --short
```

Expected: no uncommitted source changes from final verification.
