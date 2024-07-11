#include <cstdio>
#include "game.h"

// NOTE(Fermin): extern "C" makes the compiler not mangle the function
// name so we can link to it with GetProcAddress
extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    printf(trace);
}
