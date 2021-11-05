internal b32 
RaycastWithAngle2D(v2 rayStartPos, float angle, v2 segmentStart, v2 segmentEnd)
{
    b32 result = false;

    real32 rayDx = Cos(angle);
    Assert(rayDx != 0.0f);
    real32 rayDy = Sin(angle);
    real32 segmentDx = segmentEnd.x - segmentStart.x;
    real32 segmentDy = segmentEnd.y - segmentStart.y;

    // NOTE : Checking parellel
    if (rayDy * segmentDx == rayDx * segmentDy)
    {
        return result;
    }

    real32 t2 = (rayDx * (segmentStart.y - rayStartPos.y) + rayDy * (rayStartPos.x - segmentStart.x)) /
                (segmentDx * rayDy - segmentDy * rayDx);

    // Be sure that rDx isn't 0!!
    real32 t1 = (segmentStart.x + segmentDx*t2 - rayStartPos.x) / rayDx;

    if (t1 >= 0.0f && t2 > 0.0f && t2 < 1.0f)
    {
        result = true;
    }

    return result;
}

struct test_segment
{
    v2 start;
    v2 end;
};

internal b32 
RaycastToEntityVolume2D(v2 startPos, float angle, sim_entity *testEntity)
{
    b32 result = false;

    for(uint32 volumeIndex = 0;
        volumeIndex < testEntity->collision->volumeCount;
        volumeIndex++)
    {
        sim_entity_collision_volume *volume = testEntity->collision->volumes + volumeIndex;

        r32 halfDimX = 0.5f*volume->dim.x;
        r32 halfDimY = 0.5f*volume->dim.y;
        v2 pos = testEntity->pos.xy + volume->offset.xy;

        v2 topRightCorner = pos + V2(halfDimX, halfDimY);
        v2 downRightCorner = pos + V2(halfDimX, -halfDimY);
        v2 topLeftCorner = pos + V2(-halfDimX, halfDimY);
        v2 downLeftCorner = pos + V2(-halfDimX, -halfDimY);

        test_segment segments[] = 
        {
            {topRightCorner, downRightCorner},
            {downRightCorner, downLeftCorner},
            {downLeftCorner, topLeftCorner},
            {topLeftCorner, topRightCorner}
        };

        for(i32 segmentIndex = 0;
            segmentIndex < ArrayCount(segments);
            ++segmentIndex)
        {
            test_segment *segment = segments + segmentIndex;
            if(RaycastWithAngle2D(startPos, angle, segment->start, segment->end))
            {
                result = true;
                break;
            }
        }
    }

    return result;
}

struct raycast2d_result
{
    sim_entity *hitEntity;
    v2 hitPos;
};

internal raycast2d_result
RaycastToObjects2D(v2 startPos, r32 angle, sim_entity *shooter, r32 laserDim, sim_region *simRegion)
{
    r32 laserHalfDim = 0.5f*laserDim;
    r32 laserQuarterDim = 0.25f*laserDim;
    i32 laserCount = (i32)(laserDim/0.5f);
    r32 laserSpacing = laserDim/(r32)laserCount;

    r32 tMin = 100000.0f;
    sim_entity *hitEntity = 0;
    raycast2d_result result = {};

    test_segment hitSegment = {};
    r32 t2Min = 0.0f;

    // TODO : Spatial partition here!
    for(uint32 testEntityIndex = 0;
        testEntityIndex < simRegion->entityCount;
        ++testEntityIndex)
    {
        sim_entity *testEntity = simRegion->entities + testEntityIndex;

        if(shooter != testEntity)
        {
            if((IsSet(testEntity, EntityFlag_CanCollide) && testEntity->invincibleTime <= 0.0f) ||
                testEntity->type == EntityType_Space) 
            {
                for(uint32 volumeIndex = 0;
                    volumeIndex < testEntity->collision->volumeCount;
                    volumeIndex++)
                {
                    sim_entity_collision_volume *volume = testEntity->collision->volumes + volumeIndex;

                    r32 halfDimX = 0.5f*volume->dim.x;
                    r32 halfDimY = 0.5f*volume->dim.y;
                    v2 pos = testEntity->pos.xy + volume->offset.xy;

                    v2 topRightCorner = pos + V2(halfDimX, halfDimY);
                    v2 downRightCorner = pos + V2(halfDimX, -halfDimY);
                    v2 topLeftCorner = pos + V2(-halfDimX, halfDimY);
                    v2 downLeftCorner = pos + V2(-halfDimX, -halfDimY);

                    test_segment segments[] = 
                    {
                        {topRightCorner, downRightCorner},
                        {downRightCorner, downLeftCorner},
                        {downLeftCorner, topLeftCorner},
                        {topLeftCorner, topRightCorner}
                    };

                    for(i32 segmentIndex = 0;
                        segmentIndex < ArrayCount(segments);
                        ++segmentIndex)
                    {
                        test_segment *segment = segments + segmentIndex;

                        real32 rayDx = Cos(angle);
                        // Assert(rayDx != 0.0f);
                        real32 rayDy = Sin(angle);
                        real32 segmentDx = segment->end.x - segment->start.x;
                        real32 segmentDy = segment->end.y - segment->start.y;

                        for(i32 laserCountIndex = 0;
                            laserCountIndex < laserCount;
                            ++laserCountIndex)
                        {
                            v2 rayStartPos = startPos + Rotate(V2(0.0f, -laserHalfDim + laserCountIndex*laserSpacing), angle);
                            // NOTE : Checking parellel
                            if (rayDy * segmentDx != rayDx * segmentDy)
                            {
                                real32 t2 = (rayDx * (segment->start.y - rayStartPos.y) + rayDy * (rayStartPos.x - segment->start.x)) /
                                            (segmentDx * rayDy - segmentDy * rayDx);

                                // Be sure that rDx isn't 0!!
                                real32 t1 = (segment->start.x + segmentDx*t2 - rayStartPos.x) / rayDx;

                                if (t1 >= 0.0f && (t2 > 0.0f && t2 < 1.0f))
                                {
                                    if(tMin > t1)
                                    {

                                        tMin = t1;
                                        result.hitEntity = testEntity;
                                        // TODO : Get the hit pos!
                                        t2Min = t2;
                                        hitSegment = *segment;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if(result.hitEntity)
    {
        result.hitPos = Lerp(hitSegment.start, t2Min, hitSegment.end);
    }

    return result;
}
