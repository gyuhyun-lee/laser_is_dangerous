#include "fox_sim_region.h"

inline move_spec
DefaultMoveSpec()
{
    move_spec result;

    result.unitMaxAccelVector = false;
    result.speed = 1.0f;
    result.drag = {};

    return result;
}

inline move_spec
MoveSpec(b32 unitMaxAccelVector, r32 speed, v3 drag, r32 mass)
{
    move_spec result;

    result.unitMaxAccelVector = unitMaxAccelVector;
    result.speed = speed;
    result.drag = drag;
    result.mass = mass;

    return result;
}

inline bool32 
IsAlive(sim_entity *entity)
{
    bool32 result = false;

    if(entity->hitPointMax > 0)
    {
        result = true;
    }

    return result;
}

inline bool32
IsEntityOverlappingRectangle(v3 pos, sim_entity_collision_volume volume, rect3 rectangle)
{
    // Just another minkowski thing
    // grow the rect by the size of the entity..
    rect3 grownRect = AddRadiusToRect(rectangle, 0.5f * volume.dim);
    bool32 result = IsInRectangle(grownRect, pos + volume.offset);

    return result;
}

// Get the position based on the region postition
// which means, camera position is (0, 0)
inline v3
GetSimSpacePos(sim_region *simRegion, low_entity *stored)
{
    // Map the entity into camera space
    // Totally garbage value
    // TODO : Do we want to set this to signaling NAN in debug mode
    // to make sure nobody else uses the position of a nonspatial entity?
    v3 diff = InvalidPos;
    if(!IsSet(&stored->sim, EntityFlag_Nonspatial))
    {
        diff = SubstractTwoWMP(simRegion->world, &stored->pos, &simRegion->origin);
    }
    return diff;
}

internal sim_entity_hash *
GetHashFromStorageIndex(sim_region *simRegion, uint32 storageIndex)
{
    Assert(storageIndex);

    sim_entity_hash *result = 0;

    uint32 hashValue = storageIndex;

    // Internal hash search
    // Search for all the hashs
    for(uint32 offset = 0;
        offset < ArrayCount(simRegion->hash);
        ++offset)
    {
        // ArrayCount is for getting only the low bits 
        uint32 hashMask = ArrayCount(simRegion->hash) - 1;

        // & with hashMask will keep the hashValue + offset value within the max count of hashes
        uint32 hashIndex = ((hashValue + offset) & hashMask);
        sim_entity_hash *entry = simRegion->hash + hashIndex;

        // if entry->index == 0, we found a empty hash before finding a hash that has corret index
        // which means this index was never stored
        // if entry->index == index, we found a hash that has same index, so we return it 
        if(entry->index == 0 || entry->index == storageIndex)
        {
            result = entry;
            break;
        }
    }

    return result;
}

inline void
StoreEntityReference(entity_reference *ref)
{
    if(ref->ptr != 0)
    {
        ref->index = ref->ptr->storageIndex;
    }
}

// This function will look inside the hash with the given storageIndex,
// And then return the sim entity pointer which is stored inside the hash
inline sim_entity *
GetEntityByStorageIndex(sim_region *simRegion, uint32 storageIndex)
{
    sim_entity_hash *entry = GetHashFromStorageIndex(simRegion, storageIndex);
    sim_entity *result = entry->ptr;

    return result;
}

internal sim_entity *
AddEntityToSimRegion(game_state *gameState, sim_region *region, uint32 storageIndex, low_entity *source, v3 *simPos);
inline void
LoadEntityReference(game_state *gameState, sim_region *simRegion, entity_reference *ref)
{
    // Check whether the added entity has entity reference 
    // that also needed to be loaded
    // if the index was 0, it means there are no entity referenced
    if(ref->index)
    {
        sim_entity_hash *entry = GetHashFromStorageIndex(simRegion, ref->index);

        if(entry->ptr == 0)
        {
            low_entity *low = gameState->lowEntities + ref->index;
            entry->index = ref->index;
            v3 simSpacePos = GetSimSpacePos(simRegion, low);            
            // The reference was not in there yet
            entry->ptr = AddEntityToSimRegion(gameState, simRegion, ref->index, low, &simSpacePos);
        }

        ref->ptr = entry->ptr;
    }
}

