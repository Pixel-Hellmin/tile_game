#if !defined(GAME_H)

#include "windows_main.h"

inline b32 is_set(Game_State *game_state, u32 flag)
{
    b32 result = game_state->debug_flags & flag;
    
    return result;
}

inline void set_flag(Game_State *game_state, u32 flag)
{
    game_state->debug_flags |= flag;
}

inline void unset_flag(Game_State *game_state, u32 flag)
{
    game_state->debug_flags &= ~flag;
}

#define GAME_H
#endif
