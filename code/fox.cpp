#include <time.h>
#include "fox.h"
#include "fox_random.h"
#include "fox_sim_region.h"
#include "fox_entity.h"
#include "fox_asset.h"
#include "fox_world.cpp"

#include "fox_render_group.cpp"
#include "fox_sim_region.cpp"
#include "fox_asset.cpp"
#include "fox_particle.cpp"
#include "fox_raycast2d.cpp"

// TODO : This is only for the world building! Remove it completely later.
inline world_position
TilePositionToChunkPosition(world *world, int32 absTileX, int32 absTileY, int32 absTileZ, 
    v3 additionalOffset = V3(0, 0, 0))
{
    world_position basePos = {};

    real32 tileSideInMeters = 1.4f;
    real32 tileDeptInMeters = 3.0f;

    v3 tileDim = V3(tileSideInMeters, tileSideInMeters, tileDeptInMeters);
    v3 offset = Hadamard(tileDim, V3((real32)absTileX, (real32)absTileY, (real32)absTileZ));

    // Recanonicalize this value
    world_position result = MapIntoChunkSpace(world, basePos, offset + additionalOffset);

    Assert(IsCanonical(world, result.offset_));

    return result;
}

struct add_low_entity_result
{
    uint32 storageIndex;
    low_entity *low;
};

// This function adds new low entity AND put it to the entity block based on the chunk
// where the entity is.
internal add_low_entity_result
AddLowEntity(game_state *gameState, entity_type type, world_position worldPosition)
{
    Assert(gameState->lowEntityCount < ArrayCount(gameState->lowEntities));
    uint32 lowIndex = gameState->lowEntityCount++;

    low_entity *low = gameState->lowEntities + lowIndex;
    *low = {};
    low->sim.type = type;

    low->pos = NullPosition();

    ChangeEntityLocation(&gameState->worldArena, gameState->world, low, lowIndex, worldPosition);
    
    add_low_entity_result result = {};
    result.low = low;
    result.storageIndex = lowIndex;

    return result;
}

// Draw grounded low entities
internal add_low_entity_result
AddGroundedLowEntity(game_state *gameState, entity_type type, world_position worldPosition,
    sim_entity_collision_volume_group *collisionGroup)
{
    add_low_entity_result result = AddLowEntity(gameState, type, worldPosition);
    result.low->sim.collision = collisionGroup;
    result.low->sim.alpha = 1.0f;

    return result;
}

internal void
InitHitPoints(low_entity *low, uint32 hitPointCount)
{
    low->sim.hitPointMax = hitPointCount;
    // Assert(hitPointCount < ArrayCount(low->sim.hitPoints));
    // for(uint32 hitPointIndex = 0;
    //     hitPointIndex < low->sim.hitPointMax;
    //     ++hitPointIndex)
    // {
    //     hit_point *hitPoint = low->sim.hitPoints + hitPointIndex;
    //     hitPoint->flags = 0;
    //     hitPoint->filledAmount = HIT_POINT_SUB_COUNT;
    // }
}

internal add_low_entity_result
AddBullet(game_state *gameState)
{
    add_low_entity_result entity = AddLowEntity(gameState, EntityType_Bullet, NullPosition());
    AddFlags(&entity.low->sim, EntityFlag_Movable | EntityFlag_YSupported | EntityFlag_CanCollide|EntityFlag_Nonspatial); 

    entity.low->sim.collision = gameState->swordCollision;

    return entity;
}

internal add_low_entity_result
AddSword(game_state *gameState)
{
    add_low_entity_result entity = AddLowEntity(gameState, EntityType_Bullet, NullPosition());
	AddFlags(&entity.low->sim, EntityFlag_Movable | EntityFlag_YSupported | EntityFlag_CanCollide);	
    entity.low->sim.collision = gameState->swordCollision;

    return entity;
}

internal add_low_entity_result
AddCloud(game_state *gameState, uint32 absTileX, uint32 absTileY, uint32 absTileZ, r32 alpha = 1.0f)
{
    world_position pos = TilePositionToChunkPosition(gameState->world, absTileX, absTileY, absTileZ);
    add_low_entity_result entity = AddGroundedLowEntity(gameState, EntityType_Cloud, pos, gameState->playerCollision);

    AddFlags(&entity.low->sim, EntityFlag_YSupported); 
    entity.low->sim.alpha = alpha;

    entity.low->sim.size = RandomBetween(&DEBUGSeries, 15.0f, 40.0f);

    return entity;
}

internal add_low_entity_result
AddMountain(game_state *gameState, uint32 absTileX, uint32 absTileY, uint32 absTileZ, r32 alpha = 1.0f)
{
    world_position pos = TilePositionToChunkPosition(gameState->world, absTileX, absTileY, absTileZ);
    add_low_entity_result entity = AddGroundedLowEntity(gameState, EntityType_Mountain, pos, gameState->playerCollision);

    AddFlags(&entity.low->sim, EntityFlag_YSupported); 
    entity.low->sim.alpha = alpha;

    entity.low->sim.size = RandomBetween(&DEBUGSeries, 70.0f, 100.0f);

    return entity;
}

internal add_low_entity_result
AddSun(game_state *gameState, uint32 absTileX, uint32 absTileY, uint32 absTileZ, r32 alpha = 1.0f)
{
    world_position pos = TilePositionToChunkPosition(gameState->world, absTileX, absTileY, absTileZ);
    add_low_entity_result entity = AddGroundedLowEntity(gameState, EntityType_Sun, pos, gameState->playerCollision);

    AddFlags(&entity.low->sim, EntityFlag_YSupported); 
    entity.low->sim.alpha = alpha;

    return entity;
}

internal add_low_entity_result
AddTutorial(game_state *gameState, uint32 absTileX, uint32 absTileY, uint32 absTileZ, r32 alpha = 1.0f)
{
    world_position pos = TilePositionToChunkPosition(gameState->world, absTileX, absTileY, absTileZ);
    add_low_entity_result entity = AddGroundedLowEntity(gameState, EntityType_Tutorial, pos, gameState->playerCollision);

    AddFlags(&entity.low->sim, EntityFlag_YSupported); 
    entity.low->sim.alpha = alpha;

    return entity;
}

inline void
SetAnimation(sim_entity_animation *animation, u32 totalFrame, r32 timePerFrame, asset_type_id ID, v2 offset = V2(0, 0))
{
    animation->offset = offset;
    animation->totalFrame = totalFrame;
    animation->timePerFrame = timePerFrame;
    animation->currentFrameTime = 0.0f;
    animation->currentFrame = 0;
    animation->ID = ID;
}

internal add_low_entity_result
AddPlayer(game_state *gameState, uint32 absTileX, uint32 absTileY, uint32 absTileZ)
{
    world_position pos = TilePositionToChunkPosition(gameState->world, absTileX, absTileY, absTileZ);
    add_low_entity_result entity = AddGroundedLowEntity(gameState, EntityType_Player, pos, gameState->playerCollision);
    
    AddFlags(&entity.low->sim, EntityFlag_Movable|EntityFlag_CanCollide);

    InitHitPoints(entity.low, 1);

    entity.low->sim.maxInvincibleTime = 0.5f;

    // If there is no entity that the camera is following,
    // Make this new entity followed by the camera
     gameState->cameraFollowingEntityIndex = entity.storageIndex;

    return entity;
}

internal add_low_entity_result
AddMonster(game_state *gameState, uint32 absTileX, uint32 absTileY, uint32 absTileZ)
{
    world_position worldPositionOfTile = 
    TilePositionToChunkPosition(gameState->world, absTileX, absTileY, absTileZ);
    
    add_low_entity_result entity = 
    AddGroundedLowEntity(gameState, EntityType_Monster, worldPositionOfTile, gameState->monsterCollision);

    AddFlags(&entity.low->sim, EntityFlag_Movable|EntityFlag_CanCollide);

    entity.low->sim.moveSpec = MoveSpec(true, RandomBetween(&DEBUGSeries, 20.0f, 35.0f), 
                                            V3(RandomBetween(&DEBUGSeries, 0.3f, 0.4f), 
                                                RandomBetween(&DEBUGSeries, 0.2f, 0.3f), 
                                                0.0f), 0.3f);
    entity.low->sim.maxInvincibleTime = 1.0f;

    InitHitPoints(entity.low, 1);
    
    return entity;
}

internal add_low_entity_result
AddBigFollowingEnemy(game_state *gameState, uint32 absTileX, uint32 absTileY, uint32 absTileZ)
{
    world_position worldPositionOfTile = 
    TilePositionToChunkPosition(gameState->world, absTileX, absTileY, absTileZ);
    
    add_low_entity_result entity = 
    AddGroundedLowEntity(gameState, EntityType_BigFollowingEnemy, worldPositionOfTile, gameState->bigEnemyCollision);

    AddFlags(&entity.low->sim, EntityFlag_Movable|EntityFlag_CanCollide);

    entity.low->sim.moveSpec = MoveSpec(true, RandomBetween(&DEBUGSeries, 20.0f, 25.0f), 
                                            V3(RandomBetween(&DEBUGSeries, 0.3f, 0.4f), 
                                                RandomBetween(&DEBUGSeries, 0.2f, 0.3f), 
                                                0.0f), 0.3f);
    entity.low->sim.maxInvincibleTime = 1.0f;

    InitHitPoints(entity.low, 2);
    
    return entity;
}


internal add_low_entity_result
AddPatrolEnemy(game_state *gameState, uint32 absTileX, uint32 absTileY, uint32 absTileZ)
{
    world_position worldPositionOfTile = 
    TilePositionToChunkPosition(gameState->world, absTileX, absTileY, absTileZ);
    
    add_low_entity_result entity = 
    AddGroundedLowEntity(gameState, EntityType_PatrolEnemy, worldPositionOfTile, gameState->monsterCollision);

    AddFlags(&entity.low->sim, EntityFlag_Movable|EntityFlag_CanCollide);

    entity.low->sim.moveSpec = MoveSpec(true, RandomBetween(&DEBUGSeries, 40.0f, 65.0f), 
                                        V3(RandomBetween(&DEBUGSeries, 0.3f, 0.4f), 
                                            RandomBetween(&DEBUGSeries, 0.2f, 0.3f), 
                                            0.0f), 0.3f);
    entity.low->sim.maxInvincibleTime = -1.0f;

    InitHitPoints(entity.low, 1);
    
    return entity;

}

internal add_low_entity_result
AddLaserShootingEnemy(game_state *gameState, uint32 absTileX, uint32 absTileY, uint32 absTileZ)
{
    world_position worldPositionOfTile = 
    TilePositionToChunkPosition(gameState->world, absTileX, absTileY, absTileZ);
    
    add_low_entity_result entity = 
    AddGroundedLowEntity(gameState, EntityType_LaserShootingEnemy, worldPositionOfTile, gameState->bigEnemyCollision);

    AddFlags(&entity.low->sim, EntityFlag_Movable|EntityFlag_CanCollide|EntityFlag_YSupported);
    InitHitPoints(entity.low, 2);

    entity.low->sim.maxInvincibleTime = 1.0f;
    entity.low->sim.moveSpec = MoveSpec(true, RandomBetween(&DEBUGSeries, 5.0f, 15.0f), 
                                        V3(RandomBetween(&DEBUGSeries, 0.3f, 0.5f), 
                                            RandomBetween(&DEBUGSeries, 0.3f, 0.4f), 
                                            0.0f), 0.3f);
    return entity;
}

internal add_low_entity_result
AddSponge(game_state *gameState, uint32 absTileX, uint32 absTileY, uint32 absTileZ)
{
    world_position worldPositionOfTile = 
    TilePositionToChunkPosition(gameState->world, absTileX, absTileY, absTileZ);
    
    add_low_entity_result entity = 
    AddGroundedLowEntity(gameState, EntityType_Sponge, worldPositionOfTile, gameState->spongeCollision);

    AddFlags(&entity.low->sim, EntityFlag_CanCollide | EntityFlag_YSupported);

    InitHitPoints(entity.low, 2);
    
    return entity;
}

internal add_low_entity_result
AddLaserTrap(game_state *gameState, uint32 absTileX, uint32 absTileY, uint32 absTileZ)
{
    world_position worldPositionOfTile = 
    TilePositionToChunkPosition(gameState->world, absTileX, absTileY, absTileZ);
    
    add_low_entity_result entity = 
    AddGroundedLowEntity(gameState, EntityType_LaserTrap, worldPositionOfTile, gameState->monsterCollision);

    AddFlags(&entity.low->sim, EntityFlag_YSupported);
    
    return entity;
}

