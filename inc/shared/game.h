/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma once

#include "shared/list.h"

//
// game.h -- game dll information visible to server
//

#define GAME_API_VERSION_OLD    3       // game uses gclient_old_t, pmove_old_t
#define GAME_API_VERSION_NEW    3302    // game uses gclient_new_t, pmove_new_t

#if USE_NEW_GAME_API
#define GAME_API_VERSION    GAME_API_VERSION_NEW
#else
#define GAME_API_VERSION    GAME_API_VERSION_OLD
#endif

// edict->svflags

#define SVF_NOCLIENT            BIT(0)      // don't send entity to clients, even if it has effects
#define SVF_DEADMONSTER         BIT(1)      // treat as CONTENTS_DEADMONSTER for collision
#define SVF_MONSTER             BIT(2)      // only used by server as entity priority hint

#if USE_PROTOCOL_EXTENSIONS
#define SVF_PLAYER              BIT(3)      // treat as CONTENTS_PLAYER for collision
#define SVF_BOT                 BIT(4)
#define SVF_NOBOTS              BIT(5)
#define SVF_RESPAWNING          BIT(6)
#define SVF_PROJECTILE          BIT(7)      // treat as CONTENTS_PROJECTILE for collision
#define SVF_INSTANCED           BIT(8)
#define SVF_DOOR                BIT(9)
#define SVF_NOCULL              BIT(10)     // always send entity to clients (no PVS checks)
#define SVF_HULL                BIT(11)
#endif

// edict->solid values

typedef enum {
    SOLID_NOT,          // no interaction with other objects
    SOLID_TRIGGER,      // only touch when inside, after moving
    SOLID_BBOX,         // touch on edge
    SOLID_BSP           // bsp clip, touch on edge
} solid_t;

// extended features

// R1Q2 and Q2PRO specific
#define GMF_CLIENTNUM               BIT(0)      // game sets clientNum gclient_s field
#define GMF_PROPERINUSE             BIT(1)      // game maintains edict_s inuse field properly
#define GMF_MVDSPEC                 BIT(2)      // game is dummy MVD client aware
#define GMF_WANT_ALL_DISCONNECTS    BIT(3)      // game wants ClientDisconnect() for non-spawned clients

// Q2PRO specific
#define GMF_ENHANCED_SAVEGAMES      BIT(10)     // game supports safe/portable savegames
#define GMF_VARIABLE_FPS            BIT(11)     // game supports variable server FPS
#define GMF_EXTRA_USERINFO          BIT(12)     // game wants extra userinfo after normal userinfo
#define GMF_IPV6_ADDRESS_AWARE      BIT(13)     // game supports IPv6 addresses
#define GMF_ALLOW_INDEX_OVERFLOW    BIT(14)     // game wants PF_FindIndex() to return 0 on overflow
#define GMF_PROTOCOL_EXTENSIONS     BIT(15)     // game supports protocol extensions

//===============================================================

#define MAX_ENT_CLUSTERS    16

typedef struct edict_s edict_t;
typedef struct gclient_s gclient_t;

#ifndef GAME_INCLUDE

typedef struct gclient_old_s gclient_old_t;
typedef struct gclient_new_s gclient_new_t;

struct gclient_old_s {
    player_state_old_t  ps;     // communicated by server to clients
    int                 ping;

    // set to (client POV entity number) - 1 by game,
    // only valid if g_features has GMF_CLIENTNUM bit
    int             clientNum;

    // the game dll can add anything it wants after
    // this point in the structure
};

struct gclient_new_s {
    player_state_new_t  ps;     // communicated by server to clients
    int                 ping;

    // set to (client POV entity number) - 1 by game,
    // only valid if g_features has GMF_CLIENTNUM bit
    int             clientNum;

    // the game dll can add anything it wants after
    // this point in the structure
};

struct edict_s {
    entity_state_t  s;
    void        *client;
    qboolean    inuse;
    int         linkcount;

    // FIXME: move these fields to a server private sv_entity_t
    list_t      area;               // linked to a division node or leaf

    int         num_clusters;       // if -1, use headnode instead
    int         clusternums[MAX_ENT_CLUSTERS];
    int         headnode;           // unused if num_clusters != -1
    int         areanum, areanum2;

    //================================

    int         svflags;            // SVF_NOCLIENT, SVF_DEADMONSTER, SVF_MONSTER, etc
    vec3_t      mins, maxs;
    vec3_t      absmin, absmax, size;
    solid_t     solid;
    int         clipmask;
    edict_t     *owner;

    //================================

#if USE_PROTOCOL_EXTENSIONS
    // extra entity state communicated to clients
    // only valid if g_features has GMF_PROTOCOL_EXTENSIONS bit
    entity_state_extension_t    x;
#endif

    // the game dll can add anything it wants after
    // this point in the structure
};

#endif      // GAME_INCLUDE

//===============================================================

