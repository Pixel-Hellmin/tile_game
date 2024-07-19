#if !defined(GAME_H)

#include "buffer.cpp"
#include "math.cpp"

struct Controls
{
    b32 up;
    b32 down;
    b32 right;
    b32 left;
};

struct Rect
{
    // NOTE(Fermin): This rectangles are drawn in 3d space,
    // the min and max p are coords in world space and z is always 0
    V2 min_p;
    V2 max_p;
    V4 color;
};

#define GAME_UPDATE_AND_RENDER(name) void name(Buffer *render_rect, u32 *tile_count, Rect *dude, Controls *input_state)
typedef GAME_UPDATE_AND_RENDER(Game_Update_And_Render);
GAME_UPDATE_AND_RENDER(game_update_and_render_stub)
{
}

#define GAME_H
#endif