internal sim_entity *
AddEntityToSimRegionRaw(game_state *gameState, sim_region *simRegion, uint32 storageIndex, low_entity *source)
{
    Assert(storageIndex);

    sim_entity *entity = 0;

    sim_entity_hash *entry = GetHashFromStorageIndex(simRegion, storageIndex);

    // If this entity was never been stored in the sim region
    if(entry->ptr == 0)
    {
        if(simRegion->entityCount < simRegion->maxEntityCount)
        {
            entity = simRegion->entities + simRegion->entityCount++;

            // MapStorageIndexToHash 
            Assert(entry->index == 0 || entry->index == storageIndex);
            // If the entry hash was already set, this process is unnecassirly but doesn't matter
            // however, if the entry->index == 0, it means the hash is not set, so we have to set it
            // This is basically overkill XD
            entry->index = storageIndex;
            entry->ptr = entity;

            if(source)
            {
                // TODO : This should really be a decompression step, not a copy!
                *entity = source->sim;

                // Load Entity Reference that this sim entity has, which is sword for now.
                // This has to be here because if we did not do the copy, 
                // the sim does not have reference to the sowrd - it's empty!
                // LoadEntityReference(gameState, simRegion, &entity->bullets);
            }

            entity->storageIndex = storageIndex;
            entity->updatable = false;
        }
        else
        {
            InvalidCodePath;
        }
    }

    return entity;
}

// We get the stored entity and make it to simulation entity
internal sim_entity *
AddEntityToSimRegion(game_state *gameState, sim_region *simRegion, uint32 storageIndex, low_entity *source, 
                    v3 *simPos)
{
    sim_entity *dest = AddEntityToSimRegionRaw(gameState, simRegion, storageIndex, source);
    if(dest)
    {
        // TODO : Convert the stored entity to a simulation entity
        if(simPos)
        {
           // Assert(IsCanonical(simRegion->world, *simPos));
            dest->pos = *simPos;
dest->updatable = IsEntityOverlappingRectangle(dest->pos, dest->collision->totalVolume, simRegion->updatableBounds);
        }
        else
        {
            dest->pos = GetSimSpacePos(simRegion, source);
        }
    }

    return dest;
}

// start the simulation to update the entities
internal sim_region *
BeginSim(memory_arena *simArena, game_state *gameState, world *world, 
        world_position regionCenter, rect3 regionBounds,
        real32 dt)
{
    // TODO : Maybe don't take a gameState here, and make the low entities stored in the world?
    // For now, we need gameState to get the stored entites
    sim_region *simRegion = PushStruct(simArena, sim_region);
    ZeroStruct(simRegion->hash);

    // TODO : Try to make these get enforced more precisely
    simRegion->maxEntityRadius = 5.0f;
    simRegion->maxEntityVelocity = 1800.0f;
    // See how far can the entity go in one frame
    real32 updateSafetyMargin = simRegion->maxEntityRadius + dt*simRegion->maxEntityVelocity; 
    real32 updateSafetyMarginZ = 1.0f;

    simRegion->world = world;
    simRegion->origin = regionCenter;
    simRegion->updatableBounds = 
        AddRadiusToRect(regionBounds, V3(simRegion->maxEntityRadius, simRegion->maxEntityRadius, 0.0f));
    simRegion->bounds = 
        AddRadiusToRect(simRegion->updatableBounds, V3(updateSafetyMargin, updateSafetyMargin, updateSafetyMarginZ));

    // TODO : Need to be more specific about maxEntityCounts
    simRegion->maxEntityCount = 1024;
    simRegion->entityCount = 0;
    simRegion->entities = PushArray(simArena, simRegion->maxEntityCount, sim_entity);

#if 0
    // NOTE : First, add all the bullets
    for(u32 bulletIndex = 0;
        bulletIndex < gameState->bulletCount;
        ++bulletIndex)
    {
        uint32 storedEntityIndex = gameState->firstBulletIndex + bulletIndex;
        low_entity *stored = gameState->lowEntities + storedEntityIndex;
        v3 simSpacePos = GetSimSpacePos(simRegion, stored);
        simRegion->bullets[bulletIndex] = 
            AddEntityToSimRegion(gameState, simRegion, storedEntityIndex, stored, &simSpacePos);
    }


    // NOTE : Add all raycast bullets
    for(u32 raycastBulletIndex = 0;
        raycastBulletIndex < gameState->firstRaycastBulletIndex;
        ++raycastBulletIndex)
    {
        uint32 storedEntityIndex = gameState->firstRaycastBulletIndex + raycastBulletIndex;
        low_entity *stored = gameState->lowEntities + storedEntityIndex;
        v3 simSpacePos = GetSimSpacePos(simRegion, stored);
        simRegion->raycastBullets[raycastBulletIndex] = 
            AddEntityToSimRegion(gameState, simRegion, storedEntityIndex, stored, &simSpacePos);
    }
#endif
    
    world_position minChunkPos = MapIntoChunkSpace(world, simRegion->origin, GetMinCorner(simRegion->bounds));
    world_position maxChunkPos = MapIntoChunkSpace(world, simRegion->origin, GetMaxCorner(simRegion->bounds));

    for(int32 chunkZ = minChunkPos.chunkZ;
        chunkZ <= maxChunkPos.chunkZ;
        ++chunkZ)
    {
        for(int32 chunkY = minChunkPos.chunkY;
            chunkY <= maxChunkPos.chunkY;
            ++chunkY)
        {
            for(int32 chunkX = minChunkPos.chunkX;
                chunkX <= maxChunkPos.chunkX;
                ++chunkX)
            {
                world_chunk *chunk = GetWorldChunk(world, chunkX, chunkY, chunkZ);

                // There is a possibility that we don't get the chunk
                if(chunk)
                {
                    for(world_entity_block *block = &chunk->firstBlock;
                        block;
                        block = block->next)
                    {
                        for(uint32 entityIndexIndex = 0;
                            entityIndexIndex < block->entityCount;
                            ++entityIndexIndex)
                        {
                            uint32 storedEntityIndex = block->lowEntityIndexes[entityIndexIndex];
                            low_entity *stored = gameState->lowEntities + storedEntityIndex;

                            if(!IsSet(&stored->sim, EntityFlag_Nonspatial))
                            {
                                // Get the simspacepos so that when we add entity to the sim region,
                                // we can also set the position of that entity
                                v3 simSpacePos = GetSimSpacePos(simRegion, stored);
                                if(IsEntityOverlappingRectangle(simSpacePos, stored->sim.collision->totalVolume, simRegion->bounds))
                                {
                                    // TODO : Check a second rectangle to set the entity
                                    // to be movable or not?
                                    // Get the stored entity and add entity ot the sim region 
                                    // so we can process it.
                                    AddEntityToSimRegion(gameState, simRegion, storedEntityIndex, stored, &simSpacePos);
                                }
                            }
                        }
                    }
                }
            }    
        }
    }

    return simRegion;
}

