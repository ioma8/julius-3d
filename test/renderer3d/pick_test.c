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
    failures += expect_tile("down", 420, 296, 33, 33);
    return failures ? 1 : 0;
}
