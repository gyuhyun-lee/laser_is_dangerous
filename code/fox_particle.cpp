inline void
RandomParticle(particle *particle, particle_emitter *emitter, random_series *series)
{
    particle->maxLifeTime = RandomBetween(series, 0.1f, 1.2f);
    particle->lifeTime = particle->maxLifeTime;
    particle->offset = V3(RandomBetween(series, -0.3f, 0.3f), 
                                    RandomBetween(series, 0.0f, 0.0f), 0);
    particle->dPos = RandomBetween(series, 0.0f, 1.0f)*V3(Cos(emitter->facingRad - Pi32), 
                                                        Sin(emitter->facingRad - Pi32), 0);
    particle->facingRad = emitter->facingRad;
    particle->dFacingRad = 0.0f;
    particle->color = V4(RandomBetween(series, 0.0f, 1.0f),
                        RandomBetween(series, 0.0f, 1.0f),
                        RandomBetween(series, 0.0f, 1.0f),
                        1.0f);
}

inline void
ThrustParticle(particle *particle, particle_emitter *emitter, random_series *series)
{
    particle->maxLifeTime = RandomBetween(series, 0.1f, 0.2f);
    particle->lifeTime = particle->maxLifeTime;

    particle->offset = Rotate(V3(RandomBetween(series, 0.0f, 0.0f), 
                                            RandomBetween(series, -0.2f, 0.2f), 0), emitter->facingRad);

    particle->dPos = RandomBetween(series, 15.0f, 30.0f)*V3(Cos(emitter->facingRad - Pi32), 
                                                        Sin(emitter->facingRad - Pi32), 0);

    particle->facingRad = emitter->facingRad;
    particle->dFacingRad = 0.0f;
    // particle->color = V4(0.7f,
    //                     RandomBetween(series, 0.0f, 0.51f),
    //                     0.0f,
    //                     1.0f);
    particle->color = V4(RandomBetween(series, 0.0f, 0.4f),
                        RandomBetween(series, 0.2f, 1.0f),
                        RandomBetween(series, 0.7f, 0.9f),
                        1.0f);
}

inline void
EnemyThrustParticle(particle *particle, particle_emitter *emitter, random_series *series)
{
    particle->maxLifeTime = RandomBetween(series, 0.1f, 0.5f);
    particle->lifeTime = particle->maxLifeTime;

    particle->offset = Rotate(V3(RandomBetween(series, 0.0f, 0.0f), 
                                            RandomBetween(series, -0.2f, 0.2f), 0), emitter->facingRad);

    particle->dPos = RandomBetween(series, 15.0f, 30.0f)*V3(Cos(emitter->facingRad - Pi32), 
                                                        Sin(emitter->facingRad - Pi32), 0);

    particle->facingRad = emitter->facingRad;
    particle->dFacingRad = 0.0f;
    particle->color = V4(0.7f,
                        RandomBetween(series, 0.0f, 0.51f),
                        0.0f,
                        1.0f);
}

inline void
BoostedThrustParticle(particle *particle, particle_emitter *emitter, random_series *series)
{
    particle->maxLifeTime = RandomBetween(series, 0.1f, 0.4f);
    particle->lifeTime = particle->maxLifeTime;
    particle->offset = Rotate(V3(RandomBetween(series, 0.0f, 0.0f), 
                                            RandomBetween(series, -0.5f, 0.5f), 0), emitter->facingRad);
    particle->dPos = RandomBetween(series, 30.0f, 50.0f)*V3(Cos(emitter->facingRad - Pi32), 
                                                        Sin(emitter->facingRad - Pi32), 0);
    particle->facingRad = emitter->facingRad;
    particle->dFacingRad = 0.0f;
    particle->color = V4(RandomBetween(series, 0.0f, 1.0f),
                        RandomBetween(series, 0.0f, 1.0f),
                        RandomBetween(series, 0.0f, 1.0f),
                        1.0f);
}

inline void
SmokeParticle(particle *particle, particle_emitter *emitter, random_series *series)
{
    particle->maxLifeTime = RandomBetween(series, 0.1f, 4.0f);
    particle->lifeTime = particle->maxLifeTime;

    // particle->offset = V3(RandomBetween(series, -0.5f, 0.5f), 
    //                                 RandomBetween(series, 0.0f, 0.0f), 0);
    particle->offset = V3(0, 0, 0);

    particle->dPos = RandomBetween(series, 10.0f, 40.0f)*V3(RandomBetween(series, -1.0f, 1.f), 
                                                        RandomBetween(series, -1.0f, 1.0f), 
                                                        RandomBetween(series, -2.5f, 1.5f));

    particle->facingRad = emitter->facingRad;
    particle->dFacingRad = 0.0f;
    particle->color = V4(RandomBetween(series, 0.0f, 1.0f),
                        RandomBetween(series, 0.0f, 0.2f),
                        RandomBetween(series, 0.0f, 0.2f),
                        1.0f);
}

