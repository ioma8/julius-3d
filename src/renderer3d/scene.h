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
