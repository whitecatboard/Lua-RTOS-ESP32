#ifndef _BLOCKS_H_
#define _BLOCKS_H_

#include "lua.h"
#include "lstate.h"
#include "llex.h"

#define luaVB_BLOCK_START_MSG       1
#define luaVB_BLOCK_END_MSG         2
#define luaVB_BLOCK_ERR_MSG         3
#define luaVB_BLOCK_ERR_CATCH_MSG   4

#define luaVB_BLOCK_MSG_STATE_NONE   0
#define luaVB_BLOCK_MSG_STATE_START  1
#define luaVB_BLOCK_MSG_STATE_END    2

BlockContext *luaVB_getBlock(lua_State *L, CallInfo *where);
BlockContext *luaVB_pushBlock(lua_State *L, CallInfo *where, int id);
BlockContext *luaVB_popBlock(lua_State *L, CallInfo *where);
void luaVB_dumpBlock(BlockContext *bctx);
void luaVB_emitMessage(lua_State *L, int type, int id);

void luaXB_openAnnotation(LexState *ls, int i);
void luaXB_closeAnnotation(LexState *ls, int i);
void luaK_emitAnnotation(LexState *ls);

int luaVB_init(lua_State *L);

#endif
