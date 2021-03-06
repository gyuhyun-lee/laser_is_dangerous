
#ifndef FOX_ENTITY_H
#define FOX_ENTITY_H

enum sim_entity_flags
{
    EntityFlag_Nonspatial = (1 << 0),
    EntityFlag_Movable = (1 << 1),
    EntityFlag_CanCollide = (1 << 2),
    // Whether the entity is effected by the gravity(y axis)
    EntityFlag_YSupported = (1 << 3),
    // NOTE : Whether the another can go through this entity _ex_ space
    EntityFlag_Traversable = (1 << 4),    
};

// using uint32 here because if we pass multiple flags like
// collides|nonspatial, it will not work if we use sim_entity_flags
inline bool32
IsSet(sim_entity *entity, uint32 flag)
{
    bool32 result = entity->flags & flag;
    return result;
}

inline b32
IsEntityNonSpatial(sim_entity *entity)
{
    return IsSet(entity, EntityFlag_Nonspatial);
}

// This is addflag, not setflag because it can ALSO add flag, too by |(or) it.
inline void
AddFlags(sim_entity *entity, uint32 flag)
{
    entity->flags |= flag;
}

inline void
ClearFlags(sim_entity *entity, uint32 flag)
{
    entity->flags &= ~flag;
}

inline void
MakeEntityNonSpatial(sim_entity *entity)
{
    AddFlags(entity, EntityFlag_Nonspatial);
    entity->pos = InvalidPos;
}

inline void
MakeEntitySpatial(sim_entity *entity, v3 pos, v3 dPos)
{
    ClearFlags(entity, EntityFlag_Nonspatial);
    entity->pos = pos;

    entity->dPos = dPos;
}

inline v3
GetEntityGroundPoint(sim_entity *entity)
{
    v3 result = entity->pos;

    return(result);
}

#if 0
inline real32 
GetStairGround(sim_entity *region, v3 atGroundPoint)
{
    Assert(region->type == EntityType_Stairwell);

    rect2 regionRect = RectCenterDim(region->pos.xy, region->walkableDim);
    
    // barycentralized coordinates
    // We need clamped one because even if we overlapp, the mover->pos can NOT be 
    // inside the regionrect, making some coords negative.
    // We need to clamp those to minimum value of regionRect
    v2 bary = Clamp01(GetBarycentric(regionRect, atGroundPoint.xy));
    
    real32 resultGround = region->pos.z + bary.y * region->walkableHeight;

    return resultGround;
}
#endif

#endif