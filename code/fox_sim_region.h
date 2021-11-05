#ifndef FOX_SIM_REGION_H

#define InvalidPos V3(Real32Max, Real32Max, Real32Max)

// Movement Spec
// Drag, acceleration..
struct move_spec
{
    bool32 unitMaxAccelVector;
    real32 speed;
    v3 drag;
    // NOTE : For accleration
    r32 mass;
};

enum entity_type
{
    EntityType_Null,

    EntityType_Space,

    EntityType_Player,
    EntityType_Wall,
    EntityType_Familiar,

    // NOTE : Monsters
    EntityType_Sponge,
    EntityType_Monster,
    EntityType_BigFollowingEnemy,
    EntityType_PatrolEnemy,
    EntityType_LaserShootingEnemy,

    // NOTE : Bullet entities
    EntityType_Bullet,    
    EntityType_PlayerBullet,
    EntityType_EnemyBullet,

    // NOTE : Visual entities
    EntityType_Cloud,
    EntityType_Mountain,
    EntityType_Sun,
    EntityType_Tutorial,

    EntityType_Stairwell,    
    EntityType_LaserTrap,
    EntityType_Moon,

};

#define HIT_POINT_SUB_COUNT 4
struct hit_point
{
    // TODO : Bake this down into one variable!
    uint8 flags;
    uint8 filledAmount;
};

struct sim_entity;
union entity_reference
{
    uint32 index;
    sim_entity *ptr;  
};

struct sim_entity_collision_volume
{
    // NOTE : 
    v3 offset;
    v3 dim;
};

struct sim_entity_collision_volume_group
{
    // This is the giant rectangle that contains every volumes
    // because usually we don't think about the volumes seperately
    // when we are trying to add to sim region
    sim_entity_collision_volume totalVolume;

    uint32 volumeCount;
    sim_entity_collision_volume *volumes;
};

enum bullet_type
{
    BulletType_Single,
    BulletType_Twin,
    BulletType_Quad,
    BulletType_Laser,
};

struct bullet_spec
{
    r32 speed;
    // NOTE : This direction is counterclockwise
    r32 directionRad;
};

struct bullet_spec_group
{
    bullet_spec *specs;
    u32 specCount;
    bullet_type type;
    
    // TODO : Put this inside the bullet_spec?
    r32 recoilAmount;
    r32 attackCooldown;
    r32 distanceLimit;
};

enum laser_type
{
    LaserType_Single,
    LaserType_Triple,
    LaserType_Backward,
    LaserType_Quad,
};

struct laser_spec
{
    // NOTE : This direction is counterclockwise
    r32 directionRad;
};

struct laser_spec_group
{
    r32 dim;
    laser_spec *specs;
    u32 specCount;
    laser_type type;

    sim_entity *shooter;

    r32 recoilRad;

    // TODO : Put this inside the bullet_spec?
    r32 recoilAmount;
    r32 fuelConsume;
};

struct sim_entity_animation
{
    v2 offset;

    r32 currentFrameTime;
    r32 timePerFrame;
    u32 currentFrame;
    // TODO : How can I get rid of this?
    u32 totalFrame;
    
    // TODO : Might hold Asset type too?
    enum asset_type_id ID;
};

struct sim_entity 
{
    uint32 storageIndex; // Low entity index
    bool32 updatable;

    // These are now the ground point, NOT the center of the entity! 
    v3 pos;
    v3 dPos;

    move_spec moveSpec;

    entity_type type;
    uint32 flags;
    
    sim_entity_collision_volume_group *collision;

    i32 hitPointMax;

    // distance limit of the sword
    real32 distanceLimit;
    real32 facingRad;
    real32 mass;
    r32 attackCooldown;

    // NOTE : For drawing thrust particle
    b32 isAnyMoveKeyPressed;

    particle_emitter emitter;
    particle_emitter emitter2;
    particle_emitter emitter3;

    // NOTE : Enemy
    r32 awareCooldown;
    v3 awaredPos;

    r32 alpha;
    r32 size;
    b32 shouldThereBeRecoil;
    v3 recoil;

    r32 invincibleTime;
    r32 maxInvincibleTime;

    // NOTE : DEPRECATED.
#if 0
    // TODO : Put this on different spot
    // instead of sim_entity
    sim_entity_animation *animation;
    u32 animationCount;
    real32 walkableHeight;
    v2 walkableDim;
    moon moon;
#endif
};

struct sim_entity_hash
{
    sim_entity *ptr;
    uint32 index;
};

// 
struct sim_region
{
    world *world;

    real32 maxEntityRadius;
    real32 maxEntityVelocity;

    // center of this sim_region
    world_position origin;
    // bounds of this sim_region
    rect3 bounds;
    rect3 updatableBounds;

    uint32 maxEntityCount;
    uint32 entityCount;
    sim_entity *entities;
    
    // sim_entity *emitters[32];

    // This is to get the sim entity based on the storage index
    // because now, the storgae entity does not know about sim entity(it does have the sim entity, but it's only for 
    // the storage purpose. It does not know about the sim entity inside the sim region)
    // So if someone want to get the sim entity with storageindex,
    // you have to come to hash using the storageIndex, get the hash,
    // and then get the pointer to the sim entity
    // NOTE : IMPORTANT : This should be the same number as the max entity number!
    sim_entity_hash hash[4096];
};


#define FOX_SIM_REGION_H
#endif