inline void
CircleParticle(particle *particle, particle_emitter *emitter, random_series *series)
{
    particle->maxLifeTime = RandomBetween(series, 0.0f, 0.1f);
    particle->lifeTime = particle->maxLifeTime;

    particle->offset = Rotate(V3(RandomBetween(series, 0.0f, 0.0f), 
                                            RandomBetween(series, -0.5f, 0.5f), 0), emitter->facingRad);
    
    // particle->dPos = V3(0, RandomBetween(series, 0.0f, 5.0f), 0);
    particle->dPos = RandomBetween(series, 0.0f, 5.0f)*V3(Cos(emitter->facingRad), 
                                                        Sin(emitter->facingRad), 0);
    particle->facingRad = emitter->facingRad;
    particle->dFacingRad = 2.0f;
    particle->color = V4(RandomBetween(series, 0.0f, 1.0f),
                        RandomBetween(series, 0.0f, 1.0f),
                        RandomBetween(series, 0.0f, 1.0f),
                        1.0f);
}

inline void
LaserSurroundingParticle(particle *particle, particle_emitter *emitter, random_series *series)
{
    particle->maxLifeTime = RandomBetween(series, 0.0f, 0.07f);
    particle->lifeTime = particle->maxLifeTime;

    particle->offset = Rotate(V3(RandomBetween(series, 0.0f, 0.0f), 
                                            RandomBetween(series, -0.5f, 0.5f), 0), emitter->facingRad);
    
    // particle->dPos = V3(0, RandomBetween(series, 0.0f, 5.0f), 0);
    particle->facingRad = emitter->facingRad + RandomBetween(series, -0.4f*Pi32, 0.4f*Pi32);
    particle->dPos = RandomBetween(series, 65.0f, 90.0f)*V3(Cos(particle->facingRad), 
                                                        Sin(particle->facingRad), 0);
    particle->dFacingRad = 0.0f;
    particle->color = V4(RandomBetween(series, 0.0f, 1.0f),
                        RandomBetween(series, 0.0f, 1.0f),
                        RandomBetween(series, 0.0f, 1.0f),
                        1.0f);   
}

inline void
ShotgunParticle(particle *particle, particle_emitter *emitter, random_series *series)
{
    particle->maxLifeTime = RandomBetween(series, 0.0f, 0.02f);
    particle->lifeTime = particle->maxLifeTime;

    particle->offset = Rotate(V3(RandomBetween(series, 0.0f, 0.0f), 
                                            RandomBetween(series, -0.5f, 0.5f), 0), emitter->facingRad);

    r32 dir = 0.0f;
    switch(NextRandomUInt32(series)%5)
    {
        case 0:
        {
            dir = -0.1f*Pi32;
        }break;
        case 1:
        {
            dir = -0.05f*Pi32;
        }break;
        case 2:
        {
            dir = 0.0f;
        }break;
        case 3:
        {
            dir = 0.05f*Pi32;
        }break;
        case 4:
        {
            dir = 0.1f*Pi32;
        }break;
    }

    particle->facingRad = emitter->facingRad + dir;

    particle->dPos = RandomBetween(series, 50.0f, 500.0f)*V3(Cos(particle->facingRad), 
                                                        Sin(particle->facingRad), 0);
    particle->dFacingRad = 0.0f;
    particle->color = V4(RandomBetween(series, 0.0f, 1.0f),
                        RandomBetween(series, 0.0f, 1.0f),
                        RandomBetween(series, 0.0f, 1.0f),
                        1.0f);   
}

inline void
LaserHitParticle(particle *particle, particle_emitter *emitter, random_series *series)
{
    particle->maxLifeTime = RandomBetween(series, 0.0f, 0.07f);
    particle->lifeTime = particle->maxLifeTime;

    particle->offset = Rotate(V3(RandomBetween(series, 0.0f, 0.0f), 
                                            RandomBetween(series, -0.5f, 0.5f), 0), emitter->facingRad);
    
    // particle->dPos = V3(0, RandomBetween(series, 0.0f, 5.0f), 0);
    particle->facingRad = emitter->facingRad + RandomBetween(series, -0.52f*Pi32, 0.52f*Pi32);
    particle->dPos = RandomBetween(series, 40.0f, 60.0f)*V3(Cos(particle->facingRad), 
                                                        Sin(particle->facingRad), 0);
    particle->dFacingRad = 0.0f;
    particle->color = V4(RandomBetween(series, 0.0f, 1.0f),
                        RandomBetween(series, 0.0f, 1.0f),
                        RandomBetween(series, 0.0f, 1.0f),
                        1.0f);   
}

