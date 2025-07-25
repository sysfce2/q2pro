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
#include "g_local.h"
#include "m_player.h"

static char *ClientTeam(edict_t *ent)
{
    char        *p;
    static char value[MAX_INFO_STRING];

    value[0] = 0;

    if (!ent->client)
        return value;

    strcpy(value, Info_ValueForKey(ent->client->pers.userinfo, "skin"));
    p = strchr(value, '/');
    if (!p)
        return value;

    if ((int)(dmflags->value) & DF_MODELTEAMS) {
        *p = 0;
        return value;
    }

    // if ((int)(dmflags->value) & DF_SKINTEAMS)
    return ++p;
}

bool OnSameTeam(edict_t *ent1, edict_t *ent2)
{
    char    ent1Team[MAX_INFO_STRING];
    char    ent2Team[MAX_INFO_STRING];

    if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
        return false;

    strcpy(ent1Team, ClientTeam(ent1));
    strcpy(ent2Team, ClientTeam(ent2));

    if (strcmp(ent1Team, ent2Team) == 0)
        return true;
    return false;
}

static void SelectNextItem(edict_t *ent, int itflags)
{
    gclient_t   *cl;
    int         i, index;
    const gitem_t   *it;

    cl = ent->client;

    if (cl->chase_target) {
        ChaseNext(ent);
        return;
    }

    // scan  for the next valid one
    for (i = 1; i <= game.num_items; i++) {
        index = (cl->pers.selected_item + i) % game.num_items;
        if (!cl->pers.inventory[index])
            continue;
        it = &itemlist[index];
        if (!it->use)
            continue;
        if (!(it->flags & itflags))
            continue;

        cl->pers.selected_item = index;
        return;
    }

    cl->pers.selected_item = -1;
}

static void SelectPrevItem(edict_t *ent, int itflags)
{
    gclient_t   *cl;
    int         i, index;
    const gitem_t   *it;

    cl = ent->client;

    if (cl->chase_target) {
        ChasePrev(ent);
        return;
    }

    // scan  for the next valid one
    for (i = 1; i <= game.num_items; i++) {
        index = (cl->pers.selected_item + game.num_items - i) % game.num_items;
        if (!cl->pers.inventory[index])
            continue;
        it = &itemlist[index];
        if (!it->use)
            continue;
        if (!(it->flags & itflags))
            continue;

        cl->pers.selected_item = index;
        return;
    }

    cl->pers.selected_item = -1;
}

void ValidateSelectedItem(edict_t *ent)
{
    gclient_t   *cl;

    cl = ent->client;

    if (cl->pers.inventory[cl->pers.selected_item])
        return;     // valid

    SelectNextItem(ent, -1);
}

