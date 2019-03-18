#include "lua.h"
#include "lstate.h"
#include "llex.h"
#include "lparser.h"
#include "blocks.h"

#include <string.h>

#include <sys/time.h>

static char *parseErrMsg(const char *error_msg, int *err) {
    const char *whereEnd = NULL;
    const char *lineEnd = NULL;
    const char *messageStart = NULL;
    const char *messageEnd = NULL;

    whereEnd = strchr(error_msg,':');
    if (whereEnd) {
        lineEnd = strchr(whereEnd + 1,':');
    }

    if (lineEnd) {
        if (!sscanf(lineEnd + 2,"%d:", err)) {
            *err = 0;
            messageStart = lineEnd + 2;
        } else {
            messageStart = strchr(lineEnd + 2,':');
            messageStart++;
        }
    } else {
        messageStart = error_msg;
    }

    messageEnd = messageStart;
    while (*messageEnd && (*messageEnd != '\r') && (*messageEnd != '\n')) {
        messageEnd++;
    }

    int size = (messageEnd - messageStart);
    char *msg = calloc(1, size + 1);
    lua_assert(msg != NULL);

    memcpy(msg, messageStart, size);

    return msg;
}

void luaVB_dumpBlock(BlockContext *bctx) {
    printf("block stack\r\n");

    while (bctx) {
        printf("block %d\r\n",bctx->block);
        bctx = bctx->previous;
    }

    printf("\r\n");
}

BlockContext *luaVB_getBlock(lua_State *L, CallInfo *where) {
    CallInfo *ci;

    if (where == NULL) {
        ci = L->ci;
    } else{
        ci = where;
    }

    return ci->bctx;
}

BlockContext *luaVB_pushBlock(lua_State *L, CallInfo *where, int id) {
    CallInfo *ci;

    if (where == NULL) {
        ci = L->ci;
    } else{
        ci = where;
    }

    // Check that id is not on top
    if (ci->bctx && ci->bctx->block == id) {
        return ci->bctx;
    }

    // Push id
    BlockContext *bctx = luaM_malloc(L, sizeof(BlockContext));
    if (bctx) {
        bctx->block = id;
        bctx->previous = ci->bctx;
        ci->bctx = bctx;
    }

    return bctx;
}

BlockContext *luaVB_popBlock(lua_State *L, CallInfo *where) {
    CallInfo *ci;

    if (where == NULL) {
        ci = L->ci;
    } else{
        ci = where;
    }

    BlockContext *bctx = ci->bctx;
    if (bctx) {
        ci->bctx = bctx->previous;
        luaM_freemem(L, bctx, sizeof(BlockContext));
    }

    return ci->bctx;
}

#define LUAVB_BLOCK_MESSAGE_TABLE_SIZE 32

typedef struct BlockMessage {
    int block; /* Block id */
    uint8_t state;
    uint64_t last;
    struct BlockMessage *next;
} BlockMessage;

static BlockMessage **message_table;

static uint8_t get_hash(uint32_t key) {
  key = key * 2654435761 & (LUAVB_BLOCK_MESSAGE_TABLE_SIZE - 1);
  return key;
}

int luaVB_init(lua_State *L) {
    message_table = luaM_malloc(L, sizeof(BlockMessage *) * LUAVB_BLOCK_MESSAGE_TABLE_SIZE);
    if (!message_table) {
        // Not enough memory
        return -1;
    }

    memset(message_table, 0, sizeof(BlockMessage *) * LUAVB_BLOCK_MESSAGE_TABLE_SIZE);

    return 0;
}

void luaVB_emitMessage(lua_State *L, int type, int id) {
    // Current message
    BlockMessage *current = NULL;

    // Get current time
    struct timeval now;

    gettimeofday(&now, NULL);

    // Convert to milliseconds
    uint64_t now_millis = now.tv_sec * 1000 + (now.tv_usec / 1000);

    if ((type == luaVB_BLOCK_START_MSG) || (type == luaVB_BLOCK_END_MSG)) {
        // Locate block message into block message table
        uint8_t hash = get_hash(id);

        if (!message_table[hash]) {
            // Has entry is empty, create the first entry, emit message always
            current = luaM_malloc(L, sizeof(BlockMessage));
            if (current) {
                memset(current, 0, sizeof(BlockMessage));

                message_table[hash] = current;

                // Store block id and message type
                current->block = id;
            } else {
                // Not enough memory, silent exit, and do not emit message
                return;
            }
        } else {
            // Search for message
            current = message_table[hash];
            while (current) {
                if (current->block == id) {
                    break;
                }
                current = current->next;
            }

            if (!current) {
                // Not found, create a new one
                current = luaM_malloc(L, sizeof(BlockMessage));
                if (current) {
                    memset(current, 0, sizeof(BlockMessage));

                    current->next = message_table[hash];
                    message_table[hash] = current;

                    // Store block id and message type
                    current->block = id;
                } else {
                    // Not enough memory, silent exit, and do not emit message
                    return;
                }
            }
        }
    }

    switch (type) {
        case luaVB_BLOCK_START_MSG:
            if ((current->state == luaVB_BLOCK_MSG_STATE_NONE) || (current->state == luaVB_BLOCK_MSG_STATE_END)) {
                if (now_millis - current->last < 200) {
                    return;
                }

                current->state = luaVB_BLOCK_MSG_STATE_START;
                current->last = now_millis;
                printf("<blockStart,%d>\r\n",id);
            }
            break;
        case luaVB_BLOCK_END_MSG:
            if (current->state == luaVB_BLOCK_MSG_STATE_START) {
                current->state = luaVB_BLOCK_MSG_STATE_END;
                current->last = now_millis;
                printf("<blockEnd,%d>\r\n",id);
            }
            break;
        case luaVB_BLOCK_ERR_CATCH_MSG:
            printf("<blockErrorCatched,%d>\r\n",id);
            break;
        case luaVB_BLOCK_ERR_MSG: {
			// Get error message from the stack
			const char *error_msg = lua_tostring(L, -1);

			// Parse error message
			int err;

			char * msg = parseErrMsg(error_msg, &err);
			if (msg) {
			    printf("<blockError,%d,%s>\r\n", id, msg);
			}

            break;
		}
    }
}