internal add_low_entity_result
AddFamiliar(game_state *gameState, uint32 absTileX, uint32 absTileY, uint32 absTileZ)
{
    world_position worldPositionOfTile = 
    TilePositionToChunkPosition(gameState->world, absTileX, absTileY, absTileZ);
    
    add_low_entity_result entity = AddLowEntity(gameState, EntityType_Familiar, worldPositionOfTile);

    entity.low->sim.collision = gameState->familiarCollision;

    AddFlags(&entity.low->sim, EntityFlag_Movable);

    return entity;
}

#if 0
internal add_low_entity_result
AddStair(game_state *gameState, uint32 absTileX, uint32 absTileY, uint32 absTileZ)
{
    // For the stairs, we want to offset a little be higher
    // so that the maximum Z value is the ground of next floor
    world_position worldPositionOfTile = 
    TilePositionToChunkPosition(gameState->world, absTileX, absTileY, absTileZ);

    add_low_entity_result entity = 
    AddGroundedLowEntity(gameState, EntityType_Stairwell, worldPositionOfTile, gameState->stairCollision);

    // AddFlags(&entity.low->sim, EntityFlag_ZSupported);    

    entity.low->sim.walkableDim = entity.low->sim.collision->totalVolume.dim.xy;    
    entity.low->sim.walkableHeight = gameState->typicalFloorHeight;

    return entity;
}
#endif

// Unlike other addentity calls, this is from the center!
// Which means, absTileX,Y,Z are or centertiles
internal add_low_entity_result
AddStandardSpace(game_state *gameState, uint32 absTileX, uint32 absTileY, uint32 absTileZ)
{
    world_position worldPositionOfTile = 
    TilePositionToChunkPosition(gameState->world, absTileX, absTileY, absTileZ);

    add_low_entity_result entity = 
    AddGroundedLowEntity(gameState, EntityType_Space, worldPositionOfTile, gameState->standardRoomCollision);

    AddFlags(&entity.low->sim, EntityFlag_Traversable);

    return entity;
}

// Add wall based on the tilemap
// Of course because we don't have tilemap anymore, 
// we have to map this tilemap to the chunkXY and offset
internal add_low_entity_result
AddWall(game_state *gameState, uint32 absTileX, uint32 absTileY, uint32 absTileZ)
{
    world_position worldPositionOfTile = 
    TilePositionToChunkPosition(gameState->world, absTileX, absTileY, absTileZ);

    add_low_entity_result entity = 
    AddGroundedLowEntity(gameState, EntityType_Wall, worldPositionOfTile, gameState->wallCollision);

    AddFlags(&entity.low->sim, EntityFlag_CanCollide);
    
    return entity;
}

internal void
AddEntityToTileY(game_state *gameState, entity_type type)
{
    u32 tileX = (u32)RandomBetween(&DEBUGSeries, (float)gameState->roomMinTileX - 10, (float)gameState->roomMaxTileX + 10);
    u32 tileY = gameState->roomMinTileY - 24;

    r32 absMaxCloudZ = 2.0f;
    i32 tileZ = (i32)RandomBetween(&DEBUGSeries, absMaxCloudZ-1.0f, absMaxCloudZ + 2.0f);

    AddMountain(gameState, tileX, tileY, tileZ, 0.5f + (r32)tileZ/(absMaxCloudZ+1.0f));
}

internal void
AddEntityToRandomPosition(game_state *gameState, entity_type type)
{
    u32 tileX = (u32)RandomBetween(&DEBUGSeries, (float)gameState->roomMinTileX, (float)gameState->roomMaxTileX);
    u32 tileY = (u32)RandomBetween(&DEBUGSeries, (float)gameState->roomMinTileY, (float)gameState->roomMaxTileY);
    switch(type)
    {
        case EntityType_Monster:
        {
            AddMonster(gameState, tileX, tileY, gameState->roomMinTileZ);
        }break;

        case EntityType_PatrolEnemy:
        {
            AddPatrolEnemy(gameState, tileX, tileY, gameState->roomMinTileZ);
        }break;
        
        case EntityType_Cloud:
        {
            r32 absMaxCloudZ = 8.0f;
            i32 tileZ = (i32)RandomBetween(&DEBUGSeries, -absMaxCloudZ+4.0f, absMaxCloudZ + 2.0f);
            AddCloud(gameState, tileX, tileY, gameState->roomMinTileZ+tileZ, 0.2f + (r32)tileZ/(absMaxCloudZ+1.0f));
        }break;

        case EntityType_LaserShootingEnemy:
        {
            AddLaserShootingEnemy(gameState, tileX, tileY, gameState->roomMinTileZ);
        }break;

        case EntityType_BigFollowingEnemy:
        {
            AddBigFollowingEnemy(gameState, tileX, tileY, gameState->roomMinTileZ);
        }break;
    }
}

internal void
DrawHitpoints(sim_entity *entity, render_group *pieceGroup)
{
    if(entity->hitPointMax >= 1)
    {
        // size of each health square
        v2 healthDim = {0.2f, 0.2f};
        // space between each heatlh square
        real32 spacingX = 1.5f * healthDim.x;
        v2 hitP = {-0.5f * (entity->hitPointMax - 1) * spacingX, -1.0f};
        v2 dHitP = {spacingX, 0.0f};

        for(i32 healthIndex = 0;
            healthIndex < entity->hitPointMax;
            ++healthIndex)
        {
            v4 color = {1.0f, 0.0f, 0.0f, 1.0f};
            PushRect(pieceGroup, V3(hitP, 0), healthDim, color);
            hitP += dHitP;
        }
    }
}

internal void
DrawShield(sim_entity *entity, render_group *renderGroup, r32 size, v3 color)
{
    if(entity->invincibleTime >= 0.0f)
    {
        r32 invincibleRatio = entity->invincibleTime/entity->maxInvincibleTime;
        PushBitmap(renderGroup, GetFirstBitmapFrom(renderGroup->assets, Asset_Type_Shield), size/entity->invincibleTime, V3(0, 0, 0), Cos(entity->invincibleTime*Pi32), Sin(entity->invincibleTime*Pi32), V4(color, invincibleRatio));
        PushBitmap(renderGroup, GetFirstBitmapFrom(renderGroup->assets, Asset_Type_Shield), (size/5.0f)/entity->invincibleTime, V3(0, 0, 0), Cos(-entity->invincibleTime*Pi32), Sin(-entity->invincibleTime*Pi32), V4(color, invincibleRatio));
    }   
}

internal void
DrawNumbers(i32 number, r32 size, v2 pos, v2 dPos, render_group *renderGroup, v4 color = V4(1, 1, 1, 1))
{
    Assert(number >= 0);
    i32 digit = (number < 10 ? 1 :   
                (number < 100 ? 2 :   
                (number < 1000 ? 3 :   
                (number < 10000 ? 4 :    
                (number < 100000 ? 5 :   
                (number < 1000000 ? 6 :   
                (number < 10000000 ? 7 :  
                (number < 100000000 ? 8 :  
                (number < 1000000000 ? 9 : 10)))))))));  

    // NOTE : Because we are printing from the end.
    dPos *= -1.0f;

    for(i32 digitIndex = 0;
        digitIndex < digit;
        ++digitIndex)
    {
        asset_match_vector matchVector = {};
        matchVector.e[Asset_Tag_Numbering] = (r32)(number%10);
        asset_weight_vector weightVector = {};
        weightVector.e[Asset_Tag_Numbering] = 1.0f;
        PushBitmap(renderGroup, GetBestMatchBitmap(renderGroup->assets, Asset_Type_Number, &matchVector, &weightVector), size, 
                    V3(pos, 0), 1.0f, 0.0f, color, false);

        pos += dPos;
        number /= 10;
    }
}

internal sim_entity_collision_volume_group *
MakeSimpleGroundedCollision(game_state *gameState, real32 dimX, real32 dimY, real32 dimZ)
{
    // TODO : Not world arena! Change to using the fundamental entity arena
    sim_entity_collision_volume_group *result = 
    PushStruct(&gameState->worldArena, sim_entity_collision_volume_group);
    result->volumeCount = 1;
    result->volumes = PushArray(&gameState->worldArena, result->volumeCount, sim_entity_collision_volume);
    result->totalVolume.offset = V3(0, 0, 0.5f*dimZ);
    result->totalVolume.dim = V3(dimX, dimY, dimZ);
    result->volumes[0] = result->totalVolume;

    return result;
}

// TODO : MultiThreaded?
internal void
FillGroundChunks(transient_state *tranState, game_state *gameState, 
    ground_buffer *groundBuffer, world_position *chunkPos)
{
    temporary_memory groundRenderMemory = BeginTemporaryMemory(&tranState->tranArena);
    
    // TODO : How do we want to control our ground chunk resolution?
    // TODO : Find out what is the precies maxPushBufferSize is!
    render_group *groundRenderGroup = 
    AllocateRenderGroup(tranState->assets, &tranState->tranArena, Megabytes(4), groundBuffer->bitmap.width, groundBuffer->bitmap.height);

    Clear(groundRenderGroup, V4(0.2f, 0.2f, 0.2f, 1.0f));

    loaded_bitmap *buffer = &groundBuffer->bitmap;
    buffer->alignPercentage = V2(0.5f, 0.5f);
    buffer->widthOverHeight = 1.0f;

    groundBuffer->pos = *chunkPos; 

#if 0
    real32 width = gameState->world->chunkDimInMeters.x;
    real32 height = gameState->world->chunkDimInMeters.y;

    for(int32 chunkOffsetY = -1;
        chunkOffsetY <= 1;
        ++chunkOffsetY)
    {
        for(int32 chunkOffsetX = -1;
            chunkOffsetX <= 1;
            ++chunkOffsetX)
        {
            int32 chunkX = chunkPos->chunkX + chunkOffsetX;
            int32 chunkY = chunkPos->chunkY + chunkOffsetY;
            int32 chunkZ = chunkPos->chunkZ;

            // TODO : Make random number generation more systemic
            // TODO : Look into wang hashing or some other spatial seed generation
            random_series series = Seed(132*chunkX + 217*chunkY + 532*chunkZ);

            v2 center = V2(chunkOffsetX*width, chunkOffsetY*height);
    
            for(uint32 grassIndex = 0;
                grassIndex < 100;
                ++grassIndex)
            {
                loaded_bitmap *stamp;
                if(RandomChoice(&series, 2))
                {
                    stamp = gameState->grass + RandomChoice(&series, ArrayCount(gameState->grass));
                }
                else
                {
                    stamp = gameState->stone + RandomChoice(&series, ArrayCount(gameState->stone));            
                }

                v2 pos = center + Hadamard(V2(width, height), V2(RandomUnilateral(&series), RandomUnilateral(&series)));

                PushBitmap(groundRenderGroup, stamp, 1.0f, V3(pos, 0.0f));
            }
        }
    }    
#endif

    TiledRenderGroupToOutput(groundRenderGroup, buffer, tranState->renderQueue);
    EndTemporaryMemory(groundRenderMemory);
}

internal void
ClearBitmap(loaded_bitmap *bitmap)
{
    if(bitmap)
    {
        int32 totalBitmapSize = bitmap->width * bitmap->height * BITMAP_BYTES_PER_PIXEL;
        ZeroSize(totalBitmapSize, bitmap->memory);
    }
}

internal loaded_bitmap
MakeEmptyBitmap(memory_arena *arena, int32 width, int32 height, bool32 shouldBeCleared = true)
{
    loaded_bitmap result = {};

    result.width = width;
    result.height = height;
    result.pitch = result.width * BITMAP_BYTES_PER_PIXEL;
    int32 totalBitmapSize = result.width * result.height * BITMAP_BYTES_PER_PIXEL;
    result.memory = PushSize_(arena, totalBitmapSize);

    if(shouldBeCleared)
    {
        ClearBitmap(&result);
    }

    return result;
}