//=================================================================================

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
static void Cmd_Give_f(edict_t *ent)
{
    char        *name;
    const gitem_t   *it;
    int         index;
    int         i;
    bool        give_all;
    edict_t     *it_ent;

    if ((deathmatch->value || coop->value) && !sv_cheats->value) {
        gi.cprintf(ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
        return;
    }

    name = gi.args();

    if (Q_stricmp(name, "all") == 0)
        give_all = true;
    else
        give_all = false;

    if (give_all || Q_stricmp(gi.argv(1), "health") == 0) {
        if (gi.argc() == 3)
            ent->health = Q_atoi(gi.argv(2));
        else
            ent->health = ent->max_health;
        if (!give_all)
            return;
    }

    if (give_all || Q_stricmp(name, "weapons") == 0) {
        for (i = 0; i < game.num_items; i++) {
            it = itemlist + i;
            if (!it->pickup)
                continue;
            if (!(it->flags & IT_WEAPON))
                continue;
            ent->client->pers.inventory[i] += 1;
        }
        if (!give_all)
            return;
    }

    if (give_all || Q_stricmp(name, "ammo") == 0) {
        for (i = 0; i < game.num_items; i++) {
            it = itemlist + i;
            if (!it->pickup)
                continue;
            if (!(it->flags & IT_AMMO))
                continue;
            Add_Ammo(ent, it, 1000);
        }
        if (!give_all)
            return;
    }

    if (give_all || Q_stricmp(name, "armor") == 0) {
        const gitem_armor_t *info;

        it = FindItem("Jacket Armor");
        ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

        it = FindItem("Combat Armor");
        ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

        it = FindItem("Body Armor");
        info = (const gitem_armor_t *)it->info;
        ent->client->pers.inventory[ITEM_INDEX(it)] = info->max_count;

        if (!give_all)
            return;
    }

    if (give_all || Q_stricmp(name, "Power Shield") == 0) {
        it = FindItem("Power Shield");
        it_ent = G_Spawn();
        it_ent->classname = it->classname;
        SpawnItem(it_ent, it);
        Touch_Item(it_ent, ent, NULL, NULL);
        if (it_ent->inuse)
            G_FreeEdict(it_ent);

        if (!give_all)
            return;
    }

    if (give_all) {
        for (i = 0; i < game.num_items; i++) {
            it = itemlist + i;
            if (!it->pickup)
                continue;
            if (it->flags & (IT_ARMOR | IT_WEAPON | IT_AMMO))
                continue;
            ent->client->pers.inventory[i] = 1;
        }
        return;
    }

    it = FindItem(name);
    if (!it) {
        name = gi.argv(1);
        it = FindItem(name);
        if (!it) {
            gi.cprintf(ent, PRINT_HIGH, "unknown item\n");
            return;
        }
    }

    if (!it->pickup) {
        gi.cprintf(ent, PRINT_HIGH, "non-pickup item\n");
        return;
    }

    index = ITEM_INDEX(it);

    if (it->flags & IT_AMMO) {
        if (gi.argc() == 3)
            ent->client->pers.inventory[index] = Q_atoi(gi.argv(2));
        else
            ent->client->pers.inventory[index] += it->quantity;
    } else {
        it_ent = G_Spawn();
        it_ent->classname = it->classname;
        SpawnItem(it_ent, it);
        Touch_Item(it_ent, ent, NULL, NULL);
        if (it_ent->inuse)
            G_FreeEdict(it_ent);
    }
}

/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
static void Cmd_God_f(edict_t *ent)
{
    if ((deathmatch->value || coop->value) && !sv_cheats->value) {
        gi.cprintf(ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
        return;
    }

    ent->flags ^= FL_GODMODE;
    if (!(ent->flags & FL_GODMODE))
        gi.cprintf(ent, PRINT_HIGH, "godmode OFF\n");
    else
        gi.cprintf(ent, PRINT_HIGH, "godmode ON\n");
}

/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
static void Cmd_Notarget_f(edict_t *ent)
{
    if ((deathmatch->value || coop->value) && !sv_cheats->value) {
        gi.cprintf(ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
        return;
    }

    ent->flags ^= FL_NOTARGET;
    if (!(ent->flags & FL_NOTARGET))
        gi.cprintf(ent, PRINT_HIGH, "notarget OFF\n");
    else
        gi.cprintf(ent, PRINT_HIGH, "notarget ON\n");
}

/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
static void Cmd_Noclip_f(edict_t *ent)
{
    if ((deathmatch->value || coop->value) && !sv_cheats->value) {
        gi.cprintf(ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
        return;
    }

    if (ent->movetype == MOVETYPE_NOCLIP) {
        ent->movetype = MOVETYPE_WALK;
        gi.cprintf(ent, PRINT_HIGH, "noclip OFF\n");
    } else {
        ent->movetype = MOVETYPE_NOCLIP;
        gi.cprintf(ent, PRINT_HIGH, "noclip ON\n");
    }
}

/*
==================
Cmd_Use_f

Use an inventory item
==================
*/
static void Cmd_Use_f(edict_t *ent)
{
    int         index;
    const gitem_t   *it;
    char        *s;

    s = gi.args();
    it = FindItem(s);
    if (!it) {
        gi.cprintf(ent, PRINT_HIGH, "unknown item: %s\n", s);
        return;
    }
    if (!it->use) {
        gi.cprintf(ent, PRINT_HIGH, "Item is not usable.\n");
        return;
    }
    index = ITEM_INDEX(it);
    if (!ent->client->pers.inventory[index]) {
        gi.cprintf(ent, PRINT_HIGH, "Out of item: %s\n", s);
        return;
    }

    it->use(ent, it);
}

/*
==================
Cmd_Drop_f

Drop an inventory item
==================
*/
static void Cmd_Drop_f(edict_t *ent)
{
    int         index;
    const gitem_t   *it;
    char        *s;

    s = gi.args();
    it = FindItem(s);
    if (!it) {
        gi.cprintf(ent, PRINT_HIGH, "unknown item: %s\n", s);
        return;
    }
    if (!it->drop || ((coop->value) && (it->flags & IT_STAY_COOP))) {
        gi.cprintf(ent, PRINT_HIGH, "Item is not droppable.\n");
        return;
    }
    index = ITEM_INDEX(it);
    if (!ent->client->pers.inventory[index]) {
        gi.cprintf(ent, PRINT_HIGH, "Out of item: %s\n", s);
        return;
    }

    it->drop(ent, it);
}

/*
=================
Cmd_Inven_f
=================
*/
static void Cmd_Inven_f(edict_t *ent)
{
    int         i;
    gclient_t   *cl;

    cl = ent->client;

    cl->showscores = false;
    cl->showhelp = false;

    if (cl->showinventory) {
        cl->showinventory = false;
        return;
    }

    cl->showinventory = true;

    gi.WriteByte(svc_inventory);
    for (i = 0; i < MAX_ITEMS; i++) {
        gi.WriteShort(cl->pers.inventory[i]);
    }
    gi.unicast(ent, true);
}

/*
=================
Cmd_InvUse_f
=================
*/
static void Cmd_InvUse_f(edict_t *ent)
{
    const gitem_t   *it;

    ValidateSelectedItem(ent);

    if (ent->client->pers.selected_item == -1) {
        gi.cprintf(ent, PRINT_HIGH, "No item to use.\n");
        return;
    }

    it = &itemlist[ent->client->pers.selected_item];
    if (!it->use) {
        gi.cprintf(ent, PRINT_HIGH, "Item is not usable.\n");
        return;
    }
    it->use(ent, it);
}

/*
=================
Cmd_WeapPrev_f
=================
*/
static void Cmd_WeapPrev_f(edict_t *ent)
{
    gclient_t   *cl;
    int         i, index;
    const gitem_t   *it;
    int         selected_weapon;

    cl = ent->client;

    if (!cl->pers.weapon)
        return;

    selected_weapon = ITEM_INDEX(cl->pers.weapon);

    // scan  for the next valid one
    for (i = 1; i <= game.num_items; i++) {
        index = (selected_weapon + i) % game.num_items;
        if (!cl->pers.inventory[index])
            continue;
        it = &itemlist[index];
        if (!it->use)
            continue;
        if (!(it->flags & IT_WEAPON))
            continue;
        it->use(ent, it);
        if (cl->pers.weapon == it)
            return; // successful
    }
}

/*
=================
Cmd_WeapNext_f
=================
*/
static void Cmd_WeapNext_f(edict_t *ent)
{
    gclient_t   *cl;
    int         i, index;
    const gitem_t   *it;
    int         selected_weapon;

    cl = ent->client;

    if (!cl->pers.weapon)
        return;

    selected_weapon = ITEM_INDEX(cl->pers.weapon);

    // scan  for the next valid one
    for (i = 1; i <= game.num_items; i++) {
        index = (selected_weapon + game.num_items - i) % game.num_items;
        if (!cl->pers.inventory[index])
            continue;
        it = &itemlist[index];
        if (!it->use)
            continue;
        if (!(it->flags & IT_WEAPON))
            continue;
        it->use(ent, it);
        if (cl->pers.weapon == it)
            return; // successful
    }
}

/*
=================
Cmd_WeapLast_f
=================
*/
static void Cmd_WeapLast_f(edict_t *ent)
{
    gclient_t   *cl;
    int         index;
    const gitem_t   *it;

    cl = ent->client;

    if (!cl->pers.weapon || !cl->pers.lastweapon)
        return;

    index = ITEM_INDEX(cl->pers.lastweapon);
    if (!cl->pers.inventory[index])
        return;
    it = &itemlist[index];
    if (!it->use)
        return;
    if (!(it->flags & IT_WEAPON))
        return;
    it->use(ent, it);
}

/*
=================
Cmd_InvDrop_f
=================
*/
static void Cmd_InvDrop_f(edict_t *ent)
{
    const gitem_t   *it;

    ValidateSelectedItem(ent);

    if (ent->client->pers.selected_item == -1) {
        gi.cprintf(ent, PRINT_HIGH, "No item to drop.\n");
        return;
    }

    it = &itemlist[ent->client->pers.selected_item];
    if (!it->drop || ((coop->value) && (it->flags & IT_STAY_COOP))) {
        gi.cprintf(ent, PRINT_HIGH, "Item is not droppable.\n");
        return;
    }
    it->drop(ent, it);
}

/*
=================
Cmd_Kill_f
=================
*/
static void Cmd_Kill_f(edict_t *ent)
{
    if ((level.framenum - ent->client->respawn_framenum) < 5 * BASE_FRAMERATE)
        return;
    ent->flags &= ~FL_GODMODE;
    ent->health = 0;
    meansOfDeath = MOD_SUICIDE;
    player_die(ent, ent, ent, 100000, ent->s.origin);
}

/*
=================
Cmd_PutAway_f
=================
*/
static void Cmd_PutAway_f(edict_t *ent)
{
    ent->client->showscores = false;
    ent->client->showhelp = false;
    ent->client->showinventory = false;
}

static int PlayerSort(void const *a, void const *b)
{
    int     anum, bnum;

    anum = *(int *)a;
    bnum = *(int *)b;

    anum = game.clients[anum].ps.stats[STAT_FRAGS];
    bnum = game.clients[bnum].ps.stats[STAT_FRAGS];

    if (anum < bnum)
        return -1;
    if (anum > bnum)
        return 1;
    return 0;
}

/*
=================
Cmd_Players_f
=================
*/
static void Cmd_Players_f(edict_t *ent)
{
    int     i;
    int     count;
    char    small[64];
    char    large[1280];
    int     index[MAX_CLIENTS];

    count = 0;
    for (i = 0; i < game.maxclients; i++)
        if (game.clients[i].pers.connected) {
            index[count] = i;
            count++;
        }

    // sort by frags
    qsort(index, count, sizeof(index[0]), PlayerSort);

    // print information
    large[0] = 0;

    for (i = 0; i < count; i++) {
        Q_snprintf(small, sizeof(small), "%3i %s\n",
                   game.clients[index[i]].ps.stats[STAT_FRAGS],
                   game.clients[index[i]].pers.netname);
        if (strlen(small) + strlen(large) > sizeof(large) - 100) {
            // can't print all of them in one packet
            strcat(large, "...\n");
            break;
        }
        strcat(large, small);
    }

    gi.cprintf(ent, PRINT_HIGH, "%s\n%i players\n", large, count);
}

/*
=================
Cmd_Wave_f
=================
*/
static void Cmd_Wave_f(edict_t *ent)
{
    int     i;

    i = Q_atoi(gi.argv(1));

    // can't wave when ducked
    if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
        return;

    if (ent->client->anim_priority > ANIM_WAVE)
        return;

    ent->client->anim_priority = ANIM_WAVE;

    switch (i) {
    case 0:
        gi.cprintf(ent, PRINT_HIGH, "flipoff\n");
        ent->s.frame = FRAME_flip01 - 1;
        ent->client->anim_end = FRAME_flip12;
        break;
    case 1:
        gi.cprintf(ent, PRINT_HIGH, "salute\n");
        ent->s.frame = FRAME_salute01 - 1;
        ent->client->anim_end = FRAME_salute11;
        break;
    case 2:
        gi.cprintf(ent, PRINT_HIGH, "taunt\n");
        ent->s.frame = FRAME_taunt01 - 1;
        ent->client->anim_end = FRAME_taunt17;
        break;
    case 3:
        gi.cprintf(ent, PRINT_HIGH, "wave\n");
        ent->s.frame = FRAME_wave01 - 1;
        ent->client->anim_end = FRAME_wave11;
        break;
    case 4:
    default:
        gi.cprintf(ent, PRINT_HIGH, "point\n");
        ent->s.frame = FRAME_point01 - 1;
        ent->client->anim_end = FRAME_point12;
        break;
    }
}

static bool FloodProtect(edict_t *ent)
{
    int i, msgs = flood_msgs->value;
    gclient_t *cl = ent->client;

    if (msgs < 1)
        return false;

    if (level.time < cl->flood_locktill) {
        gi.cprintf(ent, PRINT_HIGH, "You can't talk for %d more seconds\n",
                   (int)(cl->flood_locktill - level.time));
        return true;
    }

    i = cl->flood_whenhead - min(msgs, FLOOD_MSGS) + 1;
    if (i < 0)
        i += FLOOD_MSGS;
    if (cl->flood_when[i] &&
        level.time - cl->flood_when[i] < flood_persecond->value) {
        cl->flood_locktill = level.time + flood_waitdelay->value;
        gi.cprintf(ent, PRINT_CHAT, "Flood protection:  You can't talk for %d seconds.\n",
                   (int)flood_waitdelay->value);
        return true;
    }

    cl->flood_whenhead = (cl->flood_whenhead + 1) % FLOOD_MSGS;
    cl->flood_when[cl->flood_whenhead] = level.time;
    return false;
}

/*
==================
Cmd_Say_f
==================
*/
static void Cmd_Say_f(edict_t *ent, bool team, bool arg0)
{
    int     j;
    edict_t *other;
    char    text[2048];

    if (gi.argc() < 2 && !arg0)
        return;

    if (FloodProtect(ent))
        return;

    if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
        team = false;

    if (team)
        Q_snprintf(text, sizeof(text), "(%s): ", ent->client->pers.netname);
    else
        Q_snprintf(text, sizeof(text), "%s: ", ent->client->pers.netname);

    if (arg0) {
        Q_strlcat(text, gi.argv(0), sizeof(text));
        Q_strlcat(text, " ", sizeof(text));
        Q_strlcat(text, gi.args(), sizeof(text));
    } else {
        Q_strlcat(text, COM_StripQuotes(gi.args()), sizeof(text));
    }

    // don't let text be too long for malicious reasons
    if (strlen(text) > 150)
        text[150] = 0;

    Q_strlcat(text, "\n", sizeof(text));

    if (dedicated->value)
        gi.cprintf(NULL, PRINT_CHAT, "%s", text);

    for (j = 1; j <= game.maxclients; j++) {
        other = &g_edicts[j];
        if (!other->inuse)
            continue;
        if (!other->client)
            continue;
        if (team) {
            if (!OnSameTeam(ent, other))
                continue;
        }
        gi.cprintf(other, PRINT_CHAT, "%s", text);
    }
}

static void Cmd_PlayerList_f(edict_t *ent)
{
    int i;
    char st[80];
    char text[1400];
    edict_t *e2;

    // connect time, ping, score, name
    *text = 0;
    for (i = 0, e2 = g_edicts + 1; i < game.maxclients; i++, e2++) {
        if (!e2->inuse)
            continue;

        Q_snprintf(st, sizeof(st), "%02d:%02d %4d %3d %s%s\n",
                   (level.framenum - e2->client->resp.enterframe) / 600,
                   ((level.framenum - e2->client->resp.enterframe) % 600) / 10,
                   e2->client->ping,
                   e2->client->resp.score,
                   e2->client->pers.netname,
                   e2->client->resp.spectator ? " (spectator)" : "");
        if (strlen(text) + strlen(st) > sizeof(text) - 50) {
            if (strlen(text) < sizeof(text) - 12)
                strcat(text, "And more...\n");
            gi.cprintf(ent, PRINT_HIGH, "%s", text);
            return;
        }
        strcat(text, st);
    }
    gi.cprintf(ent, PRINT_HIGH, "%s", text);
}

/*
=================
ClientCommand
=================
*/
void ClientCommand(edict_t *ent)
{
    char    *cmd;

    if (!ent->client)
        return;     // not fully in game yet

    cmd = gi.argv(0);

    if (Q_stricmp(cmd, "players") == 0) {
        Cmd_Players_f(ent);
        return;
    }
    if (Q_stricmp(cmd, "say") == 0) {
        Cmd_Say_f(ent, false, false);
        return;
    }
    if (Q_stricmp(cmd, "say_team") == 0) {
        Cmd_Say_f(ent, true, false);
        return;
    }
    if (Q_stricmp(cmd, "score") == 0) {
        Cmd_Score_f(ent);
        return;
    }
    if (Q_stricmp(cmd, "help") == 0) {
        Cmd_Help_f(ent);
        return;
    }

    if (level.intermission_framenum)
        return;

    if (Q_stricmp(cmd, "use") == 0)
        Cmd_Use_f(ent);
    else if (Q_stricmp(cmd, "drop") == 0)
        Cmd_Drop_f(ent);
    else if (Q_stricmp(cmd, "give") == 0)
        Cmd_Give_f(ent);
    else if (Q_stricmp(cmd, "god") == 0)
        Cmd_God_f(ent);
    else if (Q_stricmp(cmd, "notarget") == 0)
        Cmd_Notarget_f(ent);
    else if (Q_stricmp(cmd, "noclip") == 0)
        Cmd_Noclip_f(ent);
    else if (Q_stricmp(cmd, "inven") == 0)
        Cmd_Inven_f(ent);
    else if (Q_stricmp(cmd, "invnext") == 0)
        SelectNextItem(ent, -1);
    else if (Q_stricmp(cmd, "invprev") == 0)
        SelectPrevItem(ent, -1);
    else if (Q_stricmp(cmd, "invnextw") == 0)
        SelectNextItem(ent, IT_WEAPON);
    else if (Q_stricmp(cmd, "invprevw") == 0)
        SelectPrevItem(ent, IT_WEAPON);
    else if (Q_stricmp(cmd, "invnextp") == 0)
        SelectNextItem(ent, IT_POWERUP);
    else if (Q_stricmp(cmd, "invprevp") == 0)
        SelectPrevItem(ent, IT_POWERUP);
    else if (Q_stricmp(cmd, "invuse") == 0)
        Cmd_InvUse_f(ent);
    else if (Q_stricmp(cmd, "invdrop") == 0)
        Cmd_InvDrop_f(ent);
    else if (Q_stricmp(cmd, "weapprev") == 0)
        Cmd_WeapPrev_f(ent);
    else if (Q_stricmp(cmd, "weapnext") == 0)
        Cmd_WeapNext_f(ent);
    else if (Q_stricmp(cmd, "weaplast") == 0)
        Cmd_WeapLast_f(ent);
    else if (Q_stricmp(cmd, "kill") == 0)
        Cmd_Kill_f(ent);
    else if (Q_stricmp(cmd, "putaway") == 0)
        Cmd_PutAway_f(ent);
    else if (Q_stricmp(cmd, "wave") == 0)
        Cmd_Wave_f(ent);
    else if (Q_stricmp(cmd, "playerlist") == 0)
        Cmd_PlayerList_f(ent);
    else    // anything that doesn't match a command will be a chat
        Cmd_Say_f(ent, false, true);
}