//
// functions provided by the main engine
//
typedef struct {
    // special messages
    void (* q_printf(2, 3) bprintf)(int printlevel, const char *fmt, ...);
    void (* q_printf(1, 2) dprintf)(const char *fmt, ...);
    void (* q_printf(3, 4) cprintf)(edict_t *ent, int printlevel, const char *fmt, ...);
    void (* q_printf(2, 3) centerprintf)(edict_t *ent, const char *fmt, ...);
    void (*sound)(edict_t *ent, int channel, int soundindex, float volume, float attenuation, float timeofs);
    void (*positioned_sound)(const vec3_t origin, edict_t *ent, int channel, int soundindex, float volume, float attenuation, float timeofs);

    // config strings hold all the index strings, the lightstyles,
    // and misc data like the sky definition and cdtrack.
    // All of the current configstrings are sent to clients when
    // they connect, and changes are sent to all connected clients.
    void (*configstring)(int num, const char *string);

    void (* q_noreturn_ptr q_printf(1, 2) error)(const char *fmt, ...);

    // the *index functions create configstrings and some internal server state
    int (*modelindex)(const char *name);
    int (*soundindex)(const char *name);
    int (*imageindex)(const char *name);

    void (*setmodel)(edict_t *ent, const char *name);

    // collision detection
    trace_t (* q_gameabi trace)(const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, edict_t *passent, int contentmask);
    int (*pointcontents)(const vec3_t point);
    qboolean (*inPVS)(const vec3_t p1, const vec3_t p2);
    qboolean (*inPHS)(const vec3_t p1, const vec3_t p2);
    void (*SetAreaPortalState)(int portalnum, qboolean open);
    qboolean (*AreasConnected)(int area1, int area2);

    // an entity will never be sent to a client or used for collision
    // if it is not passed to linkentity.  If the size, position, or
    // solidity changes, it must be relinked.
    void (*linkentity)(edict_t *ent);
    void (*unlinkentity)(edict_t *ent);     // call before removing an interactive edict
    int (*BoxEdicts)(const vec3_t mins, const vec3_t maxs, edict_t **list, int maxcount, int areatype);
#ifdef GAME_INCLUDE
    void (*Pmove)(pmove_t *pmove);          // player movement code common with client prediction
#else
    void (*Pmove)(void *pmove);
#endif

    // network messaging
    void (*multicast)(const vec3_t origin, multicast_t to);
    void (*unicast)(edict_t *ent, qboolean reliable);
    void (*WriteChar)(int c);
    void (*WriteByte)(int c);
    void (*WriteShort)(int c);
    void (*WriteLong)(int c);
    void (*WriteFloat)(float f);
    void (*WriteString)(const char *s);
    void (*WritePosition)(const vec3_t pos);    // some fractional bits
    void (*WriteDir)(const vec3_t pos);         // single byte encoded, very coarse
    void (*WriteAngle)(float f);

    // managed memory allocation
    void *(*TagMalloc)(unsigned size, unsigned tag);
    void (*TagFree)(void *block);
    void (*FreeTags)(unsigned tag);

    // console variable interaction
    cvar_t *(*cvar)(const char *var_name, const char *value, int flags);
    cvar_t *(*cvar_set)(const char *var_name, const char *value);
    cvar_t *(*cvar_forceset)(const char *var_name, const char *value);

    // ClientCommand and ServerCommand parameter access
    int (*argc)(void);
    char *(*argv)(int n);
    char *(*args)(void);     // concatenation of all argv >= 1

    // add commands to the server console as if they were typed in
    // for map changing, etc
    void (*AddCommandString)(const char *text);

    void (*DebugGraph)(float value, int color);
} game_import_t;

//
// functions exported by the game subsystem
//
typedef struct {
    int         apiversion;

    // the init function will only be called when a game starts,
    // not each time a level is loaded.  Persistent data for clients
    // and the server can be allocated in init
    void (*Init)(void);
    void (*Shutdown)(void);

    // each new level entered will cause a call to SpawnEntities
    void (*SpawnEntities)(const char *mapname, const char *entstring, const char *spawnpoint);

    // Read/Write Game is for storing persistent cross level information
    // about the world state and the clients.
    // WriteGame is called every time a level is exited.
    // ReadGame is called on a loadgame.
    void (*WriteGame)(const char *filename, qboolean autosave);
    void (*ReadGame)(const char *filename);

    // ReadLevel is called after the default map information has been
    // loaded with SpawnEntities
    void (*WriteLevel)(const char *filename);
    void (*ReadLevel)(const char *filename);

    qboolean (*ClientConnect)(edict_t *ent, char *userinfo);
    void (*ClientBegin)(edict_t *ent);
    void (*ClientUserinfoChanged)(edict_t *ent, char *userinfo);
    void (*ClientDisconnect)(edict_t *ent);
    void (*ClientCommand)(edict_t *ent);
    void (*ClientThink)(edict_t *ent, usercmd_t *cmd);

    void (*RunFrame)(void);

    // ServerCommand will be called when an "sv <command>" command is issued on the
    // server console.
    // The game can issue gi.argc() / gi.argv() commands to get the rest
    // of the parameters
    void (*ServerCommand)(void);

    //
    // global variables shared between game and server
    //

    // The edict array is allocated in the game dll so it
    // can vary in size from one game to another.
    //
    // The size will be fixed when ge->Init() is called
    struct edict_s  *edicts;
    int         edict_size;
    int         num_edicts;     // current number, <= max_edicts
    int         max_edicts;
} game_export_t;

typedef game_export_t *(*game_entry_t)(game_import_t *);