internal void
MakeSphereDiffuseMap(loaded_bitmap *bitmap, real32 Cx = 1.0f, real32 Cy = 1.0f)
{
    real32 InvWidth = 1.0f / (real32)(bitmap->width - 1);
    real32 InvHeight = 1.0f / (real32)(bitmap->height - 1);
    
    uint8 *row = (uint8 *)bitmap->memory;
    for(int32 y = 0;
        y < bitmap->height;
        ++y)
    {
        uint32 *pixel = (uint32 *)row;
        for(int32 x = 0;
            x < bitmap->width;
            ++x)
        {
            v2 bitmapUV = {InvWidth*(real32)x, InvHeight*(real32)y};

            real32 Nx = Cx*(2.0f*bitmapUV.x - 1.0f);
            real32 Ny = Cy*(2.0f*bitmapUV.y - 1.0f);

            real32 rootTerm = 1.0f - Nx*Nx - Ny*Ny;
            real32 alpha = 0.0f;            
            if(rootTerm >= 0.0f)
            {
                alpha = 1.0f;
            }

            v3 baseColor = {0.0f, 0.0f, 0.0f};
            alpha *= 255.0f;
            v4 color = {alpha*baseColor.x,
                alpha*baseColor.y,
                alpha*baseColor.z,
                alpha};

                *pixel++ = (((uint32)(color.a + 0.5f) << 24) |
                    ((uint32)(color.r + 0.5f) << 16) |
                    ((uint32)(color.g + 0.5f) << 8) |
                    ((uint32)(color.b + 0.5f) << 0));
            }

            row += bitmap->pitch;
        }
    }

    internal void
    MakeSphereNormalMap(loaded_bitmap *bitmap, real32 roughness)
    {
        real32 invWidth = 1.0f/(real32)(bitmap->width - 1);
        real32 invHeight = 1.0f/(real32)(bitmap->height - 1);

        uint8 *row = (uint8 *)bitmap->memory;
        for(int32 y = 0;
            y < bitmap->height;
            ++y)
        {
            uint32 *pixel = (uint32 *)row;
            for(int32 x = 0;
                x < bitmap->width;
                ++x)
            {
                v2 bitmapUV = {invWidth*(real32)x, invHeight*(real32)y};

                real32 nX = 2.0f*bitmapUV.x - 1.0f;
                real32 nY = 2.0f*bitmapUV.y - 1.0f;

                real32 rootTerm = 1.0f - nX*nX - nY*nY;

            // Normal is in -101 space!
                v3 normal = {0, 0.707106781188f, 0.707106781188f};
                real32 nZ = 0.0f;
                if(rootTerm >= 0.0f)
                {
                    nZ = Root2(rootTerm);
                    normal = {nX, nY, nZ};
                }

                v4 color = {255.0f*(0.5f*(normal.x + 1.0f)),
                    255.0f*(0.5f*(normal.y +11.0f)),
                    255.0f*(0.5f*(normal.z + 1.0f)),
                    255.0f*roughness};

                    *pixel++ = (((uint32)(color.a + 0.5f) << 24) |
                        ((uint32)(color.r + 0.5f) << 16) |
                        ((uint32)(color.g + 0.5f) << 8) |
                        ((uint32)(color.b + 0.5f) << 0));
                }

                row += bitmap->pitch;
            }
        }
        
internal void
PlaySound(game_state *gameState, sound_slot_id ID, r32 volume = 1.0f)
{
    if(!gameState->firstFreePlayingSound)
    {
        gameState->firstFreePlayingSound = PushStruct(&gameState->worldArena, playing_sound);
        gameState->firstFreePlayingSound->next = 0;
    }

    playing_sound *playingSound = gameState->firstFreePlayingSound;
    gameState->firstFreePlayingSound = playingSound->next;

    playingSound->samplesPlayed = 0;
    playingSound->volume[0] = volume;
    playingSound->volume[1] = volume;
    playingSound->ID = ID;

    playingSound->next = gameState->firstPlayingSound;
    gameState->firstPlayingSound = playingSound;
}

internal raycast2d_result
SetLaser(sim_entity *shooter, laser_spec_group *laserGroup, sim_region *simRegion,
            render_group *renderGroup = 0)
{
    raycast2d_result raycastResult = {};

    r32 length = 300.0f;
    v3 initialBulletPosOffset = Rotate(V3(0.5f*length, 0, 0), shooter->facingRad);

    r32 angle = 0.0f;
    v4 color = V4(RandomBetween(&DEBUGSeries, 0.0f, 1.0f), RandomBetween(&DEBUGSeries, 0.0f, 1.0f), RandomBetween(&DEBUGSeries, 0.0f, 1.0f), 1.0f);
    for(u32 specIndex = 0;
        specIndex < laserGroup->specCount;
        ++specIndex)
    {
        laser_spec *spec = laserGroup->specs + specIndex;
        angle = shooter->facingRad + spec->directionRad;
        r32 cos = Cos(angle);
        r32 sin = Sin(angle);

        raycastResult = RaycastToObjects2D(shooter->pos.xy, angle, shooter, laserGroup->dim, simRegion);

        if(raycastResult.hitEntity)
        {
            --raycastResult.hitEntity->hitPointMax;
            raycastResult.hitEntity->invincibleTime = raycastResult.hitEntity->maxInvincibleTime;

            v3 hitEntityRelPos = V3(raycastResult.hitPos - shooter->pos.xy, 0.0f);
            initialBulletPosOffset = 0.5f*(hitEntityRelPos);
            v3 shooterFacingRelVector = V3(cos, sin, 0.0f);
            // NOTE : Project to shooting line so that we offset the rectangle correctly.
            initialBulletPosOffset = Inner(initialBulletPosOffset, shooterFacingRelVector)*shooterFacingRelVector;
            length = Length(hitEntityRelPos);
        }

        renderGroup->transform.entitySimPos = shooter->pos;
        PushBitmapWithDesiredHeight(renderGroup, GetFirstBitmapFrom(renderGroup->assets, Asset_Type_RaycastAttack), 
                                length, initialBulletPosOffset, 
                                laserGroup->dim, cos, sin, color, true);
    }

    shooter->shouldThereBeRecoil = true;
    shooter->recoil = laserGroup->recoilAmount*V3(Cos(shooter->facingRad + laserGroup->recoilRad), 
                                                    Sin(shooter->facingRad + laserGroup->recoilRad), 0);

    return raycastResult;
}

internal v2
GetEntityFacingDirectionWithMouse(sim_entity *entity, v2 mousePos)
{
    v2 result = {};

    v2 dir = mousePos - entity->pos.xy;
    r32 lengthOfDir = Length(dir);

    result.x = dir.y / lengthOfDir;
    result.y = -dir.x / lengthOfDir;

    return result;
}

internal r32
GetEntityFacingDirection(r32 cos)
{
    r32 result = 0.0f;
    r32 absCos = AbsoluteValue(cos);
    r32 rad15 = 0.084f*Pi32;

    if(absCos >= Cos(rad15) &&
        absCos <= 1.0f)
    {
        // side
        result = 1.0f;
    }
    else if(cos >= Cos(5.0f*rad15) &&
            cos <= Cos(rad15))
    {
        // diagonal
        result = 1.0f;
    }
    else if(cos >= Cos(7.0f*rad15) &&
            cos <= Cos(5.0f*rad15))
    {
        // straight
        result = 2.0f;
    }
    else 
    {
        // diagonal
        result = 1.0f;
    }

    return result;
}

internal v3
ShakeCamera(r32 baseShakeAmount, r32 shakeAmount)
{
    v3 result = {};
    result.x = baseShakeAmount + RandomBetween(&DEBUGSeries, -shakeAmount, shakeAmount);
    result.y = baseShakeAmount + RandomBetween(&DEBUGSeries, -shakeAmount, shakeAmount);
    result.z = baseShakeAmount + RandomBetween(&DEBUGSeries, -5.0f*shakeAmount, 5.0f*shakeAmount);

    return result;
}

internal r32
ProcessAnimation(sim_entity_animation *animation, r32 dt)
{
    // NOTE : Process Animation
    animation->currentFrameTime += dt;
    if(animation->currentFrameTime >= animation->timePerFrame)
    {
        animation->currentFrameTime = 0.0f;
        animation->currentFrame = (animation->currentFrame+1)%animation->totalFrame;
    }

    return (r32)animation->currentFrame;
}

#if 0
internal v3
ShootBullet(sim_entity *shooter, game_state *gameState, sim_region *simRegion, bullet_type type,    
            v3 initialBulletPosOffset, entity_type whoseBullet = EntityType_Bullet, r32 attackCooldown = -1.0f, render_group *renderGroup = 0)
{
    bullet_spec_group *bulletGroup = 0;
    v3 recoil = {};

    switch(type)
    {
        case BulletType_Single:
        {
            bulletGroup = gameState->single;
        }break;

        case BulletType_Twin:
        {
            bulletGroup = gameState->single;
        }break;

        case BulletType_Quad:
        {
            bulletGroup = gameState->single;
        }break;

        case BulletType_Laser:
        {
            bulletGroup = gameState->raycast;
        }break;
    }

    if(type != BulletType_Laser)
    {
        for(u32 bulletSpecIndex = 0;
            bulletSpecIndex < bulletGroup->specCount;
            ++bulletSpecIndex)
        {
            bullet_spec *bulletSpec = bulletGroup->specs + bulletSpecIndex;
            sim_entity *bullet = 0;

            // NOTE : Find Bullet
            for(u32 bulletIndex = 0;
                bulletIndex < ArrayCount(simRegion->bullets);
                ++bulletIndex)
            {
                sim_entity *targetBullet = simRegion->bullets[bulletIndex];

                if(IsSet(targetBullet, EntityFlag_Nonspatial))
                {
                    bullet = targetBullet;
                    break;
                }
            }

            if(bullet)
            {
                // NOTE : Whose bullet is it? Player, or monster?
                bullet->type = whoseBullet;
                bullet->distanceLimit = bulletGroup->distanceLimit;

                v3 swordPos = shooter->pos + V3(0, 1.4f, 0);
                swordPos.xy = Rotate(swordPos.xy, shooter->facingRad);

                MakeEntitySpatial(bullet, shooter->pos + initialBulletPosOffset, 50.0f*V3(Cos(shooter->facingRad + bulletSpec->directionRad), Sin(shooter->facingRad + bulletSpec->directionRad), 0));

                // Sword itself should not collide with the player!
                AddCollisionRule(gameState, shooter->storageIndex, bullet->storageIndex, false);

            }
        }
    }
    else
    {
        sim_entity *bullet = 0;

        // NOTE : Find Bullet
        for(u32 raycastBulletIndex = 0;
            raycastBulletIndex < ArrayCount(simRegion->raycastBullets);
            ++raycastBulletIndex)
        {
            sim_entity *targetBullet = simRegion->raycastBullets[raycastBulletIndex];
            Assert(targetBullet);
            if(IsSet(targetBullet, EntityFlag_Nonspatial))
            {
                bullet = targetBullet;
                break;
            }
        }

        if(bullet)
        {
            bullet->facingRad = shooter->facingRad;
            bullet->alpha = 1.0f;
            bullet->rayStartPos = shooter->pos;
            bullet->raycastRemainingTime = 2;
            MakeEntitySpatial(bullet, shooter->pos + initialBulletPosOffset, V3(0, 0, 0));
        }
    }

    r32 recoilAmount = bulletGroup->recoilAmount;
    recoil = recoilAmount*V3(Cos(shooter->facingRad + Pi32), Sin(shooter->facingRad + Pi32), 0);

    if(attackCooldown <= 0.0f)
    {
        shooter->attackCooldown = bulletGroup->attackCooldown;
    }
    else
    {
        shooter->attackCooldown = attackCooldown;
    }

    return recoil;
}
#endif

#if FOX_DEBUG
        game_memory *debugGlobalMemory;
