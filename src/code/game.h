#if !defined(GAME_H)

#define GAME_UPDATE_AND_RENDER(name) void name(char *trace)
typedef GAME_UPDATE_AND_RENDER(Game_Update_And_Render);
GAME_UPDATE_AND_RENDER(game_update_and_render_stub)
{
}

#define GAME_H
#endif