// After the entites(in past, this was high entities) are processed, 
// move all the entities inside the region to the low space
internal void
EndSim(sim_region *simRegion, game_state *gameState, r32 dt)
{
    // TODO : Maybe don't take a gameState here, low entities shold be stored in the world?
    // For now, we need gameState to get the stored entites
    sim_entity *simEntity = simRegion->entities;
    for(uint32 entityIndex = 0;
        entityIndex < simRegion->entityCount;
        ++entityIndex, ++simEntity)
    {
        low_entity *storage = gameState->lowEntities + simEntity->storageIndex;
        
        // Just block copy the sim entity
        storage->sim = *simEntity;
        // Store the sword to the low space
        // StoreEntityReference(&storage->sim.bullets);

        // If the entityflag_nonspatial was set, it means it should go to the nullposition
        // if not, map into the chunkspace to store it.
        world_position newChunkBasedPos = 
            IsSet(simEntity, EntityFlag_Nonspatial) ? 
                NullPosition() : 
                MapIntoChunkSpace(gameState->world, simRegion->origin, simEntity->pos);

        // TODO : Save state back to the stored entity, once high entities
        // do state decompression, etc.
        ChangeEntityLocation(&gameState->worldArena, gameState->world, storage, simEntity->storageIndex,   
                            newChunkBasedPos);
    
        if(simEntity->storageIndex == gameState->cameraFollowingEntityIndex)
        {
            // NOTE: Make the camera to follow the player slowly.
            world_position newCameraPos = gameState->cameraPos;

            v3 differenceBetweenCameraAndPlayer = SubstractTwoWMP(gameState->world, &storage->pos, &newCameraPos);
            if(LengthSq(differenceBetweenCameraAndPlayer) != 0.0f)
            {
                r32 cameraSpeed = 4.0f;
                newCameraPos = 
                    AddV3ToWorldPosition(gameState->world, &newCameraPos, cameraSpeed*dt*differenceBetweenCameraAndPlayer);
            }
            else
            {
                newCameraPos = storage->pos;
            }

            // SubstractTwoWMP
            gameState->cameraPos = newCameraPos;
        }

    }
}

// It's just written in one form, but it can used for every situation - don't worry!
/*
    real32 wallX, line segment of the wall we are testing 
    real32 relX, one of the formal player position coord relative to the base position
    real32 relY, the other formal player position coord
    real32 entityDeltaX, one of the destination coord. destination means the point when tMin = 1.0f
    real32 entityDeltaY,
    real32 *tMin, 
    real32 minY, 
    real32 maxY
*/
struct test_wall
{
    real32 x;
    real32 relX; 
    real32 relY; 
    real32 deltaX; 
    real32 deltaY;
    real32 minY; 
    real32 maxY;
    v3 normal;
};