#endif

    extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
    {

        PlatformAddEntry = memory->PlatformAddEntry;
        PlatformCompleteAllWork = memory->PlatformCompleteAllWork; 
        DEBUGPlatformReadEntireFile = memory->debugPlatformReadEntireFile;

#if FOX_DEBUG
        debugGlobalMemory = memory;
#endif

        BEGIN_TIMED_BLOCK(GameUpdateAndRender);

        Assert(sizeof(game_state) <= memory->permanentStorageSize);

        uint32 groundBufferWidth = 256;
        uint32 groundBufferHeight = 256;

        real32 pixelsToMeters = 1.0f / 42.0f;
        v3 playerColor = V3(0.47058f, 0.97254f, 0.8f);
        v3 enemyColor = V3(0.8f, 0.2f, 0.2f);

        game_state *gameState = (game_state *)memory->permanentStorage;
        if(!gameState->isInitialized)
        {
    // TODO : Talk about this soon!  Let's start partitioning our memory space!
            InitializeArena(&gameState->worldArena, 
                            (memory_index)(memory->permanentStorageSize - sizeof(game_state)),
                            (uint8 *)memory->permanentStorage + sizeof(game_state));

            gameState->world = PushStruct(&gameState->worldArena, world);

            gameState->typicalFloorHeight = 3.0f;
            InitializeWorld(gameState->world, 
                V3(pixelsToMeters*groundBufferWidth,
                    pixelsToMeters*groundBufferHeight,
                    gameState->typicalFloorHeight));

            // TODO : Find out the safe bound for this!
            uint32 tilesPerWidth = 500;
            uint32 tilesPerHeight = 200;

            real32 tileSideInMeters = 1.4f;
            real32 tileDeptInMeters = 3.0f;

            // NOTE : Reserve entity slot 0 for the null entity
            AddLowEntity(gameState, EntityType_Null, NullPosition());
            gameState->lowEntityCount = 1;

            gameState->swordCollision = MakeSimpleGroundedCollision(gameState, 0.2f, 0.2f, 0.1f);
            gameState->stairCollision = MakeSimpleGroundedCollision(gameState, tileSideInMeters, 2.0f*tileSideInMeters, 1.1f*tileDeptInMeters);
            gameState->playerCollision = MakeSimpleGroundedCollision(gameState, 1.3f, 1.2f, 1.2f);
            gameState->monsterCollision = MakeSimpleGroundedCollision(gameState, 2.5f, 2.5f, 0.5f);
            gameState->bigEnemyCollision = MakeSimpleGroundedCollision(gameState, 5.0f, 5.0f, 0.5f);

            // gameState->monsterCollision = MakeSimpleGroundedCollision(gameState, 10.5f, 10.5f, 0.5f);
            gameState->wallCollision = MakeSimpleGroundedCollision(gameState, tileSideInMeters, tileSideInMeters, 0.5f*tileDeptInMeters); 
            gameState->standardRoomCollision = MakeSimpleGroundedCollision(gameState, tilesPerWidth * tileSideInMeters, tilesPerHeight * tileSideInMeters, 0.9f*tileDeptInMeters);
            gameState->spongeCollision = MakeSimpleGroundedCollision(gameState, 15.0f, 15.0f, 0.5f);

            // (0, 0, 0) is the center of the world!!
            uint32 screenBaseX = 0;
            uint32 screenBaseY = 0;
            uint32 screenBaseZ = 0;
            uint32 screenX = screenBaseX;
            uint32 screenY = screenBaseY;

            uint32 absTileZ = screenBaseZ;

            bool32 doorLeft = false;
            bool32 doorRight = false;
            bool32 doorTop = false;
            bool32 doorBottom = false;
            bool32 doorUp = false;
            bool32 doorDown = false;

            bool32 zDoorCreated = false;

            random_series series = Seed(321);

        // One screen is worth one room

        //
        // NOTE : Map Generation
        // 
           // NOTE : Adjust this to make more rooms
            for(uint32 screenIndex = 0;
                screenIndex < 1;
                ++screenIndex)
            {
                uint32 doorDirection = RandomChoice(&series, (doorUp || doorDown) ? 2 : 3);
                // uint32 doorDirection = RandomChoice(&series, 2);

                bool32 createdZDoor = false;
                if(doorDirection == 2)
                {
                    createdZDoor = true;
                    if(absTileZ == 0)
                    {
                        doorUp = false;
                    }
                    else
                    {
                        doorDown = false;
                    }
                }
                else if(doorDirection == 1)
                {
                    doorRight = false;
                }
                else
                {
                    doorTop = false;
                }

                // AddStandardSpace(gameState, 
                //     screenBaseX * tilesPerWidth + tilesPerWidth/2,
                //     screenBaseY * tilesPerHeight + tilesPerHeight/2,
                //     screenBaseZ);

                // Loop through tiles
                for(uint32 tileY = 0;
                    tileY < tilesPerHeight;
                    ++tileY)
                {
                    for(uint32 tileX = 0;
                        tileX < tilesPerWidth;
                        ++tileX)
                    {
                // These values will always increase
                        uint32 absTileX = screenX * tilesPerWidth + tileX;
                        uint32 absTileY = screenY * tilesPerHeight + tileY;


                        bool32 shouldBeWall = false;
#if 1
                        // if(tileX == 0 && ((tileY != (tilesPerHeight / 2))))
                        if(tileX == 0)
                        {
                            shouldBeWall = true;
                        }

                        // if(tileX == tilesPerWidth - 1 && (tileY != tilesPerHeight / 2))
                        if(tileX == tilesPerWidth - 1)
                        {
                            shouldBeWall = true;
                        }

                        // if(tileY == 0 && (tileX != tilesPerWidth / 2))
                        if(tileY == 0)
                        {
                            shouldBeWall = true;
                        } 

                        // if(tileY == tilesPerHeight - 1 && (tileX != tilesPerWidth / 2))
                        if(tileY == tilesPerHeight - 1)
                        {
                            shouldBeWall = true;
                        }
#endif

                        if(shouldBeWall)
                        {
                            // AddWall(gameState, absTileX, absTileY, absTileZ);
                        }

                        #if 0
                        else if(createdZDoor)
                        {
                            if((tileX == 10) && (tileY == 6))
                            {
                                if(!zDoorCreated)
                                {
                                    AddStair(gameState, absTileX, absTileY - 1, absTileZ);
                                    zDoorCreated = true;
                                }
                        // if(doorUp)
                        // {
                        //     tileValue = 3;
                        // }

                        // if(doorDown)
                        // {
                        //     tileValue = 4;
                        // }   
                            }
                        }
                        #endif
                    }
                }

        // If the door was OPEN to the left in this tilemap,
        // The door in the next tilemap should be OPEN to the right 
        // Same with the top and bottom
                doorLeft = doorRight;
                doorBottom = doorTop;

                if(createdZDoor)
                {
                    doorDown = !doorDown;
                    doorUp = !doorUp;
                }
                else
                {
                    doorUp = false;
                    doorDown = false;
                }

                doorRight = false;
                doorTop = false;

                // TODO : Make moor doors
                if(doorDirection == 2)
                {
                    if(absTileZ == screenBaseZ)
                    {
                        absTileZ = screenBaseZ + 1;
                    }
                    else
                    {
                        absTileZ = screenBaseZ;
                    }                
                }
                else if(doorDirection == 1)
                {
                    screenX += 1;
                }
                else
                {
                    screenY += 1;
                }
            }

            controlled_hero *conHero = gameState->controlledHeroes;
            conHero->boost = 0.0f;
            conHero->maxBoostFuel = 1.5f;
            conHero->boostFuel = conHero->maxBoostFuel;
            conHero->maxAttackFuel = 0.5f;
            conHero->attackFuel = conHero->maxAttackFuel;

            gameState->singleLaser = PushStruct(&gameState->worldArena, laser_spec_group);
            gameState->singleLaser->type = LaserType_Single;
            gameState->singleLaser->recoilAmount = 200.5f;
            gameState->singleLaser->fuelConsume = conHero->maxAttackFuel/20.0f;
            gameState->singleLaser->recoilRad = -Pi32;
            gameState->singleLaser->specCount = 1;
            gameState->singleLaser->dim = 0.5f;
            gameState->singleLaser->specs = PushArray(&gameState->worldArena, gameState->singleLaser->specCount, laser_spec);
            gameState->singleLaser->specs[0].directionRad = 0.0f;
            ++gameState->laserGroupCount;

            gameState->tripleLaser = PushStruct(&gameState->worldArena, laser_spec_group);
            gameState->tripleLaser->type = LaserType_Triple;
            gameState->tripleLaser->recoilAmount = 820.0f;
            gameState->tripleLaser->fuelConsume = conHero->maxAttackFuel/3.0f;
            gameState->tripleLaser->specCount = 5;
            gameState->tripleLaser->recoilRad = -Pi32;
            gameState->tripleLaser->dim = 0.5f;
            gameState->tripleLaser->specs = PushArray(&gameState->worldArena, gameState->tripleLaser->specCount, laser_spec);
            gameState->tripleLaser->specs[0].directionRad = -0.1f*Pi32;
            gameState->tripleLaser->specs[1].directionRad = -0.05f*Pi32;
            gameState->tripleLaser->specs[2].directionRad = 0.0f;
            gameState->tripleLaser->specs[3].directionRad = 0.05f*Pi32;
            gameState->tripleLaser->specs[4].directionRad = 0.1f*Pi32;
            ++gameState->laserGroupCount;

            gameState->backwardLaser = PushStruct(&gameState->worldArena, laser_spec_group);
            gameState->backwardLaser->type = LaserType_Backward;
            gameState->backwardLaser->recoilAmount = 150.5f;
            gameState->backwardLaser->fuelConsume = conHero->maxAttackFuel/25.0f;
            gameState->backwardLaser->recoilRad = 0.0f;
            gameState->backwardLaser->specCount = 1;
            gameState->backwardLaser->dim = 5.0f;
            gameState->backwardLaser->specs = PushArray(&gameState->worldArena, gameState->backwardLaser->specCount, laser_spec);
            gameState->backwardLaser->specs[0].directionRad = -Pi32;
            ++gameState->laserGroupCount;

            gameState->enemySingleLaser = PushStruct(&gameState->worldArena, laser_spec_group);
            gameState->enemySingleLaser->type = LaserType_Single;
            gameState->enemySingleLaser->recoilAmount = 250.5f;
            gameState->enemySingleLaser->fuelConsume = conHero->maxAttackFuel/16.0f;
            gameState->enemySingleLaser->recoilRad = -Pi32;
            gameState->enemySingleLaser->specCount = 1;
            gameState->enemySingleLaser->dim = 2.0f;
            gameState->enemySingleLaser->specs = PushArray(&gameState->worldArena, gameState->singleLaser->specCount, laser_spec);
            gameState->enemySingleLaser->specs[0].directionRad = 0.0f;

            gameState->currentLaser = gameState->singleLaser;
            gameState->nextLaser = gameState->singleLaser;

            world_position cameraPos = {};
            uint32 cameraTileX = screenBaseX*tilesPerWidth + tilesPerWidth/2;
            uint32 cameraTileY = screenBaseY*tilesPerHeight + tilesPerHeight/2;
            uint32 cameraTileZ = screenBaseZ;

            cameraPos = TilePositionToChunkPosition(gameState->world, cameraTileX, cameraTileY, cameraTileZ);
            gameState->cameraPos = cameraPos;
            gameState->cameraTileX = cameraTileX;
            gameState->cameraTileY = cameraTileY;
            gameState->cameraTileZ = cameraTileZ;
            AddStandardSpace(gameState, cameraTileX, cameraTileY, cameraTileZ);

            gameState->roomMinTileX = screenBaseX*tilesPerWidth;
            gameState->roomMinTileY = screenBaseY*tilesPerHeight;
            gameState->roomMinTileZ = screenBaseZ;

            gameState->roomMaxTileX = gameState->roomMinTileX + tilesPerWidth;
            gameState->roomMaxTileY = gameState->roomMinTileY + tilesPerHeight;
            gameState->roomMaxTileZ = screenBaseZ;

            gameState->roomDim = V2(tileSideInMeters*(gameState->roomMaxTileX - gameState->roomMinTileX),
                                    tileSideInMeters*(gameState->roomMaxTileY - gameState->roomMinTileY));

            AddSun(gameState, cameraTileX+25, cameraTileY+20, cameraTileZ-8, 0.6f);
            AddSun(gameState, cameraTileX-110, cameraTileY-60, cameraTileZ-4, 0.3f);

            AddTutorial(gameState, cameraTileX, cameraTileY, cameraTileZ-6, 0.6f);

            for(i32 cloudIndex = 0;
                cloudIndex < 40;
                ++cloudIndex)
            {
                AddEntityToRandomPosition(gameState, EntityType_Cloud);
            }

            for(i32 mountainIndex = 0;
                mountainIndex < 50;
                ++mountainIndex)
            {
                AddEntityToTileY(gameState, EntityType_Mountain);
            }

            // AddSponge(gameState, cameraTileX, cameraTileY, cameraTileZ);

			u32 timae = (u32)time(0)/10000;
            // TODO : More robust random generation
            DEBUGSeries = Seed(timae);

            gameState->maxTutorialDisplayTime = 8.0f;
            gameState->tutorialDisplayTime = gameState->maxTutorialDisplayTime;
            gameState->maxDigipenLogoDisplayTime = 2.2f;
            gameState->digipenLogoDisplayTime = gameState->maxDigipenLogoDisplayTime;
            gameState->remainingTimeForNextWave = 10.0f;

            gameState->isInitialized = true;
        }

        // NOTE : Initialize transient state
        Assert(sizeof(transient_state) <= memory->transientStorageSize);
        transient_state *tranState= (transient_state *)memory->transientStorage;
        if(!tranState->isInitialized)
        {
            InitializeArena(&tranState->tranArena, 
                (memory_index)(memory->transientStorageSize - sizeof(transient_state)),
                (uint8 *)memory->transientStorage + sizeof(transient_state));                

            tranState->renderQueue = memory->highPriorityQueue;
            tranState->lowPriorityQueue = memory->lowPriorityQueue;

            for(int taskIndex = 0;
                taskIndex < ArrayCount(tranState->backgroundTasks);
                ++taskIndex)
            {
                background_task_with_memory *backgroundTask = tranState->backgroundTasks + taskIndex;
                backgroundTask->beingUsed = false;
                // TODO : Check the size is good enough
                SubArena(&backgroundTask->arena, &tranState->tranArena, Megabytes(1));
                backgroundTask->tempMemory = {};
            }

            // Get all the assets;
            tranState->assets = 
                AllocateGameAssets(tranState, &tranState->tranArena, Megabytes(64));

            PlaySound(gameState, GetFirstSoundFrom(tranState->assets, Asset_Type_Music), 0.7f);

            tranState->groundBufferCount = 64;
            tranState->groundBuffers = 
                PushArray(&tranState->tranArena, tranState->groundBufferCount, ground_buffer);

            for(uint32 groundIndex = 0;
                groundIndex < tranState->groundBufferCount;
                ++groundIndex)
            {
                ground_buffer *groundBuffer = tranState->groundBuffers + groundIndex;
                groundBuffer->bitmap = MakeEmptyBitmap(&tranState->tranArena, groundBufferWidth, groundBufferHeight, false);
                groundBuffer->pos = NullPosition();
            }

            tranState->isInitialized = true;
        }

        // NOTE : Process mouse input
        v2 mousePos = V2((real32)input->mouseX, (real32)input->mouseY);

        for(int controllerIndex = 0;
            // controllerIndex < ArrayCount(input->controllers);
            controllerIndex < 1;
            ++controllerIndex)
        {
          game_controller *controller = &input->controllers[controllerIndex];
          game_controller *oldController = &oldInput->controllers[controllerIndex];
          controlled_hero *conHero = gameState->controlledHeroes + controllerIndex;

          if(controller->credit.endedDown)
          {
            gameState->shouldDisplayCredit = !gameState->shouldDisplayCredit;
          }

          // Only add if there was no entity
          if(conHero->entityIndex == 0)
          {
            if(controller->start.endedDown)
            {
                add_low_entity_result player = AddPlayer(gameState, gameState->cameraTileX, gameState->cameraTileY, gameState->cameraTileZ);
                gameState->controlledHeroes->entityIndex = player.storageIndex;
            }
        }
        else
        {
            sim_entity *player = &gameState->lowEntities[gameState->cameraFollowingEntityIndex].sim;
            conHero->dZ = 0.0f;

            conHero->ddPlayer = {};

            conHero->isSpacePressed = false;
            conHero->boost = 0.0f;

            if(controller->isAnalog)
            {
            //The player is using sticks
                conHero->ddPlayer = v2{controller->averageStickX, controller->averageStickY};
            }
            else
            {
                //Use digital movement tuning
                r32 ddPlayerSize = 0.4f;

                real32 ddfacingRad = 0.0f;
                r32 facingRadSpeed = 45.0f;

                if(controller->moveLeft.endedDown)
                {
                    ddfacingRad = facingRadSpeed*input->dtForFrame;
                }
                if(controller->moveRight.endedDown)
                {
                    ddfacingRad = -facingRadSpeed*input->dtForFrame;
                }

                real32 facingRadDrag = 10.0f;
                ddfacingRad -= facingRadDrag * conHero->dfacingRad;

                conHero->dfacingRad = ddfacingRad*input->dtForFrame + conHero->dfacingRad;

                conHero->facingRad += conHero->dfacingRad;
                if(controller->moveUp.endedDown)
                {
                    conHero->ddPlayer += V2(Cos(conHero->facingRad), Sin(conHero->facingRad));
                    if(controller->actionDown.endedDown)
                    {
                        if(conHero->boostFuel >= 0.0f)
                        {
                            conHero->boost = 120.0f;
                            conHero->boostFuel -= input->dtForFrame;
                            // conHero->moveKeyPressTime = 0
                        }
                    }
                    
                }
                else
                {
                    conHero->moveKeyPressTime = 0.5f;
                }
            }   

            if(!controller->actionDown.endedDown)
            {
                conHero->boostFuel += 3.0f*input->dtForFrame;
                if(conHero->boostFuel >= conHero->maxBoostFuel)
                {
                    conHero->boostFuel = conHero->maxBoostFuel;
                }
            }

            if(controller->actionUp.endedDown)
            {
                if(conHero->attackFuel >= 0.0f)
                {
                    conHero->isSpacePressed = true;
                }
            }
            else
            {
                conHero->attackFuel += input->dtForFrame;
                if(conHero->attackFuel >= conHero->maxAttackFuel)
                {
                   conHero->attackFuel = conHero->maxAttackFuel; 
                }
            }

            if(controller->start.endedDown)
            {
                gameState->shouldBeRestarted = true;
            }

            if(controller->back.endedDown)
            {
                conHero->isGPressed = !conHero->isGPressed;
            }

            if(controller->killPlayer.endedDown)
            {
                player->hitPointMax = -1;
            }
            else if(controller->credit.endedDown)
            {
            }
            else if(controller->mute.endedDown)
            {
            }
            else if(controller->nextWave.endedDown)
            {
                ++gameState->waveCount;
            }
        }
    }
//
// NOTE : Rendering
//

    temporary_memory renderMemory = BeginTemporaryMemory(&tranState->tranArena);

    loaded_bitmap drawBuffer_ = {};
    loaded_bitmap *drawBuffer = &drawBuffer_;
    drawBuffer->width = buffer->width;
    drawBuffer->height = buffer->height;
    drawBuffer->pitch = buffer->pitch;
    drawBuffer->memory = buffer->memory;

    // TODO : Find out what is the precies maxPushBufferSize is!
    render_group *renderGroup = AllocateRenderGroup(tranState->assets, &tranState->tranArena, Megabytes(4), drawBuffer->width, drawBuffer->height);

    Clear(renderGroup, V4(0.3f, 0.3f, 0.3f, 1.0f));

    real32 widthOfMonitor = 0.635f;
    real32 metersToPixels = (real32)drawBuffer->width * widthOfMonitor;
    // Perspective(renderGroup, drawBuffer->width, drawBuffer->height, metersToPixels, 0.6f, 150.5f);
    Perspective(renderGroup, drawBuffer->width, drawBuffer->height, metersToPixels, 0.6f, 85.5f);

    rect2 screenBound = GetCameraRectangleAtTarget(renderGroup);
    v2 screenCenter = 0.5f* V2i(drawBuffer->width, drawBuffer->height); 
    rect3 cameraBoundsInMeters = RectMinMax(V3(screenBound.min, 0.0f), V3(screenBound.max, 0.0f));
    cameraBoundsInMeters.min.z = -3.0f*gameState->typicalFloorHeight;
    cameraBoundsInMeters.max.z = 1.0f*gameState->typicalFloorHeight;

    // TODO : How big do we actually want to expand here?
    // TODO : Find out why if this is too big, it will be a problem!
    v3 simBoundsExpansion = V3(700.0f, 280.0f, 20.0f);
    // The center is (0, 0) because the cameraPos is (0, 0)!!
    rect3 simBounds = AddRadiusToRect(cameraBoundsInMeters, simBoundsExpansion);
    temporary_memory simMemory = BeginTemporaryMemory(&tranState->tranArena);
    world_position simCenterPos = gameState->cameraPos;
    sim_region *simRegion = BeginSim(&tranState->tranArena, 
                                    gameState, gameState->world, 
                                    simCenterPos, simBounds, 
                                    input->dtForFrame);

    v3 cameraPos = SubstractTwoWMP(gameState->world, &gameState->cameraPos, &simCenterPos);

    // NOTE : Camera Shake
    v3 shakeOffset = {};
    {
        controlled_hero *conHero = gameState->controlledHeroes;
        if(conHero->isSpacePressed)
        {
            shakeOffset = ShakeCamera(0.0f, 0.5f);
        }
    }

    for(uint32 entityIndex = 0;
        entityIndex < simRegion->entityCount;
        ++entityIndex)
    {
        sim_entity *entity = simRegion->entities + entityIndex;

        if(entity->updatable) 
        {
            move_spec moveSpec = DefaultMoveSpec();
            v3 ddP = {};
            v3 ddPUnaffectedBySpeed = {};

            //
            // NOTE : Post phyiscs update
            //
            entity->isAnyMoveKeyPressed = false;
            switch(entity->type)
            {
                case EntityType_Player:
                {
                    moveSpec.unitMaxAccelVector = false;
                    moveSpec.speed = 70.0f; 
                    moveSpec.drag = V3(0.5f, 0.6f, 0.0f);
                    moveSpec.mass = 2.2f;

                    controlled_hero *conHero = gameState->controlledHeroes;

                    if(IsAlive(entity))
                    {
                        // Find out which controller is controlling this entity,x,,
                        // so that we don't pro140s this entity multiple times
                        if(entity->storageIndex == conHero->entityIndex)
                        {
                            if(conHero->dZ != 0.0f)
                            {
                                entity->dPos.z = conHero->dZ;
                            }

                            if(conHero->ddPlayer.x != 0.0f || conHero->ddPlayer.y != 0.0f)
                            {
                                entity->isAnyMoveKeyPressed = true;
                            }

                            if(conHero->isGPressed)
                            {
                                entity->invincibleTime = 10.0f;
                            }
                            else
                            {
                                entity->invincibleTime = -1.0f;
                            }

                            entity->facingRad = conHero->facingRad;
                            r32 cos = Cos(entity->facingRad);
                            r32 sin = Sin(entity->facingRad);

                            moveSpec.speed += conHero->boost;
                            // moveSpec.speed = 0.0f;
                            ddP = V3(conHero->ddPlayer, 0);

                            if(entity->shouldThereBeRecoil)
                            {
                                entity->shouldThereBeRecoil = false;
#if 0
                                entity->dPos += entity->recoil;
#else
                                // ddP += entity->recoil;
                                ddPUnaffectedBySpeed = entity->recoil;
#endif
                            }
                        }
                    }

                    // entity->invincibleTime -= input->dtForFrame;
                }break;

                case EntityType_PlayerBullet:
                case EntityType_EnemyBullet:
                case EntityType_Bullet:
                {
                    if(entity->distanceLimit <= 0.0f)
                    {
                        MakeEntityNonSpatial(entity);
                        ClearCollisionRulesFor(gameState, entity->storageIndex);
                    }

#if 0
                    moveSpec.mass = 0.1f;

                    Universial Law of Gravity
                    // G * (m1 *m2 ) / r2;
                    real32 distanceSquare = DistanceBetweenSquare(entity->pos, gameState->enemyPos);
                    real32 F = (Grav32 * entity->mass * 300.0f) / distanceSquare; 
                    real32 a = F / entity->mass;
                    ddP = a * Normalize(gameState->enemyPos - entity->pos);
#endif
                }break;

                case EntityType_BigFollowingEnemy:
                case EntityType_Monster:
                {
                    moveSpec = entity->moveSpec;

                    if(IsAlive(entity))
                    {
                        entity->awaredPos = gameState->lowEntities[gameState->cameraFollowingEntityIndex].sim.pos - entity->pos;

                        ddP = entity->awaredPos;
                        v3 normalizedddP = Normalize(ddP);
                        entity->facingRad = ATan2(normalizedddP.y, normalizedddP.x);
                    }
                    else
                    {
                        entity->alpha -= 0.5f*input->dtForFrame;
                        if(entity->alpha <= 0.0f)
                        {
                            MakeEntityNonSpatial(entity);
                        }
                    }

                    entity->invincibleTime -= input->dtForFrame;
                }break;

                case EntityType_PatrolEnemy:
                {
                    moveSpec = entity->moveSpec;
                    
                    if(IsAlive(entity))
                    {
                        if(entity->awareCooldown <= 0.0f)
                        {
                            entity->awaredPos = gameState->lowEntities[gameState->cameraFollowingEntityIndex].sim.pos - entity->pos;
                            entity->awareCooldown = 2.0f;
                            PlaySound(gameState, GetFirstSoundFrom(tranState->assets, Asset_Type_EnemyAwareSound), 0.2f);
                        }

                        ddP = entity->awaredPos;
                        v3 normalizedddP = Normalize(ddP);
                        entity->facingRad = ATan2(normalizedddP.y, normalizedddP.x);
                    }
                    else
                    {
                        entity->alpha -= 0.5f*input->dtForFrame;
                        if(entity->alpha <= 0.0f)
                        {
                            MakeEntityNonSpatial(entity);
                        }
                    }

                    entity->attackCooldown -= input->dtForFrame;
                    entity->invincibleTime -= input->dtForFrame;
                }break;

                case EntityType_LaserShootingEnemy:
                {
                    moveSpec = entity->moveSpec;
                    
                    if(IsAlive(entity))
                    {
                        ddP = entity->awaredPos;
                        v3 normalizedddP = Normalize(ddP);
                        entity->facingRad = ATan2(normalizedddP.y, normalizedddP.x);
                    }
                    else
                    {
                        entity->alpha -= 0.5f*input->dtForFrame;
                        if(entity->alpha <= 0.0f)
                        {
                            MakeEntityNonSpatial(entity);
                        }
                    }

                    entity->awareCooldown -= input->dtForFrame;
                    entity->invincibleTime -= input->dtForFrame;
                }break;

                default:
                {
                // InvalidCodePath;
                }break;
            }

            // Move every entity that was set special && movable
            if(!IsSet(entity, EntityFlag_Nonspatial) && IsSet(entity, EntityFlag_Movable))
            {
                MoveEntity(gameState, simRegion, entity, input->dtForFrame, &moveSpec, ddP, ddPUnaffectedBySpeed);
            }

#if 1
            renderGroup->transform.entitySimPos = GetEntityGroundPoint(entity);
            if(entity->type != EntityType_Player)
            {
                renderGroup->transform.entitySimPos += shakeOffset;
            }
#else
            renderGroup->transform.entitySimPos = GetEntityGroundPoint(entity) + V3(shakeOffset, 0.0f);
#endif

            //
            // NOTE : Post physics update
            //
            r32 cosForStraightBitmap = Cos(entity->facingRad + 0.5f*Pi32);
            r32 sinForStraightBitmap = Sin(entity->facingRad + 0.5f*Pi32);
            r32 cos = Cos(entity->facingRad);
            r32 sin = Sin(entity->facingRad);

            switch(entity->type)
            {
                case EntityType_Player:
                {
                    controlled_hero *conHero = gameState->controlledHeroes;
                    if(IsAlive(entity))
                    {
                        b32 shouldRenewAttackParticle = false;
                        if(conHero->isSpacePressed)
                        {
                            shouldRenewAttackParticle = true;
                            raycast2d_result raycastResult = SetLaser(entity, gameState->currentLaser, simRegion, renderGroup);

                            ProcessParticles(&entity->emitter3, 1.0f, Asset_Type_LaserSurrounding, ParticleType_LaserHit, 1.0f, 
                                        V3(raycastResult.hitPos, 0), V3(0, 0, 0), entity->facingRad+Pi32, renderGroup, 
                                        &DEBUGSeries, input->dtForFrame, shouldRenewAttackParticle);

                            PlaySound(gameState, GetFirstSoundFrom(tranState->assets, Asset_Type_LaserSound), 0.1f);

                            conHero->attackFuel -= gameState->currentLaser->fuelConsume;
                        }

                        if(gameState->currentLaser->type == LaserType_Triple)
                        {
                            ProcessParticles(&entity->emitter2, entity->alpha, Asset_Type_LaserSurrounding, ParticleType_Shotgun, 1.5f, entity->pos, V3(0, 0, 0), 
                                        entity->facingRad + Pi32 + gameState->currentLaser->recoilRad, renderGroup, 
                                            &DEBUGSeries, input->dtForFrame, shouldRenewAttackParticle);
                        }
                        else
                        {
                            ProcessParticles(&entity->emitter2, entity->alpha, Asset_Type_LaserSurrounding, ParticleType_LaserSurrounding, 2.0f, entity->pos, V3(0, 0, 0), 
                                        entity->facingRad + Pi32 + gameState->currentLaser->recoilRad, renderGroup, 
                                            &DEBUGSeries, input->dtForFrame, shouldRenewAttackParticle);
                        }
                    }

                    // ShootLaser(&entity->currentLaser, simRegion, renderGroup, tranState->assets);

                    v3 thrustOffset = V3(Rotate(V2(-1.5f, 0.0f), cos, sin), 0.0f);
                    v4 color = V4(1, 1, 1, 1);

                    particle_type particleType = ParticleType_Thrust;

                    if(conHero->boost != 0.0f)
                    {
                        particleType = ParticleType_BoostedThrust;
                    }

                    if(IsAlive(entity))
                    {
                        ProcessParticles(&entity->emitter, entity->alpha, Asset_Type_Shadow, particleType, 0.5f, entity->pos, thrustOffset, entity->facingRad, renderGroup, 
                                    &DEBUGSeries, input->dtForFrame, entity->isAnyMoveKeyPressed);

                        if(entity->isAnyMoveKeyPressed && conHero->moveKeyPressTime >= 0.0f)
                        {
                            PlaySound(gameState, GetFirstSoundFrom(tranState->assets, Asset_Type_BoostSound), 0.08f);
                            conHero->moveKeyPressTime -= input->dtForFrame;
                        }

                        DrawShield(entity, renderGroup, 100.0f, enemyColor);
                    }
                    else
                    {
                        particleType = ParticleType_Smoke;
                        ProcessParticles(&entity->emitter, entity->alpha, Asset_Type_Shadow, particleType, 0.8f, entity->pos, thrustOffset, entity->facingRad, renderGroup, 
                                    &DEBUGSeries, input->dtForFrame, true);
                        color = V4(1, 0.1f, 0.1f, 1.0f);
                    }

                    if(IsAlive(entity))
                    {
                        r32 boostFuelRatio = conHero->boostFuel/conHero->maxBoostFuel;
                        r32 boostRad = conHero->boostFuel*Pi32;
                        PushBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_Shield), 
                                        20.0f*boostFuelRatio, V3(0, 0, 0), Cos(boostRad), Sin(boostRad), V4(0.2f, 0.2f, 0.8f, 1.0f - boostFuelRatio));

                        r32 attackFuelRatio = conHero->attackFuel/conHero->maxAttackFuel;
                        r32 attackFuelRad = conHero->attackFuel*-Pi32;
                        PushBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_Shield), 
                                    40.0f*attackFuelRatio, V3(0, 0, 0), Cos(attackFuelRad), Sin(attackFuelRad), 
                                    V4(RandomBetween(&DEBUGSeries, 0.2f, 1.0f), RandomBetween(&DEBUGSeries, 0.2f, 1.0f), RandomBetween(&DEBUGSeries, 0.2f, 1.0f), 1.0f - attackFuelRatio));

                        // NOTE : Draw Aim
                        v3 aimPosition = {};
                        r32 laserTypeIndex = 0.0f;
                        r32 size = 0.0f;
                        switch(gameState->currentLaser->type)
                        {
                            case LaserType_Single:
                            {
                                laserTypeIndex = 0.0f;
                                aimPosition = V3(Rotate(V2(5.0f, 0.0f), cos, sin), 0.0f);
                                size = 4.5f;
                            }break;
                            case LaserType_Triple:
                            {
                                laserTypeIndex = 1.0f;
                                aimPosition = V3(Rotate(V2(6.5f, 0.0f), cos, sin), 0.0f);
                                size = 7.0f;
                            }break;
                            case LaserType_Backward:
                            {
                                laserTypeIndex = 0.0f;
                                aimPosition = V3(Rotate(V2(-7.0f, 0.0f), cos, sin), 0.0f);
                                size = 8.0f;
                            }break;
                        }
                        asset_match_vector matchVector = {};
                        matchVector.e[Asset_Tag_WeaponType] = laserTypeIndex;
                        asset_weight_vector weightVector = {};
                        weightVector.e[Asset_Tag_WeaponType] = 1.0f;
                        PushBitmap(renderGroup, GetBestMatchBitmap(renderGroup->assets, Asset_Type_Aim, &matchVector, &weightVector), size, 
                                    aimPosition, cos, sin, V4(1, 1, 1, 0.65f), false);

                        // DrawShield(entity, renderGroup, 3.0f, playerColor);
                    }

                    asset_match_vector matchVector = {};
                    matchVector.e[Asset_Tag_FacingDirection] = GetEntityFacingDirection(cos);
                    asset_weight_vector weightVector = {};
                    weightVector.e[Asset_Tag_FacingDirection] = 1.0f;
                    PushBitmap(renderGroup, GetBestMatchBitmap(tranState->assets, Asset_Type_Ship, &matchVector, &weightVector), 
                                5.0f, V3(0, 0, 0), cos, sin, color, true);


                    // TODO : Draw these hitpoints again! Or draw it in orthogonal mode?
                    // DrawHitpoints(entity, renderGroup);
                }break;

                case EntityType_Moon:
                {
                    PushBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_Tree), 2.5f, V3(0, 0, 0));
                }break;

                case EntityType_PlayerBullet:
                {
                    PushBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_Sword), 1.5f, V3(0, 0, 0), cosForStraightBitmap, sinForStraightBitmap, V4(0.2f, 1.0f, 0.8f, 1.0f), true);
                }break;

                case EntityType_EnemyBullet:
                {
                    PushBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_Sword), 1.5f, V3(0, 0, 0), cosForStraightBitmap, sinForStraightBitmap, V4(1.0f, 0.8f, 0.8f, 1.0f), true);
                }break;

                case EntityType_Bullet:
                {
    				PushBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_Sword), 1.5f, V3(0, 0, 0), cosForStraightBitmap, sinForStraightBitmap);
                }break;

                case EntityType_Wall:
                {
                    PushBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_Tree), 2.5f, V3(0, 0, 0));
                }break;

                case EntityType_PatrolEnemy:
                {
                    // TODO : More robust health checking system
                    v3 thrustOffset = V3(Rotate(V2(-1.5f, 0.0f), cos, sin), 0.0f);

                    if(IsAlive(entity))
                    {
                        ProcessParticles(&entity->emitter, entity->alpha, Asset_Type_Shadow, ParticleType_EnemyThrust, 0.5f, entity->pos, thrustOffset, entity->facingRad, renderGroup, 
                                        &DEBUGSeries, input->dtForFrame, true);

                        r32 length = (r32)buffer->width;
                        v3 rectPos = Rotate(V3(0.5f*length, 0, 0), cos, sin);
                        PushBitmapWithDesiredHeight(renderGroup, GetFirstBitmapFrom(renderGroup->assets, Asset_Type_RaycastAttack), 
                            length, rectPos, 
                            0.5f, cos, sin, V4(enemyColor, 0.3f), true);

                        PushBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_Arrow), 
                                4.0f, 0.15f*rectPos, cos, sin, V4(enemyColor, 1.0f), true);
                        PushBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_Arrow), 
                                4.0f, 0.3f*rectPos, cos, sin, V4(enemyColor, 1.0f), true);
                        PushBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_Arrow), 
                                4.0f, 0.45f*rectPos, cos, sin, V4(enemyColor, 1.0f), true);
                        PushBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_Arrow), 
                                4.0f, 0.6f*rectPos, cos, sin, V4(enemyColor, 1.0f), true);
                        PushBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_Arrow), 
                                4.0f, 0.75f*rectPos, cos, sin, V4(enemyColor, 1.0f), true);
                        PushBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_Arrow), 
                                4.0f, 0.9f*rectPos, cos, sin, V4(enemyColor, 1.0f), true);

                        DrawShield(entity, renderGroup, 3.0f, enemyColor);
                    }
                    else
                    {

                        // DrawDeath(entity, renderGroup, 3.0f, playerColor);
                        ProcessParticles(&entity->emitter, entity->alpha, Asset_Type_Shadow, ParticleType_Smoke, 0.8f, entity->pos, thrustOffset, entity->facingRad, renderGroup, 
                                    &DEBUGSeries, input->dtForFrame, true);
                    }

                    asset_match_vector matchVector = {};
                    matchVector.e[Asset_Tag_FacingDirection] = GetEntityFacingDirection(cos);
                    asset_weight_vector weightVector = {};
                    weightVector.e[Asset_Tag_FacingDirection] = 1.0f;
                    PushBitmap(renderGroup, GetBestMatchBitmap(tranState->assets, Asset_Type_Ship, &matchVector, &weightVector), 
                                5.0f, V3(0, 0, 0), Cos(entity->facingRad), Sin(entity->facingRad), V4(1.0f, 0.1f, 0.1f, entity->alpha), true);
                }break;

                case EntityType_Monster:
                {
                    // TODO : More robust health checking system
                    v3 thrustOffset = V3(Rotate(V2(-1.5f, 0.0f), cos, sin), 0.0f);

                    if(IsAlive(entity))
                    {
                        ProcessParticles(&entity->emitter, entity->alpha, Asset_Type_Shadow, ParticleType_EnemyThrust, 0.5f, entity->pos, thrustOffset, entity->facingRad, renderGroup, 
                                    &DEBUGSeries, input->dtForFrame, true);

                        DrawShield(entity, renderGroup, 3.0f, enemyColor);
                    }
                    else
                    {
                        PlaySound(gameState, GetFirstSoundFrom(tranState->assets, Asset_Type_EnemyDeathSound), 0.01f);
                        // DrawDeath(entity, renderGroup, 3.0f, playerColor);
                        ProcessParticles(&entity->emitter, entity->alpha, Asset_Type_Shadow, ParticleType_Smoke, 0.8f, entity->pos, thrustOffset, entity->facingRad, renderGroup, 
                                    &DEBUGSeries, input->dtForFrame, true);
                    }

                    asset_match_vector matchVector = {};
                    matchVector.e[Asset_Tag_FacingDirection] = GetEntityFacingDirection(cos);
                    asset_weight_vector weightVector = {};
                    weightVector.e[Asset_Tag_FacingDirection] = 1.0f;
                    PushBitmap(renderGroup, GetBestMatchBitmap(tranState->assets, Asset_Type_Ship, &matchVector, &weightVector), 
                                5.0f, V3(0, 0, 0), Cos(entity->facingRad), Sin(entity->facingRad), V4(1.0f, 0.1f, 0.1f, entity->alpha), true);

                }break;

                case EntityType_BigFollowingEnemy:
                {
                    v3 thrustOffset = V3(Rotate(V2(-1.5f, 0.0f), cos, sin), 0.0f);

                    if(IsAlive(entity))
                    {
                        ProcessParticles(&entity->emitter, entity->alpha, Asset_Type_Shadow, ParticleType_EnemyThrust, 0.5f, entity->pos, thrustOffset, entity->facingRad, renderGroup, 
                                    &DEBUGSeries, input->dtForFrame, true);

                        DrawShield(entity, renderGroup, 6.0f, enemyColor);
                    }
                    else
                    {
                        PlaySound(gameState, GetFirstSoundFrom(tranState->assets, Asset_Type_EnemyDeathSound), 0.01f);
                        // DrawDeath(entity, renderGroup, 3.0f, playerColor);
                        ProcessParticles(&entity->emitter, entity->alpha, Asset_Type_Shadow, ParticleType_Smoke, 0.8f, entity->pos, thrustOffset, entity->facingRad, renderGroup, 
                                    &DEBUGSeries, input->dtForFrame, true);
                    }

                    asset_match_vector matchVector = {};
                    matchVector.e[Asset_Tag_FacingDirection] = GetEntityFacingDirection(cos);
                    asset_weight_vector weightVector = {};
                    weightVector.e[Asset_Tag_FacingDirection] = 1.0f;
                    PushBitmap(renderGroup, GetBestMatchBitmap(tranState->assets, Asset_Type_Ship, &matchVector, &weightVector), 
                                10.0f, V3(0, 0, 0), Cos(entity->facingRad), Sin(entity->facingRad), V4(1.0f, 0.1f, 0.1f, entity->alpha), true);

                }break;

                case EntityType_LaserShootingEnemy:
                {
                    if(IsAlive(entity))
                    {
                        // NOTE : Draw laser indication
                        r32 length = (r32)buffer->width;
                        v3 rectPos = Rotate(V3(0.5f*length, 0, 0), cos, sin);
                        v4 color = V4(RandomBetween(&DEBUGSeries, 0.0f, 1.0f), RandomBetween(&DEBUGSeries, 0.0f, 0.7f), RandomBetween(&DEBUGSeries, 0.0f, 0.7f), (4.0f-entity->attackCooldown)/16.0f);
                        PushBitmapWithDesiredHeight(renderGroup, GetFirstBitmapFrom(renderGroup->assets, Asset_Type_RaycastAttack), 
                                length, rectPos, 
                                gameState->singleLaser->dim, cos, sin, color, true);

                        b32 shouldRenewAttackParticle = false;
                        if(entity->awareCooldown <= 1.0f)
                        {
                            shouldRenewAttackParticle = true;
                            raycast2d_result raycastResult = SetLaser(entity, gameState->enemySingleLaser, simRegion, renderGroup);

                            ProcessParticles(&entity->emitter3, 1.0f, Asset_Type_LaserSurrounding, ParticleType_LaserHit, 1.0f, 
                                        V3(raycastResult.hitPos, 0), V3(0, 0, 0), entity->facingRad+Pi32, renderGroup, 
                                        &DEBUGSeries, input->dtForFrame, shouldRenewAttackParticle);

                            PlaySound(gameState, GetFirstSoundFrom(tranState->assets, Asset_Type_EnemyLaserSound), 0.05f);
                            if(entity->awareCooldown <= 0.0f)
                            {
                                entity->awaredPos = gameState->lowEntities[gameState->cameraFollowingEntityIndex].sim.pos - entity->pos;
                                entity->awareCooldown = 4.0f;
                            }

                            // PlaySound(gameState, GetFirstSoundFrom(tranState->assets, Asset_Type_LaserSound), 0.1f);

                        }
                        else
                        {
                            PlaySound(gameState, GetFirstSoundFrom(tranState->assets, Asset_Type_EnemyDeathSound), 0.01f);
                            PushBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_ExclamationMark), 
                                    15.0f*(4.0f-entity->awareCooldown)/4.0f, 0.15f*rectPos, 1.0f, 0.0f, V4(1.0f, 1.0f, 1.0f, entity->alpha), true);

                            PushBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_ExclamationMark), 
                                    15.0f*(4.0f-entity->awareCooldown)/4.0f, 0.3f*rectPos, 1.0f, 0.0f, V4(1.0f, 1.0f, 1.0f, entity->alpha), true);

                            PushBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_ExclamationMark), 
                                    15.0f*(4.0f-entity->awareCooldown)/4.0f, 0.45f*rectPos, 1.0f, 0.0f, V4(1.0f, 1.0f, 1.0f, entity->alpha), true);

                            PushBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_ExclamationMark), 
                                    15.0f*(4.0f-entity->awareCooldown)/4.0f, 0.6f*rectPos, 1.0f, 0.0f, V4(1.0f, 1.0f, 1.0f, entity->alpha), true);

                            PushBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_ExclamationMark), 
                                    15.0f*(4.0f-entity->awareCooldown)/4.0f, 0.75f*rectPos, 1.0f, 0.0f, V4(1.0f, 1.0f, 1.0f, entity->alpha), true);

                            PushBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_ExclamationMark), 
                                    15.0f*(4.0f-entity->awareCooldown)/4.0f, 0.9f*rectPos, 1.0f, 0.0f, V4(1.0f, 1.0f, 1.0f, entity->alpha), true);
                        }

                        DrawShield(entity, renderGroup, 6.0f, enemyColor);
                        ProcessParticles(&entity->emitter2, entity->alpha, Asset_Type_LaserSurrounding, ParticleType_LaserSurrounding, 2.0f, entity->pos, V3(0, 0, 0), entity->facingRad, renderGroup, 
                                                &DEBUGSeries, input->dtForFrame, shouldRenewAttackParticle);
                    }
                    else
                    {
                        // DrawDeath(entity, renderGroup, 3.0f, playerColor);
                        ProcessParticles(&entity->emitter, entity->alpha, Asset_Type_Shadow, ParticleType_Smoke, 0.8f, entity->pos, V3(0, 0, 0), entity->facingRad, renderGroup, 
                                &DEBUGSeries, input->dtForFrame, true);
                    }

                    asset_match_vector matchVector = {};
                    matchVector.e[Asset_Tag_FacingDirection] = 1.0f;
                    asset_weight_vector weightVector = {};
                    weightVector.e[Asset_Tag_FacingDirection] = 1.0f;
                    PushBitmap(renderGroup, GetBestMatchBitmap(tranState->assets, Asset_Type_Ship, &matchVector, &weightVector), 
                                10.0f, V3(0, 0, 0), Cos(entity->facingRad), Sin(entity->facingRad), V4(1.0f, 0.1f, 0.1f, entity->alpha), true);
                }break;

                case EntityType_Sponge:
                {
                }
                case EntityType_Space:
                {
                    for(uint32 volumeIndex = 0;
                        volumeIndex < entity->collision->volumeCount;
                        volumeIndex++)
                    {
                        sim_entity_collision_volume *volume = entity->collision->volumes + volumeIndex;

                        r32 volumeDimWidthOverHeight = volume->dim.x/volume->dim.y;

                        if(!IsInRectangle(RectCenterDim(entity->pos.xy+volume->offset.xy, V2(0.65f*volumeDimWidthOverHeight*volume->dim.y, 0.65f*volume->dim.y)), V2(0, 0)))
                        {
                            entity->alpha += 3.0f*input->dtForFrame;
                        }
                        else
                        {
                            entity->alpha -= 1.5f*input->dtForFrame;
                        }

                        entity->alpha = Clamp01MapInRange(0.0f, entity->alpha, 1.0f);

                        v4 outlineColor = V4(0.9f, 0.1f, 0.4f, entity->alpha);

                        // NOTE : This is working because the render transform is already moved to entity pos
                        PushRectOutline(renderGroup, volume->offset - V3(0, 0, 0.5f*volume->dim.z), volume->dim.xy, outlineColor, 1.0f);
                    }
                }break;

                case EntityType_Cloud:
                {
                    PushBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_Cloud), entity->size, V3(0, 0, 0), 1.0f, 0.0f, V4(1, 1, 1, entity->alpha), true);
                }break;               

                case EntityType_Mountain:
                {
                    PushBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_Mountain), entity->size, V3(0, 0, 0), 1.0f, 0.0f, V4(1, 1, 1, entity->alpha), true);
                }break;               

                case EntityType_Sun:
                {
                    PushBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_Sun), 100.5f, V3(0, 0, 0), 1.0f, 0.0f, V4(1, 1, 1, entity->alpha), true);
                }break;

                case EntityType_Tutorial:
                {
                    if(gameState->waveCount == 0)
                    {
                        if(gameState->cameraFollowingEntityIndex != 0)
                        {
                            r32 alpha = gameState->remainingTimeForNextWave/5.0f;
                            PushBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_Tutorial), 70.5f, V3(0, 0, 0), 1.0f, 0.0f, V4(1, 1, 1, alpha), false);
                        }
                    }
                    else
                    {
                        sim_entity *player = &gameState->lowEntities[gameState->cameraFollowingEntityIndex].sim;
                        if(IsAlive(player))
                        {
                            renderGroup->transform.entitySimPos.xy = player->pos.xy;
                            if(gameState->remainingTimeForNextWave >= 4.0f)
                            {
                                r32 alpha = Clamp01((gameState->remainingTimeForNextWave-8.0f)/2.0f);
                                PushBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_Wave), 10.5f, V3(-10.0f, 0, 0), 1.0f, 0.0f, V4(1, 1, 1, alpha), false);
                                DrawNumbers(gameState->waveCount, 20.0f, V2(35.0f, 0), V2(15.0f, 0.0f), renderGroup, V4(playerColor, alpha));
                            }
                            else
                            {
                                DrawNumbers((i32)gameState->remainingTimeForNextWave, 10.0f, V2(0.0f, 0.0f), V2(0.0f, 0.0f), renderGroup, V4(enemyColor, 0.5f));
                                 if(gameState->nextLaser)
                                {
                                    r32 laserTypeIndex = 0.0f;

                                    switch(gameState->currentLaser->type)
                                    {
                                        case LaserType_Single:
                                        {
                                            laserTypeIndex = 0.0f;
                                        }break;
                                        case LaserType_Triple:
                                        {
                                            laserTypeIndex = 1.0f;
                                        }break;
                                        case LaserType_Backward:
                                        {
                                            laserTypeIndex = 2.0f;
                                        }break;
                                    }

                                    asset_match_vector matchVector = {};
                                    matchVector.e[Asset_Tag_WeaponType] = laserTypeIndex;
                                    asset_weight_vector weightVector = {};
                                    weightVector.e[Asset_Tag_WeaponType] = 1.0f;
                                    PushBitmap(renderGroup, GetBestMatchBitmap(renderGroup->assets, Asset_Type_NextWeaponIndicator, &matchVector, &weightVector), 9.0f, 
                                                V3(-20.0f, -10.0f, 0), 1.0f, 0.0f, V4(1, 1, 1, 0.3f), false);

                                    PushBitmap(renderGroup, GetBestMatchBitmap(renderGroup->assets, Asset_Type_ArrowIndicator, &matchVector, &weightVector), 8.0f, 
                                                V3(0.0f, -10.0f, 0), 1.0f, 0.0f, V4(1, 1, 1, 0.5f), false);

                                    switch(gameState->nextLaser->type)
                                    {
                                        case LaserType_Single:
                                        {
                                            laserTypeIndex = 0.0f;
                                        }break;
                                        case LaserType_Triple:
                                        {
                                            laserTypeIndex = 1.0f;
                                        }break;
                                        case LaserType_Backward:
                                        {
                                            laserTypeIndex = 2.0f;
                                        }break;
                                    }

                                    matchVector = {};
                                    matchVector.e[Asset_Tag_WeaponType] = laserTypeIndex;
                                    weightVector = {};
                                    weightVector.e[Asset_Tag_WeaponType] = 1.0f;
                                    PushBitmap(renderGroup, GetBestMatchBitmap(renderGroup->assets, Asset_Type_NextWeaponIndicator, &matchVector, &weightVector), 14.0f, 
                                                V3(20.0f, -10.0f, 0), 1.0f, 0.0f, V4(1, 1, 1, 0.5f), false); }
                            }
                        }
                    }
                }break;

                default:
                {
                    // TODO : Get this back on!
                    // InvalidCodePath;
                }break;
            }
            // NOTE : Draw volumeIndex? 
