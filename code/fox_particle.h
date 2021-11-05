#ifndef FOX_PARTICLE_H
#define FOX_PARTICLE_H

struct particle 
{
    // TODO : Some of them might be better inside the particle emitter
    // NOTE : This is the _OFFSET_ from the emitter, not the position in the sim region itself!
    v3 offset;
    v3 dPos;
    r32 lifeTime;
    r32 maxLifeTime;
    r32 facingRad;
    r32 dFacingRad;
    v4 color;
};

enum particle_type
{
    ParticleType_Custom,
    ParticleType_Thrust,
    ParticleType_EnemyThrust,
    ParticleType_BoostedThrust,
    ParticleType_Smoke,
    ParticleType_Circle,
    ParticleType_LaserSurrounding,
    ParticleType_Shotgun,
    ParticleType_LaserHit,
    ParticleType_Laser,
};

struct particle_emitter
{
    v3 pos;
    // NOTE : Particle
    // TODO : Particle Emitter & type?
    particle particles[256];
    r32 facingRad;
};

#endif