internal bool32
TestWall(real32 wallX, real32 relX, real32 relY, real32 entityDeltaX, real32 entityDeltaY,
                real32 minY, real32 maxY, real32 *tMin)
{
    bool32 collided = false;

    real32 tEpsilon = 0.001f;
    if(entityDeltaX != 0.0f)
    {
        //Equation : p0x(formal player position x) + t * dx(player delta x) = wx(Wall x)
        real32 tResult = (wallX - relX) / entityDeltaX;
        // y value of the new player position 
        real32 y = relY + tResult * entityDeltaY;

        // Check whether the tMin is greater than tResult
        // Which means, the value we computed is the closer collision point
        // We also need to check the tResult because if it's less than 0, 
        // it means the player have to go backward to hit the wall - which we don't care.
        if(tResult >= 0 && *tMin > tResult)
        {
            // Check whether the y is inside the bound(minY ~ maxY)
            // Which means, the player is going to collide with this certain wall
            if(y >= minY && y <= maxY)
            {
                // Always clamp to 0
                // because if the tresult is too small, it can be negative value
                *tMin = Maximum(0.0f, tResult - tEpsilon);
                collided = true;
            }
        }
    }

    return collided;
}

internal void
ClearCollisionRulesFor(game_state *gameState, uint32 storageIndex)
{
    // TODO : NEed to make a better data structure that allows
    // removal of collision rules without searching the entire table

    // NOTE : One way to make removal easy would be to always 
    // add_both_orders of the pairs of storage indices to the
    // hash table, so no matter which position the entity is in,
    // you can always find it. Then, when you do your first pass
    // through for removal, you just remember the original top
    // of the free list, and when your'e done, do a pass through all 
    // the new things on the free list, and remove the reverse of
    // those pairs

    uint32 hashBucket = storageIndex & (ArrayCount(gameState->collisionRules) - 1);

    for(uint32 hashTableIndex = 0;
        hashTableIndex < ArrayCount(gameState->collisionRules);
        ++hashTableIndex)
    {
        for(pairwise_collision_rule **rulePtr = &gameState->collisionRules[hashBucket];
            *rulePtr;
            )
        {
            if((*rulePtr)->storageIndexA == storageIndex ||
                (*rulePtr)->storageIndexB == storageIndex)
            {
                pairwise_collision_rule *removedRule = *rulePtr;
                *rulePtr = (*rulePtr)->nextInHash;

                // Set the First Free Collision Rule
                removedRule->nextInHash = gameState->firstFreeCollisionRule;
                gameState->firstFreeCollisionRule = removedRule;

                break;            
            }
        }
    }
}

internal void
AddCollisionRule(game_state *gameState, uint32 storageIndexA, uint32 storageIndexB, 
                // Rule we want to add
                bool32 canCollide) 
{
    if(storageIndexA > storageIndexB)
    {
        uint32 temp = storageIndexA;
        storageIndexA = storageIndexB;
        storageIndexB = temp;
    }

    pairwise_collision_rule *found = 0;
    // TODO : Better Hash Function .... LOL
    uint32 hashBucket = storageIndexA & (ArrayCount(gameState->collisionRules) - 1);
    for(pairwise_collision_rule *rule = gameState->collisionRules[hashBucket];
        rule;
        rule = rule->nextInHash)
    {
        if(rule->storageIndexA == storageIndexA && 
            rule->storageIndexB == storageIndexB)
        {
            found = rule;
            break;  
        }
    }

    if(!found)
    {
        // If we could not find it, check the first free collision rule
        found = gameState->firstFreeCollisionRule;

        if(found)
        {
            // Because now this place is full, set it to the nextInHash
            gameState->firstFreeCollisionRule = found->nextInHash;
        }
        else
        {
            // If we could not found it, make new one
            found = PushStruct(&gameState->worldArena, pairwise_collision_rule);
        }
        // Now we insert this to between the pointer and the thing that was pointed; 
        found->nextInHash = gameState->collisionRules[hashBucket];
        gameState->collisionRules[hashBucket] = found;
    }

    if(found)
    {
        found->storageIndexA = storageIndexA;
        found->storageIndexB = storageIndexB;
        found->canCollide = canCollide;
    }
}

// This function has nothing to do with the flag_collide
internal bool32
CanCollide(game_state *gameState, sim_entity *a, sim_entity *b)
{
    bool32 result = false;

    if(a != b)
    {
        // So that it can be pairwise!
        if(a->storageIndex > b->storageIndex)
        {
            sim_entity *temp = a;
            a = b;
            b = temp;
        }
        
        if(IsSet(a, EntityFlag_CanCollide) &&
            IsSet(b, EntityFlag_CanCollide))
        {
            if(!IsSet(a, EntityFlag_Nonspatial) &&
                !IsSet(b, EntityFlag_Nonspatial))
            {
                // TODO : Property based logic goes here!
                result = true;
            }                

            // TODO : Better Hash Function .... LOL
            // We always find the rule based on the first entity(which has smaller entitytype)
            // which means, this hash table is arragned from smaller entity type to bigger entity type
            uint32 hashBucket = a->storageIndex & (ArrayCount(gameState->collisionRules) - 1);

            for(pairwise_collision_rule *rule = gameState->collisionRules[hashBucket];
                rule;
                rule = rule->nextInHash)
            {
                // result will not change if there are no rules!
                if(rule->storageIndexA == a->storageIndex && 
                    rule->storageIndexB == b->storageIndex)
                {
                    // For now, we have only 1 rule
                    result = rule->canCollide;
                    break;            
                }
            }
        }
    }

    return result;
}

