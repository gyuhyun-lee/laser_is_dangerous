
#ifndef FOX_H
#define FOX_H

/*
    TODO:
    
    ARCHITECTURE EXPLORATION
    - Collision detection?
        - Entry/exit?
        - What's the plan for robustness / shape definition?
    - Implement multiple sim regions per frame
        - Per-entity clocking
        - Sim region merging?  For multiple players?
    - Z!
        - Clean up things by using v3
        - Figure out how you go "up" and "down", and how is this rendered?

    - Debug code
        - Logging
        - Diagramming
        - (A LITTLE GUI, but only a little!) Switches / sliders / etc.
        - Draw tile chunks so we can verify that things are algiend /
        
    - Audio
        - Sound effect triggers
        - Ambient sounds
        - Music
    - Asset streaming
        
    - Metagame / save game?
        - How do you enter "save slot"?
        - Persistent unlocks/etc.
        - Do we allow saved games?  Probably yes, just only for "pausing",
        * Continuous save for crash recovery?
    - Rudimentary world gen (no quality, just "what sorts of things" we do)
        - Placement of background things
        - Connectivity?
        - Non-overlapping?
        - Map display
        - Magnets - how they work???
    - AI
        - Rudimentary monstar behavior example
        * Pathfinding
        - AI "storage"
        
    * Animation, should probably lead into rendering
        - Skeletal animation
        - Particle systems

    PRODUCTION
    - Rendering
    -> GAME
        - Entity system
        - World generation
*/

#include "fox_platform.h"

#define Minimum(a, b) ((a < b) ? a : b)
#define Maximum(a, b) ((a > b)? a : b)

//TODO: Implement sine ourselves 
// because sine is pretty expensive because of precision purpose
#include <math.h>

struct memory_arena
{
    memory_index size;
    uint8 *base;
    memory_index used;

    int32 tempCount;
};

struct temporary_memory
{
    memory_index used;
    memory_arena *arena;
};

inline void
InitializeArena(memory_arena *arena, memory_index size, void *base)
{
    arena->size = size;
    // Base is the start of the memory
    arena->base = (uint8 *)base;
    arena->used = 0;
    arena->tempCount = 0;
}

inline temporary_memory
BeginTemporaryMemory(memory_arena *arena)
{
    temporary_memory result = {};

    result.arena = arena;
    result.used = arena->used;

    ++arena->tempCount;
    
    return result;
}

inline void
EndTemporaryMemory(temporary_memory memory)
{
    memory_arena *arena = memory.arena;
    Assert(arena->used >= memory.used);
    arena->used = memory.used;
    Assert(arena->tempCount > 0)
    --arena->tempCount;
}

inline void
CheckArena(memory_arena *arena)
{
    Assert(arena->tempCount == 0);
}

// NOTE : How much should I go to align by alignment?
inline memory_index
GetAlignmentOffset(memory_arena *arena, memory_index alignment)
{
    memory_index result = 0;

    memory_index resultPointer = (memory_index)arena->base + arena->used;
    memory_index alignmentMask = alignment - 1;
    // If I & this and the result is 0, it means theres no
    // 0th and 1th bit - which means aligned.
    // Otherwise, it is now aligned, so we should move the pointer 
    // by result to be aligned.
    if(resultPointer & alignmentMask)
    {
        result = alignment - (resultPointer & alignmentMask);
    }

    return result;
}
 
inline memory_index
GetArenaRemainingSize(memory_arena * arena, memory_index alignment=4)
{
    // Get the remaining size CONSIDERING the alignment.
    // For example, if there is 1 byte left, we cannot use it and start
    // after 1 byte because of the alignment.
    memory_index result = arena->size - 
        (arena->used + GetAlignmentOffset(arena, alignment));
    return result;
}

#define PushStruct(Arena, type, ...) (type *)PushSize_(Arena, sizeof(type), ## __VA_ARGS__)
#define PushArray(Arena, count, type, ...) (type *)PushSize_(Arena, count * sizeof(type), ##__VA_ARGS__)
#define PushSize(Arena, size, ...) PushSize_(Arena, size, ## __VA_ARGS__)

inline void *
PushSize_(memory_arena *arena, memory_index sizeInit, memory_index alignment = 4)
{
    memory_index size = sizeInit;

    memory_index alignmentOffset = GetAlignmentOffset(arena, alignment);
    size += alignmentOffset;

    Assert(arena->used + size <= arena->size);
    void *result = arena->base + arena->used;
    arena->used += size;

    Assert(size >= sizeInit);

    return result;
}

inline void
SubArena(memory_arena *result, memory_arena *arena, memory_index size, memory_index alignment = 16)
{
    result->size = size;
    result->base = (uint8 *)PushSize_(arena, size, alignment);
    result->used = 0;
    result->tempCount = 0;
}

#define ZeroStruct(instance) ZeroSize(sizeof(instance), &(instance))
// Zero memory
inline void
ZeroSize(memory_index size, void *ptr)
{
    // TODO : Check this guy for performance
    uint8 *byte = (uint8 *)ptr;
    while(size--)
    {
        *byte++ = 0;
    }
}

#include "fox_intrinsics.h"
#include "fox_math.h"
#include "fox_world.h"
#include "fox_particle.h"
#include "fox_sim_region.h"
#include "fox_render_group.h"
#include "fox_asset.h"
#include "fox_random.h" 