inline void
LaserParticle(particle *particle, particle_emitter *emitter, random_series *series)
{
    particle->maxLifeTime = RandomBetween(series, 0.1f, 1.0f);
    particle->lifeTime = particle->maxLifeTime;
    particle->offset = Rotate(V3(RandomBetween(series, 0.0f, 0.0f), 
                                            RandomBetween(series, -0.5f, 0.5f), 0), emitter->facingRad);
    particle->dPos = RandomBetween(series, 30.0f, 50.0f)*V3(Cos(emitter->facingRad), 
                                                        Sin(emitter->facingRad), 0);
    particle->facingRad = emitter->facingRad;
    particle->dFacingRad = 0.0f;
    particle->color = V4(RandomBetween(series, 0.0f, 1.0f),
                        RandomBetween(series, 0.0f, 1.0f),
                        RandomBetween(series, 0.0f, 1.0f),
                        1.0f);
}

internal void
ProcessParticles(particle_emitter *emitter, r32 emitterAlpha, asset_type_id assetType, particle_type type, r32 size,
                v3 pos, v3 offset, r32 facingRad, render_group *renderGroup, 
                random_series *series, r32 dt, b32 shouldRenewParticles)
{
    // NOTE : Update the particles
    // if(shouldRenewParticles)
    {
        emitter->pos = pos + offset;
        emitter->facingRad = facingRad;
    }

    for(i32 particleIndex = 0;
        particleIndex < ArrayCount(emitter->particles);
        ++particleIndex)
    {
        particle *particle = emitter->particles + particleIndex;

        particle->offset += particle->dPos*dt;

        particle->color.a = emitterAlpha * (particle->lifeTime / particle->maxLifeTime);
        // if(particle->color.a > 0.97f)
        // {
        //     particle->color.a = 0.97f*Clamp01MapInRange(1.0f, particle->color.a, 0.97f);
        // }

        // NOTE : Particle clamping
        if(particle->color.a < 0.05f)
        {
            particle->color.a = 0.05f*Clamp01MapInRange(0.05f, particle->color.a, 1.0f);
        }
#if 0
        PushBitmap(renderGroup, GetFirstBitmapFrom(tranState->assets, Asset_Type_Square), 
                    5.0f, V3(0, 0, 0), Cos(particle->facingRad), Sin(particle->facingRad), particle->color);
#else
        renderGroup->transform.entitySimPos = emitter->pos;
        // NOTE : Cool looking circle type particle
        PushBitmap(renderGroup, GetFirstBitmapFrom(renderGroup->assets, assetType), 
                    size, particle->offset, Cos(particle->facingRad), Sin(particle->facingRad), V4(particle->color.r, particle->color.g, particle->color.a, particle->color.a), true);
#endif
        if(shouldRenewParticles)
        {
            particle->lifeTime -= dt;
        }
        else
        {
            particle->lifeTime -= 0.25f*dt;
        }
        particle->facingRad += particle->dFacingRad * dt;

        if(particle->lifeTime < 0.0f && shouldRenewParticles)
        {
            switch(type)
            {
                case ParticleType_Custom:
                {
                    // Renew the particle 
                    RandomParticle(particle, emitter, series);
                }break;

                case ParticleType_Thrust:
                {
                    ThrustParticle(particle, emitter, series);
                }break;

                case ParticleType_EnemyThrust:
                {
                    EnemyThrustParticle(particle, emitter, series);
                }break;


                case ParticleType_BoostedThrust:
                {
                    BoostedThrustParticle(particle, emitter, series);
                }break;

                case ParticleType_Smoke:
                {
                    SmokeParticle(particle, emitter, series);
                }break;

                case ParticleType_Circle:
                {
                    CircleParticle(particle, emitter, series);
                }break;

                case ParticleType_LaserSurrounding:
                {
                    LaserSurroundingParticle(particle, emitter, series);
                }break;

                case ParticleType_Shotgun:
                {
                    ShotgunParticle(particle, emitter, series);
                }break;

                case ParticleType_LaserHit:
                {
                    LaserHitParticle(particle, emitter, series);
                }break;

                case ParticleType_Laser:
                {
                    LaserParticle(particle, emitter, series);
                }break;
            }
        }
    }

    renderGroup->transform.entitySimPos = pos;
}