internal b32
HandleCollision(game_state *gameState, sim_entity *entity, sim_entity *hitEntity) 
{
    // TODO : More logic here!
    bool32 stopsOnCollision = false;

#if 0
    if(entity->type == EntityType_Bullet || entity->type == EntityType_PlayerBullet || entity->type == EntityType_EnemyBullet ||
        hitEntity->type == EntityType_Bullet || hitEntity->type == EntityType_PlayerBullet || hitEntity->type == EntityType_EnemyBullet)
    {
        // NOTE : Need this so that the entity does not collide with one entity multiple times
        AddCollisionRule(gameState, entity->storageIndex, hitEntity->storageIndex, false);        
        stopsOnCollision = false;
    }
    else
    {
        stopsOnCollision = true;        
    }
#endif

    sim_entity *a = entity;
    sim_entity *b = hitEntity;

    if(a->type > b->type)
    {
        sim_entity *temp = a;
        a = b;
        b = temp;
    }

    if(a->type == EntityType_Space)
    {
        stopsOnCollision = true;
    }

    //
    // NOTE : Collision Handling
    // 

    if(a->type == EntityType_Monster && 
        (b->type == EntityType_Monster || b->type == EntityType_PatrolEnemy))
    {
        stopsOnCollision = false;
    }

    else if(a->type == EntityType_Player && 
        (b->type == EntityType_Monster || 
            b->type == EntityType_PatrolEnemy || 
            b->type == EntityType_LaserShootingEnemy ||
            b->type == EntityType_BigFollowingEnemy))
    {
        if(IsAlive(b))
        {
            if(a->invincibleTime < 0.0f)
            {
                --a->hitPointMax;
            }
        }

        stopsOnCollision = false;
    }

    else if(a->type == EntityType_Space && 
        b->type == EntityType_Player)
    {
        if(b->invincibleTime < 0.0f)
        {
            --b->hitPointMax;
            b->invincibleTime = b->maxInvincibleTime;
        }
    }
    else if(a->type == EntityType_Space &&
            b->type == EntityType_PatrolEnemy)
    {
        b->awareCooldown = -1.0f;
    }

    return stopsOnCollision;
}

internal bool32
CanOverlap(game_state *gameState, sim_entity *movingEntity, sim_entity *regionEntity)
{
    bool32 result = false;

    // It should not overlap with itself
    if(movingEntity != regionEntity)
    {
        if(regionEntity->type == EntityType_Stairwell || 
            regionEntity->type == EntityType_Space)
        {
            result = true;
        }
    }

    return result;
}

internal void
HandleOverlap(game_state *gameState, sim_entity *mover, sim_entity *region, real32 dt, real32 *ground)
{
#if 0
    if(region->type == EntityType_Stairwell)
    {
        // *ground = GetStairGround(region, GetEntityGroundPoint(mover));
    }
#endif
}

// epsilon is for more forgiving check of what space is the entity in
// AND precise stop on the edge
internal bool32
EntitiesOverlap(sim_entity *entity, sim_entity *testEntity, v3 epsilon = V3(0, 0, 0))
{
    bool32 result = false;

    for(uint32 entityVolumeIndex = 0;
        !result && entityVolumeIndex < entity->collision->volumeCount;
        ++entityVolumeIndex)
    {
        sim_entity_collision_volume *volume = 
            entity->collision->volumes + entityVolumeIndex;

        for(uint32 testVolumeIndex = 0;
            !result && testVolumeIndex < testEntity->collision->volumeCount;
            ++testVolumeIndex)
        {

            sim_entity_collision_volume *testVolume = 
                testEntity->collision->volumes + testVolumeIndex;

            rect3 entityRect = RectCenterDim(entity->pos + volume->offset, 
                                            volume->dim + epsilon);

            rect3 testEntityRect = RectCenterDim(testEntity->pos + testVolume->offset, 
                                                testVolume->dim);
            // Intersect means the entity is inside other entity!!!
            // because both has dimenstion, and intersect means 
            // some part of a entity is inside(intersect) in other entity
            // It doesn't need to be really fully inside
            result = RectanglesIntersect(entityRect, testEntityRect);
        }
    }

    return result;
}

