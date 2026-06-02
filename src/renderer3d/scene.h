#ifndef RENDERER3D_SCENE_H
#define RENDERER3D_SCENE_H

#define RENDERER3D_MAX_TERRAIN_ITEMS 4096
#define RENDERER3D_MAX_BUILDING_ITEMS 1024

typedef struct {
    int grid_offset;
    int x;
    int y;
    int terrain;
} renderer3d_terrain_item;

typedef struct {
    int building_id;
    int grid_offset;
    int x;
    int y;
    int size;
    int type;
    int height;
} renderer3d_building_item;

typedef struct {
    renderer3d_terrain_item terrain[RENDERER3D_MAX_TERRAIN_ITEMS];
    int terrain_count;
    renderer3d_building_item buildings[RENDERER3D_MAX_BUILDING_ITEMS];
    int building_count;
} renderer3d_scene;

void renderer3d_scene_build(renderer3d_scene *scene);

#endif // RENDERER3D_SCENE_H