struct low_entity
{
    // TODO : It's kind of busted that pos can be invalid here,
    // but we store whether they would be invalid in the flags field,
    // so we have to check both.
    // Can we do something better here?
    world_position pos;

    // Sim region will _COPY_ this data, so this is not changed by the sim region
    sim_entity sim;
};

struct low_entity_index
{
    u32 storageIndex;
    low_entity_index *next;
};

struct controlled_hero
{
    uint32 entityIndex;

    v2 ddPlayer;
    
    real32 facingRad;
    real32 dfacingRad;
    v2 facingDirection;

    real32 dZ;

    bool32 isSpacePressed;
    b32 isRPressed;
    b32 isGPressed;

    r32 boost;
    r32 boostFuel;
    r32 maxBoostFuel;

    r32 attackFuel;
    r32 maxAttackFuel;

    r32 moveKeyPressTime;
};

// This is an external hash!
struct pairwise_collision_rule
{
    // This is the only rule we currently have
    bool32 canCollide;
    
    uint32 storageIndexA;
    // This is the target entity index
    uint32 storageIndexB;

    // This is an external hash!    
    pairwise_collision_rule *nextInHash;
};

struct ground_buffer
{
    // NOTE : This is the center of the bitmap
    // NOTE : If the value of this is NULL, it means it's not ready
    world_position pos;
    loaded_bitmap bitmap;
};

struct player_upgrade
{
    r32 attackFuelUpgrade;
    r32 laserDimUpgrade;
};

struct transient_state;

struct game_state
{
    bool32 isInitialized;

    bool32 isGamePaused;

    // What is this arena for?
    memory_arena worldArena;
    world *world;
    
    r32 typicalFloorHeight;

    // TODO : Should we allow split-sreen?
    uint32 cameraFollowingEntityIndex;
    world_position cameraPos;    
    
    controlled_hero controlledHeroes[ArrayCount(((game_input *)0)->controllers)];

    u32 lowEntityCount;
    // TODO : Find out what will be the exact number for this!
    low_entity lowEntities[10000];

    laser_spec_group *singleLaser;
    laser_spec_group *tripleLaser;
    laser_spec_group *backwardLaser;
    laser_spec_group *quadLaser;
    i32 laserGroupCount;
    laser_spec_group *enemySingleLaser;

    laser_spec_group *currentLaser;
    laser_spec_group *nextLaser;

    // TODO : Must be power of two because we are searching using bits
    // NOTE : This is external hash table as we store pointers, not entries!
    pairwise_collision_rule *collisionRules[256];
    pairwise_collision_rule *firstFreeCollisionRule;

    sim_entity_collision_volume_group *nullCollision;    
    sim_entity_collision_volume_group *swordCollision;
    sim_entity_collision_volume_group *stairCollision;
    sim_entity_collision_volume_group *playerCollision;
    sim_entity_collision_volume_group *monsterCollision;
    sim_entity_collision_volume_group *bigEnemyCollision;
    sim_entity_collision_volume_group *familiarCollision; 
    sim_entity_collision_volume_group *wallCollision;
    sim_entity_collision_volume_group *standardRoomCollision;    
    sim_entity_collision_volume_group *spongeCollision;    

    struct playing_sound *playingSounds;
    playing_sound *firstPlayingSound;
    playing_sound *firstFreePlayingSound;

    r32 maxTutorialDisplayTime;
    r32 tutorialDisplayTime;
    r32 maxDigipenLogoDisplayTime;
    r32 digipenLogoDisplayTime;

    // NOTE : For random enemy generation
    u32 roomMinTileX;
    u32 roomMinTileY;
    u32 roomMinTileZ;
    u32 roomMaxTileX;
    u32 roomMaxTileY;
    u32 roomMaxTileZ;
    v2 roomDim;
    u32 cameraTileX;
    u32 cameraTileY;
    u32 cameraTileZ;

    // NOTE : HUD Informations
    i32 waveCount;
    r32 remainingTimeForNextWave;

    b32 shouldBeRestarted;
    b32 shouldDisplayCredit;
};

struct background_task_with_memory
{
    bool32 beingUsed;
    memory_arena arena;

    // NOTE : This will and SHOULD be flused later!
    temporary_memory tempMemory;
};

#include "fox_asset.h"

struct transient_state
{
    bool32 isInitialized;

    memory_arena tranArena;
    
    background_task_with_memory backgroundTasks[4];

    uint32 groundBufferCount;
    ground_buffer *groundBuffers;

    int32 envMapWidth;
    int32 envMapHeight;
    // 1 : bottom ,2 : middle, 3 : top
    enviromnet_map envMaps[3];

    platform_work_queue *highPriorityQueue;
    platform_work_queue *lowPriorityQueue;

    // TODO : Should this be here?
    platform_work_queue *renderQueue;

    game_assets *assets;
};

// TODO : Remove thiese global variables?
global_variable platform_add_entry *PlatformAddEntry;
global_variable platform_complete_all_work *PlatformCompleteAllWork;
global_variable debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;
global_variable debug_platform_free_file_memory *DEBUGPlatformFreeEntireFile;
global_variable random_series DEBUGSeries;

internal background_task_with_memory *BeginBackgroundTaskWithMemory(transient_state *tranState);

internal void EndBackgroundTaskWithMemory(background_task_with_memory *task);

#endif