#if 0
            for(uint32 volumeIndex = 0;
                volumeIndex < entity->collision->volumeCount;
                volumeIndex++)
            {
                sim_entity_collision_volume *volume = entity->collision->volumes + volumeIndex;
                v4 outlineColor = V4(1, 0, 1, 1);

                // NOTE : This is working because the render transform is already moved to entity pos
                PushRectOutline(renderGroup, volume->offset - V3(0, 0, 0.5f*volume->dim.z), volume->dim.xy, outlineColor);
            }
#endif

#if 0
            // NOTE : For world projection
            // TODO : Make this for menu?
            renderGroup->transform.entitySimPos = V3(0, 0, 0);
            renderGroup->transform.scale = 1.0f;
            v2 meterMousePos = mousePos*(1.0f/renderGroup->transform.metersToPixels);
            r32 localZ = 10.0f;
            v2 worldMouseP = Unproject(renderGroup, meterMousePos, localZ);
            renderGroup->transform.entitySimPos = V3(worldMouseP, renderGroup->transform.distanceAboveTarget - localZ);
            PushRect(renderGroup, V3(0, 0, 0), V2(1.0f, 1.0f), V4(0.0f, 1.0f, 1.0f, 1.0f));
#endif
        }
    }

    // NOTE : Wave system!
    if(gameState->remainingTimeForNextWave < 0.0f)
    {
        PlaySound(gameState, GetFirstSoundFrom(tranState->assets, Asset_Type_EnemyAwareSound), 0.5f);
        // NOTE : Add new enemies!
        i32 enemyCount = (i32)Clamp(0.0f, (r32)(NextRandomUInt32(&DEBUGSeries)%2 + 1 + gameState->waveCount/3), 10.0f);

        for(i32 enemyIndex = 0;
            enemyIndex < enemyCount;
            ++enemyIndex)
        {
            if(gameState->waveCount < 3)
            {
                switch(NextRandomUInt32(&DEBUGSeries)%3)
                {
                    default:
                    case 0:
                    {
                        AddEntityToRandomPosition(gameState, EntityType_Monster);
                    }break;
                    case 1:
                    {
                        AddEntityToRandomPosition(gameState, EntityType_PatrolEnemy);
                    }break;
                }
            }
            else
            {
                switch(NextRandomUInt32(&DEBUGSeries)%5)
                {
                    default:
                    case 0:
                    {
                        AddEntityToRandomPosition(gameState, EntityType_Monster);
                    }break;
                    case 1:
                    {
                        AddEntityToRandomPosition(gameState, EntityType_PatrolEnemy);
                    }break;
                    case 2:
                    {
                        AddEntityToRandomPosition(gameState, EntityType_LaserShootingEnemy);
                    }break;
                    case 3:
                    {
                        AddEntityToRandomPosition(gameState, EntityType_BigFollowingEnemy);
                    }break;
                }
            }
        }

        if(gameState->waveCount%2 == 1)
        {
            if(gameState->nextLaser == 0)
            {
                switch(NextRandomUInt32(&DEBUGSeries)%gameState->laserGroupCount)
                {
                    case 0:
                    {
                        gameState->nextLaser = gameState->singleLaser;
                    }break;
                    case 1:
                    {
                        gameState->nextLaser = gameState->tripleLaser;
                    }break;
                    case 2:
                    {
                        gameState->nextLaser = gameState->backwardLaser;
                    }break;
                }
            }
        }
        else
        {
            gameState->currentLaser = gameState->nextLaser;
            gameState->nextLaser = 0;
        }

        controlled_hero *conHero = gameState->controlledHeroes;
        // conHero->maxAttackFuel += 0.05f;

        // gameState->remainingTimeForNextWave = 10.0f + 2.0f*(gameState->waveCount/3);
        gameState->remainingTimeForNextWave = 10.0f;
        ++gameState->waveCount;
    }

    // NOTE : HUDs
    Orthographic(renderGroup, buffer->width, buffer->height, 1.0f);
    controlled_hero *conHero = gameState->controlledHeroes;
    if(conHero->entityIndex != 0)
    {

        sim_entity *player = &gameState->lowEntities[conHero->entityIndex].sim;
        if(!IsAlive(player))
        {
            DrawBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_PressR), 0.2f*buffer->width, 
                V3(0, 0.0f, 0), 1.0f, 0.0f, V4(RandomBetween(&DEBUGSeries, 0.0f, 1.0f), RandomBetween(&DEBUGSeries, 0.0f, 1.0f), RandomBetween(&DEBUGSeries, 0.0f, 1.0f), 1.0f), false);
        }
        else
        {
            gameState->remainingTimeForNextWave -= input->dtForFrame;
        }
    }
    else
    {
        if(gameState->digipenLogoDisplayTime > 0.0f)
        {
            // TODO : Somehow use this color?
            // Clear(renderGroup, V4(0.8f, 0.2f, 0.2f, 1.0f));
            Clear(renderGroup, V4(0.3f, 0.3f, 0.3f, 1.0f));
            DrawBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_DigipenLogo), 115.5f, 
                V3(0, 0, 0), 1.0f, 0.0f, V4(1, 1, 1, 1.0f), true);
        }
        else
        {
            DrawBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_Square), (r32)buffer->width, 
                V3(0, 0, 0), 1.0f, 0.0f, V4(0.05f, 0.05f, 0.05f, Clamp01(gameState->tutorialDisplayTime/gameState->maxTutorialDisplayTime)+0.3f), false);
            DrawBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_GameLogo), 250.5f, 
                V3(0, 0, 0), 1.0f, 0.0f, V4(1, 1, 1, 1.0f), false);
            DrawBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_PressR), 45.5f, 
                V3(0, -0.25f*buffer->height, 0), 1.0f, 0.0f, V4(RandomBetween(&DEBUGSeries, 0.0f, 1.0f), RandomBetween(&DEBUGSeries, 0.0f, 1.0f), RandomBetween(&DEBUGSeries, 0.0f, 1.0f), 1.0f), false);
            gameState->tutorialDisplayTime -= input->dtForFrame;
        }
        gameState->digipenLogoDisplayTime -= input->dtForFrame;
    }

    if(gameState->shouldDisplayCredit)
    {
        Clear(renderGroup, V4(0.3f, 0.3f, 0.3f, 1.0f));

        asset_match_vector matchVector = {};
        asset_weight_vector weightVector = {};

        matchVector.e[Asset_Tag_Sequence] = 0.0f;
        weightVector.e[Asset_Tag_Sequence] = 1.0f;
        PushBitmap(renderGroup, GetBestMatchBitmap(renderGroup->assets, Asset_Type_Credit, &matchVector, &weightVector), 
            0.2f*buffer->width, V3(-0.2f*buffer->width, 0.2f*buffer->height, 0.0f), 1.0f, 0.0f, V4(1, 1, 1, 1), false);

        matchVector.e[Asset_Tag_Sequence] = 1.0f;
        PushBitmap(renderGroup, GetBestMatchBitmap(renderGroup->assets, Asset_Type_Credit, &matchVector, &weightVector), 
            0.3f*buffer->width, V3(0.2f*buffer->width, 0.2f*buffer->height, 0.0f), 1.0f, 0.0f, V4(1, 1, 1, 1), false);

        matchVector.e[Asset_Tag_Sequence] = 2.0f;
        PushBitmap(renderGroup, GetBestMatchBitmap(renderGroup->assets, Asset_Type_Credit, &matchVector, &weightVector), 
            0.2f*buffer->width, V3(0.0f*buffer->width, -0.25f*buffer->height, 0.0f), 1.0f, 0.0f, V4(1, 1, 1, 1), false);
    }

    TiledRenderGroupToOutput(renderGroup, drawBuffer, tranState->renderQueue);

    if(gameState->shouldBeRestarted)
    {
        for(uint32 entityIndex = 0;
            entityIndex < simRegion->entityCount;
            ++entityIndex)
        {
            sim_entity *entity = simRegion->entities + entityIndex;
            if(entity->type != EntityType_Cloud &&
                entity->type != EntityType_Space)
            {
                if(entity->type == EntityType_Monster ||
                    entity->type == EntityType_PatrolEnemy||
                    entity->type == EntityType_LaserShootingEnemy||
                    entity->type == EntityType_BigFollowingEnemy)
                {
                    entity->hitPointMax = -1;
                }
                else if(entity->type == EntityType_Player)
                {
                    MakeEntityNonSpatial(entity);
                }
            }
        }

        conHero->entityIndex = AddPlayer(gameState, gameState->cameraTileX, gameState->cameraTileY, gameState->roomMinTileZ).storageIndex;
        gameState->cameraFollowingEntityIndex = conHero->entityIndex;
        conHero->maxAttackFuel = 0.50f;

        switch(NextRandomUInt32(&DEBUGSeries)%gameState->laserGroupCount)
        {
            default:
            case 0:
            {
                gameState->nextLaser = gameState->singleLaser;
            }break;
            case 1:
            {
                gameState->nextLaser = gameState->tripleLaser;
            }break;
            case 2:
            {
                gameState->nextLaser = gameState->backwardLaser;
            }break;
        }

        // AddStandardSpace(gameState, gameState->cameraTileX, gameState->cameraTileY, gameState->cameraTileZ);

        gameState->waveCount = 0;
        gameState->remainingTimeForNextWave = 5.0f;
        gameState->currentLaser = gameState->singleLaser;
        gameState->nextLaser = gameState->singleLaser;

        gameState->shouldBeRestarted = false; 
    }

    EndSim(simRegion, gameState, input->dtForFrame);
    EndTemporaryMemory(simMemory);
    EndTemporaryMemory(renderMemory);    

    CheckArena(&gameState->worldArena);
    CheckArena(&tranState->tranArena);

    END_TIMED_BLOCK(GameUpdateAndRender);
}


extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *gameState = (game_state *)memory->permanentStorage;
    transient_state *tranState = (transient_state *)memory->transientStorage;
    game_assets *assets = tranState->assets;
    //
    // NOTE : Outputsound
    //

    temporary_memory mixerMemory = BeginTemporaryMemory(&tranState->tranArena);

    real32 *realChannel0 = PushArray(mixerMemory.arena, soundBuffer->sampleCount, real32);
    real32 *realChannel1 = PushArray(mixerMemory.arena, soundBuffer->sampleCount, real32);

    //
    // NOTE : Clear the samples. 
    //
    real32 *destq = realChannel0;
    real32 *destw = realChannel1;
    for(int32 sampleIndex = 0;
        sampleIndex < soundBuffer->sampleCount;
        ++sampleIndex)
    {
        *destq++ = 0.0f;
        *destw++ = 0.0f;
    }

    // 
    // NOTE : Mix the sounds.
    //
    for(playing_sound **playingSoundPtr = &gameState->firstPlayingSound;
        *playingSoundPtr;
        )
    {
        playing_sound *playingSound = *playingSoundPtr; 
        bool32 isSoundFinishedPlaying = false;

        loaded_sound *loadedSound = GetLoadedSound(assets, playingSound->ID);
        if(loadedSound)
        {
            // Play sound or not?
#if 1
            real32 volume0 = playingSound->volume[0];
            real32 volume1 = playingSound->volume[1];
#else
            real32 volume0 = 0.0f;
            real32 volume1 = 0.0f;
#endif

            real32 *dest0 = realChannel0;
            real32 *dest1 = realChannel1;

            Assert(playingSound->samplesPlayed >= 0);

            uint32 samplesToMix = soundBuffer->sampleCount;
            uint32 samplesRemainingInSound = loadedSound->sampleCount - playingSound->samplesPlayed;
            if(samplesToMix > samplesRemainingInSound)
            {
                samplesToMix = samplesRemainingInSound;
            }

            for(uint32 sampleIndex = playingSound->samplesPlayed;
                sampleIndex < playingSound->samplesPlayed + samplesToMix;
                ++sampleIndex)
            { 
                // TODO : Stereo!
                real32 sampleValue = loadedSound->samples[0][sampleIndex];
                *dest0++ += volume0 * sampleValue; 
                *dest1++ += volume1 * sampleValue; 
            }

            playingSound->samplesPlayed += samplesToMix;
            isSoundFinishedPlaying = ((uint32)playingSound->samplesPlayed == loadedSound->sampleCount);
        }
        else
        {
            LoadSoundAsset(assets, playingSound->ID);
        }

        if(isSoundFinishedPlaying)
        {
			// Actually, this is getting rid of the sound from the firstPlayingSound list
			// Therefore, we have to change the value of this, not the pointer
            *playingSoundPtr = playingSound->next;

            playingSound->next = gameState->firstFreePlayingSound;
            gameState->firstFreePlayingSound = playingSound;
        }
        else
        {
			// We should not change the pointer, we should move to the next pointer.
            playingSoundPtr = &playingSound->next;
        }
    }

    //
    // NOTE : Convert to int16 space and put inside sound buffer
    //
    real32 *source0 = realChannel0;
    real32 *source1 = realChannel1;

    int16 *locToWrite = soundBuffer->samples;
    for(int sampleIndex = 0;
        sampleIndex < soundBuffer->sampleCount;
        ++sampleIndex)
    {
        *locToWrite++ = (int16)(*source0++ + 0.5f);
        *locToWrite++ = (int16)(*source1++ + 0.5f);
    }

    EndTemporaryMemory(mixerMemory);
}