/*
    NOTE : This function is for the one who wants to check collision before the actual collision

*/
/*
    NOTE : This is how the stair works!
            Whenever the player collides with the stair, 
            See how high is the player from the ground of the stair(or lamp)
            If it's bigger than the stepHeight, don't allow the player to enter 
            the stair(lamp)
*/
internal bool32
SpeculativeCollide(sim_entity *mover, sim_entity *region, v3 testPos)
{
    bool32 result = true;

    if(region->type == EntityType_Stairwell)
    {
        r32 ground = 0.0f;
        // real32 ground = GetStairGround(region, GetEntityGroundPoint(mover));
        
        real32 stepHeight = 0.1f;
#if 0
        result = ((AbsoluteValue(GetEntityGroundPoint(mover).z - ground) > stepHeight) ||
                ((bary.y > 0.1f) && (bary.y < 0.9f)));
#else
        result = AbsoluteValue(GetEntityGroundPoint(mover).z - ground) > stepHeight;
#endif
    }

    return result;
}

// This is the actual collision detection for most of the entities
internal void
MoveEntity(game_state *gameState, sim_region *simRegion, sim_entity *entity, 
            real32 dtForFrame, move_spec *moveSpec, v3 ddP, v3 ddPUnaffectedBySpeed)
{
    // If the entity was nospatial, it should not be come here!
    Assert(!IsSet(entity, EntityFlag_Nonspatial));
    
    world *world = simRegion->world;

    // Sometimes, the ddP should be normalized
    // for example for the player, player will only change the direction,
    // in this case, we want it normalized.
    if(moveSpec->unitMaxAccelVector)
    {
        real32 ddPLength = LengthSq(ddP);
        // Normalize ddPlayer(accleration)
        if(ddPLength > 1.0f)
        {
            ddP *= 1.0f / Root2(ddPLength);
        }
    }

    ddP *= moveSpec->speed;
    ddP += ddPUnaffectedBySpeed;

    // TODO : ODE here
    ddP += -Hadamard(moveSpec->drag, entity->dPos);

    if(!IsSet(entity, EntityFlag_YSupported))
    {
        
#if 1
        // NOTE : Gravity
        ddP += moveSpec->mass * V3(0, -9.8f, 0);
#else
        ddP += V3(0, -9.8f, 0);
#endif

    }

    // Delta of the old position and the new position
    // Equation : new p = 1/2 * a * t * t + v * t + p0
    v3 entityDelta = 0.5f * ddP * Square(dtForFrame) + 
                    entity->dPos * dtForFrame; 

    // This is for ACTAULLY moving the entity, because it's velocity
    // Equation : new v = a * t + v0
    entity->dPos = ddP * dtForFrame + entity->dPos; 

    // TODO : For now, this is assert
    // Later, let's update this so that we can have safe update bound for sim region!
    Assert(LengthSq(entity->dPos) <= Square(simRegion->maxEntityVelocity));
    
    real32 distanceRemaining = entity->distanceLimit;
    
    // If there was no distancewLimit assigned for the entity, 
    // just let them move as far as they want(for example, player)
    if(entity->distanceLimit == 0.0f)
    {
        // TODO : Should this number be more formal?
        distanceRemaining = 1000000.0;
    }


    for(uint32 iteration = 0;
        iteration < 4;
        ++iteration)
    {
        real32 tMin = 1.0f;
        real32 tMax = 0.0f;
        
        real32 entityDeltaLength = Length(entityDelta);

        // TODO : What do we want to do for epsilons here?
        // Think this for the final collision code
        if(entityDeltaLength > 0.0f)
        {
            if(entityDeltaLength > distanceRemaining)
            {
                tMin = distanceRemaining / entityDeltaLength;
            }

            // Min is for outside collision
            // ex_ player vs monster
            // Max is for inside collision
            // ex_ player vs space(room)
            v3 wallNormalMin = {};
            v3 wallNormalMax = {};       

            // What entity the enity hit
            sim_entity *hitEntityMin = 0;
            sim_entity *hitEntityMax = 0;            

            v3 desiredPosition = entity->pos + entityDelta;

            if(!IsSet(entity, EntityFlag_Nonspatial))
            {
                // TODO : Spatial partition here!
                for(uint32 testEntityIndex = 0;
                    testEntityIndex < simRegion->entityCount;
                    testEntityIndex++)
                {
                    sim_entity *testEntity = simRegion->entities + testEntityIndex;
					if (testEntity->type == EntityType_Player)
					{
						i32 breakHereToo = 0;
					}

                    if(entity != testEntity)
                    {
                        real32 overlapEpsilon = 0.1f;

                        b32 canOverlap = IsSet(testEntity, EntityFlag_Traversable);
                        b32 entitiesOverlap = EntitiesOverlap(entity, testEntity, V3(overlapEpsilon));
                        // b32 entitiesOverlap = true;
                        b32 canCollide = CanCollide(gameState, entity, testEntity);

                        // NOTE : We should start checking the entities if
                        // 1. two entities can overlap and are overlapping
                        // OR
                        // 2. two entities can collide
                        if((canOverlap && 
                            entitiesOverlap) ||
                            canCollide)                    
                        {
                            for(uint32 entityVolumeIndex = 0;
                                entityVolumeIndex < entity->collision->volumeCount;
                                ++entityVolumeIndex)
                            {
                                sim_entity_collision_volume *volume = 
                                    entity->collision->volumes + entityVolumeIndex;

                                for(uint32 testVolumeIndex = 0;
                                    testVolumeIndex < testEntity->collision->volumeCount;
                                    ++testVolumeIndex)
                                {
                                    sim_entity_collision_volume *testVolume = 
                                        testEntity->collision->volumes + testVolumeIndex;
                                        
                                    // Resize wall based on the player
                                    // so we can treat the player as an area
                                    // TODO : Entities have heights?
                                    v3 minkowskiDiameter = 
                                        V3(testVolume->dim.x + volume->dim.x,
                                            testVolume->dim.y + volume->dim.y,
                                            testVolume->dim.z + volume->dim.z);

                                    // Get the tile rectangle
                                    v3 minCorner = -0.5f*minkowskiDiameter;
                                    v3 maxCorner = 0.5f*minkowskiDiameter;

                                    // Coordinates relative of the entity we are testing against
                                    v3 rel = (entity->pos + volume->offset) - (testEntity->pos + testVolume->offset);
                                    
                                    // NOTE : We should start checking collision
                                    // _ONLY IF_ the Z is in the bound 
                                    if(rel.z >= minCorner.z && rel.z < maxCorner.z)
                                    {
                                        test_wall walls[] = 
                                        {
                                            {maxCorner.x, rel.x, rel.y, entityDelta.x, entityDelta.y, minCorner.y, maxCorner.y, V3(1, 0, 0)},                    
                                            {minCorner.x, rel.x, rel.y, entityDelta.x, entityDelta.y, minCorner.y, maxCorner.y, V3(-1, 0, 0)},
                                            {minCorner.y, rel.y, rel.x, entityDelta.y, entityDelta.x, minCorner.x, maxCorner.x, V3(0, -1, 0)},
                                            {maxCorner.y, rel.y, rel.x, entityDelta.y, entityDelta.x, minCorner.x, maxCorner.x, V3(0, 1, 0)}                    
                                        };

                                        if(IsSet(testEntity, EntityFlag_Traversable))
                                        {
                                            // NOTE : We are only checking this 
                                            // against the one that the entity is overlapping!

                                            v3 testWallNormal = {};
                                            real32 tMaxTest = tMax;                                    
                                            bool32 hitThis = false;
                                    
                                            for(uint32 wallIndex = 0;
                                                wallIndex < ArrayCount(walls);
                                                ++wallIndex)
                                            {
                                                test_wall *wall = walls + wallIndex;

                                                real32 tEpsilon = 0.001f;
                                                if(wall->deltaX != 0.0f)
                                                {
                                                    //Equation : p0x(formal player position x) + t * dx(player delta x) = wx(Wall x)
                                                    real32 tResult = (wall->x - wall->relX) / wall->deltaX;
                                                    // y value of the new player position 
                                                    real32 y = wall->relY + tResult * wall->deltaY;
                                            
                                                    // Check whether the tMin is greater than tResult
                                                    // Which means, the value we computed is the closer collision point
                                                    // We also need to check the tResult because if it's less than 0, 
                                                    // it means the player have to go backward to hit the wall - which we don't care.
                                                    if(tResult >= 0.0f && tMaxTest < tResult)
                                                    {
                                                        // Check whether the y is inside the bound(minY ~ maxY)
                                                        // Which means, the player is going to collide with this certain wall
                                                        if(y >= wall->minY && y <= wall->maxY)
                                                        {
                                                            tMaxTest = Maximum(0.0f, tResult - tEpsilon);
                                                            testWallNormal = wall->normal;
                                                            hitThis = true;
                                                        }
                                                    }
                                                }
                                            }

                                            if(hitThis)
                                            {
                                                tMax = tMaxTest;
                                                wallNormalMax = testWallNormal;
                                                hitEntityMax = testEntity;
                                            }
                                        }
                                        else
                                        {
                                            v3 testWallNormal = {};
                                            real32 tMinTest = tMin;                                    
                                            bool32 hitThis = false;

                                            for(uint32 wallIndex = 0;
                                                wallIndex < ArrayCount(walls);
                                                wallIndex++)
                                            {
                                                test_wall *wall = walls + wallIndex;

                                                real32 tEpsilon = 0.001f;
                                                if(wall->deltaX != 0.0f)
                                                {
                                                    //Equation : p0x(formal player position x) + t * dx(player delta x) = wx(Wall x)
                                                    real32 tResult = (wall->x - wall->relX) / wall->deltaX;
                                                    // y value of the new player position 
                                                    real32 y = wall->relY + tResult * wall->deltaY;
                                            
                                                    // Check whether the tMin is greater than tResult
                                                    // Which means, the value we computed is the closer collision point
                                                    // We also need to check the tResult because if it's less than 0, 
                                                    // it means the player have to go backward to hit the wall - which we don't care.
                                                    if(tResult >= 0 && tMinTest > tResult)
                                                    {
                                                        // Check whether the y is inside the bound(minY ~ maxY)
                                                        // Which means, the player is going to collide with this certain wall
                                                        if(y >= wall->minY && y <= wall->maxY)
                                                        {
                                                            // Always clamp to 0
                                                            // because if the tresult is too small, it can be negative value
                                                            tMinTest = Maximum(0.0f, tResult - tEpsilon);
                                                            testWallNormal = wall->normal;
                                                            hitThis = true;
                                                        }
                                                    }
                                                }
                                            }
                                            // TODO : We need a concept of stepping onto vs. stepping
                                            // off of here so that we can prevent you from _leaving_
                                            // stairs instead of just preventing you from getting onto them.
                                            if(hitThis)
                                            {
                                                v3 testP = entity->pos + tMinTest*entityDelta;
                                                if(SpeculativeCollide(entity, testEntity, testP))
                                                {
                                                    tMin = tMinTest;
                                                    wallNormalMin = testWallNormal;
                                                    hitEntityMin = testEntity;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            real32 tStop = 1.0f;
            sim_entity *hitEntity;
            v3 wallNormal;
            if(tMin < tMax)
            {
                hitEntity = hitEntityMin;
                wallNormal = wallNormalMin;

                // only change the tSTop if it hits the solid entity
                if(hitEntity)
                {
                    bool32 stopsOnCollision = HandleCollision(gameState, entity, hitEntity);
                    if(stopsOnCollision)
                    {
                        tStop = tMin;
                    }
                }
            }
            else
            {
                // NOTE : The entity is about to move the room
                // tStop = tMax;
				// TODO : For now, this is hacked in because the entities will struggle to move room, but there is
				// no actually a room asdie each room. Therefore, I will just not allow the entities from 'trying'
				// to move the room
                tStop = -0.5f;
                wallNormal = wallNormalMax;
                hitEntity = hitEntityMax;
            }

            // WE change the actual position of the entity here
            entity->pos += tStop * entityDelta;
            distanceRemaining -= tStop * entityDeltaLength;
            if(hitEntity)
            {   
                entityDelta = desiredPosition - entity->pos;
                bool32 stopsOnCollision = HandleCollision(gameState, entity, hitEntity);
                if(stopsOnCollision)
                {
                    // Recalculate the delta as much as it moved
                    entityDelta = entityDelta - 1*Inner(entityDelta, wallNormal)*wallNormal;

                    // NOTE : If this is the case, the entity is not supposed to go in or out
                    // because it stops on collision. Clues in the name XD
                    entity->dPos = entity->dPos - 1*Inner(entity->dPos, wallNormal)*wallNormal;
                }
            }
            else
            {
                // NOTE : We didn't found any colliding entity 
                break;
            }

        }
        else
        {
            // NOTE : We don't have any delta length left to move the entity!
            break;
        }
    }

#if 0
    // TODO : This has to become real height handling / ground collision / etc!
    if((entity->pos.y <= ground) ||
    (IsSet(entity, EntityFlag_YSupported) &&
     (entity->dPos.y == 0.0f)))
    {
        entity->pos.y = ground;
        entity->dPos.y = 0;
        AddFlags(entity, EntityFlag_ZSupported);
    }
    else
    {
        ClearFlags(entity, EntityFlag_ZSupported);
    }
#endif
    
    // This is equal because distanceRemaining is the full distance,
    // not the distance just for this frame!
    // TODO : Find out why this is necessarily?
    // NOTE : This is needed because if I do not have this and
    // if the distance remaining is too small, 
    // it will actually update the entity->distancelimit to be too small
    // making it to look like it's stuck.
    if(distanceRemaining != 0.0f)
    {
        entity->distanceLimit = distanceRemaining;
    }

#if 0
    if((entity->dPos.x == 0.0f) && (entity->dPos.y == 0.0f))
    {
        // NOTE : Leave facingDirection whatever it was
    }
    else
    {
        entity->facingDirection = ATan2(entity->dPos.y, entity->dPos.x);
    }
#endif
}
