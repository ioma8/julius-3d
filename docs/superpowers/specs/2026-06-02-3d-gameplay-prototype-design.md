# Fast Runnable 3D Gameplay Prototype Design

## Goal

Add a playable 3D city mode to Julius as quickly as possible while preserving the existing Caesar III simulation, saves, UI, and 2D renderer. The first prototype should let a player load a city, toggle 3D mode, see the current city viewport as simple 3D geometry, rotate/zoom/pan, click tiles, place buildings through the existing gameplay code, and switch back to 2D.

This is not a full 3D remake. It is a renderer and input bridge over the current game.

## Non-Goals

- Unique 3D assets for every building.
- Animated 3D humans.
- Reworked UI, advisors, menus, minimap, or dialogs.
- New game simulation, pathfinding, saves, or construction rules.
- Full-map 3D streaming.
- Mobile, console, or Emscripten support in the first prototype.
- Advanced lighting, shadows, terrain smoothing, camera collision, or visual polish.

## Recommended Approach

Use an SDL2 + OpenGL city viewport bridge.

Julius already uses SDL2 and a software framebuffer uploaded through an SDL renderer. The least invasive 3D path is to keep the current game loop and UI, add an OpenGL-capable rendering path for only the city viewport, and leave the existing 2D city renderer as a fallback.

If OpenGL initialization fails, the game logs the failure and uses the existing 2D renderer.

## Architecture

Add a new `src/renderer3d/` module with small, explicit boundaries:

- `renderer3d_mode`: stores whether 3D mode is enabled and camera yaw, pitch, and zoom.
- `renderer3d_scene`: reads existing map, building, figure, overlay, and construction state and emits transient render items.
- `renderer3d_gl`: owns OpenGL setup, shaders, placeholder meshes, depth buffer, and draw calls.
- `renderer3d_pick`: converts a mouse position in the city viewport into a map grid tile.
- `renderer3d_debug`: optional diagnostics for wireframe, culling, and coordinate checks.

`widget_city_draw()` is the first switch point:

- If 3D mode is disabled, call the existing `city_with_overlay_draw()` or `city_without_overlay_draw()` path.
- If 3D mode is enabled and OpenGL is available, draw the 3D city viewport.
- If 3D mode is enabled and OpenGL is unavailable, fall back to the current 2D path.

Top menu, sidebars, dialogs, advisors, minimap, and warning overlays remain drawn by the existing 2D UI.

## Fast Runnable Milestones

The implementation should become runnable early and stay runnable after each milestone.

Milestones 1-5 are the first runnable target. Milestones 6-8 make the prototype recognizable as Caesar III, but they should not block the first 3D gameplay proof.

1. **3D mode toggle with 2D fallback**
   Add a config/debug toggle. Normal game behavior remains unchanged by default.

2. **OpenGL viewport proof**
   Render a blank colored 3D viewport inside the existing city viewport area while the current 2D UI still draws around it.

3. **Terrain grid only**
   Render the currently visible city slice as flat 3D tiles. Ignore elevation, buildings, overlays, and figures.

4. **Basic camera**
   Add minimal camera controls: yaw rotation, zoom, and panning tied to the existing city camera center. Clamp pitch and zoom.

5. **Clickable terrain**
   Raycast from the camera through the mouse position to the terrain plane. Return the same `map_tile` information the current input flow expects so selection and construction can reuse existing logic.

6. **Buildings as boxes**
   Render visible buildings as simple boxes based on footprint and type. Houses vary height by house level. Roads and water use simple slabs or tile colors.

7. **Construction ghost**
   Render the active construction footprint as a translucent placeholder. Existing construction validity and placement logic stays unchanged.

8. **Figures as capsules**
   Render visible figures as simple upright capsules or crossed billboards. This is useful, but not required for the first runnable gameplay proof.

## Rendering Model

Use a simple world coordinate system:

- Grid tile `(x, y)` maps to world position `(x * tile_size, elevation, y * tile_size)`.
- Start with flat quads for terrain.
- Add elevation after the flat grid is working.
- Buildings use footprint size from existing building data.
- Placeholder building height and color are derived from building type/category.
- Roads are low dark slabs.
- Water is blue flat geometry.
- Farmland is green or brown tile geometry.
- Figures use existing figure positions and directions.

Render only the visible viewport slice plus a small margin for tall buildings and near-edge picking. Do not build a full-map renderer for the first prototype.

## Camera

The default camera starts near the current isometric angle so the city remains recognizable.

Controls for the first prototype:

- Existing map rotate hotkeys adjust 3D camera yaw while 3D mode is active.
- Mouse wheel adjusts 3D zoom while the city viewport is focused.
- Existing city scroll behavior pans the camera center.
- Pitch is fixed or lightly adjustable, but clamped so picking stays stable.
- Add a reset-to-isometric camera command only if camera state becomes hard to recover during testing.

The 3D camera follows the existing city camera center instead of introducing a separate gameplay camera model.

## Input And Picking

Keep the current input handlers in `widget/city.c` and swap only coordinate conversion when 3D mode is active.

Current 2D flow:

1. Mouse screen position enters `widget/city.c`.
2. `update_city_view_coords()` calls `city_view_pixels_to_view_tile()`.
3. The view tile becomes a grid offset.
4. Existing selection, building info, construction, and legion handling run.

3D flow:

1. Mouse screen position enters the same handlers.
2. A new city coordinate helper delegates to the old 2D conversion or to `renderer3d_pick`.
3. `renderer3d_pick` raycasts against the terrain plane.
4. The result is returned as the same map tile shape used by existing gameplay code.

Initial picking intersects only the terrain plane. Building-height picking can come later.

## Data Flow And Compatibility

The 3D renderer is read-mostly. It reads existing game state each frame and creates transient render commands.

Primary data sources:

- `city/view`: viewport rectangle, camera center, visible tile iteration.
- `map/*`: terrain, water, roads, elevation, building occupancy.
- `building/*`: building type, size, grid offset, house level, rubble/construction state.
- `figure/*`: figure type, current position, direction, visibility.
- `building/construction`: active construction type and ghost footprint.
- `game/state`: overlay mode, pause state, and current window context.

No 3D state is stored in Caesar III save files. Persistent 3D settings belong in preferences/config only:

- 3D mode enabled or disabled.
- Camera yaw, pitch, and zoom defaults.
- Debug flags, disabled by default.

## Platform Scope

Prototype support is desktop-first:

- macOS
- Linux
- Windows

Android, Vita, Switch, iOS, and Emscripten remain 2D-only until the desktop path is stable.

## Testing

Automated:

- Existing CTest save simulation tests must continue to pass.
- Add focused tests for coordinate conversion if the new picking math is written in testable pure C functions.

Manual smoke tests:

- Build and run on macOS.
- Load a Caesar III city.
- Toggle 3D mode on and off.
- See the city viewport render in 3D while 2D UI remains usable.
- Rotate, zoom, and pan.
- Hover and click a tile.
- Select a building.
- Place a road or house.
- Switch back to 2D without losing gameplay state.

Performance target:

- Interactive frame rate for the current viewport slice.
- No full-map performance target in the prototype.

## Risks And Mitigations

- **SDL renderer and OpenGL context conflict:** isolate OpenGL setup behind a backend probe and keep automatic 2D fallback.
- **UI compositing complexity:** keep 2D UI unchanged and restrict 3D to the city viewport.
- **Picking bugs:** start with terrain-plane picking only and preserve current input handlers.
- **Scope creep into asset work:** use procedural placeholder geometry until gameplay is runnable.
- **Platform churn:** support desktop first and keep unsupported platforms on 2D.

## First Runnable Definition

The first runnable implementation is successful when a player can:

1. Open a real Julius city.
2. Enable 3D mode.
3. See the visible viewport as a rotatable 3D terrain grid.
4. Use existing UI and construction flow.
5. Click and place on the 3D terrain.
6. Disable 3D mode and continue in the original 2D renderer.

## Prototype Success Definition

The prototype is successful when a player can:

1. Open a real Julius city.
2. Enable 3D mode.
3. See the visible viewport as rotatable placeholder 3D terrain and building boxes.
4. Use existing UI and construction flow.
5. Click and place on the 3D terrain.
6. Disable 3D mode and continue in the original 2D renderer.
