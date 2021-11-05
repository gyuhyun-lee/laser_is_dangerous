#pragma pack(push, 1)
struct bitmap_header
{
    uint16 fileType;
    uint32 fileSize;
    uint16 reserved1;
    uint16 reserved2;
    uint32 bitmapOffset;
    uint32 size;
    int32 width;
    int32 height;
    uint16 planes;
    uint16 bitsPerPixel;
    uint32 compression;
    uint32 sizeOfBitmap;
    int32 horzResolution;
    int32 vertResolution;
    uint32 colorsUsed;
    uint32 colorsImportant;

    uint32 redMask;
    uint32 greenMask;
    uint32 blueMask;
};

#define ProduceWaveID(a, b, c, d) (((uint32)(a) << 0) | ((uint32)(b) << 8) | ((uint32)(c) << 16) | ((uint32)(d) << 24)) 
enum
{
    WAVE_ChunkID_fmt = ProduceWaveID('f', 'm', 't', ' '),
    WAVE_ChunkID_RIFF = ProduceWaveID('R', 'I', 'F', 'F'),
    WAVE_ChunkID_data = ProduceWaveID('d', 'a', 't', 'a'),
    WAVE_ChunkID_WAVE = ProduceWaveID('W', 'A', 'V', 'E'),
};

struct WAVE_header
{
    uint32 riffID;
    uint32 size;
    uint32 waveID;
};

struct WAVE_fmt
{
    uint16 wFormatTag;
    uint16 nChannels;
    uint32 nSamplesPerSec;  
    uint32 nAvgBytesPerSec;
    uint16 nBlockAlign;
    uint16 wBitsPerSample;
    uint16 cbSize;
    uint16 wValidBitsPerSample;
    uint32 dwChannelMask;
    uint8 subFormat[16];
};

struct WAVE_chunk
{
    uint32 ID;
    uint32 size;
};

#pragma pack(pop)
// End the exact fitting and back to the regular packing

inline v2 
SetTopDownAlign(loaded_bitmap *bitmap, v2 align)
{
    align.y = bitmap->height - align.y;
    align.x = SafeRatio0((real32)align.x, (real32)bitmap->width);
    align.y = SafeRatio0((real32)align.y, (real32)bitmap->height);

    return align;
}

// NOTE : Align is based on left bottom corner as Y is up -> which means top-down
internal loaded_bitmap
DEBUGLoadBMP(char *fileName, v2 alignPercentage = V2(0.5f, 0.5f))
{
    loaded_bitmap result = {};

    debug_read_file_result readResult = DEBUGPlatformReadEntireFile(fileName);

    if(readResult.contentSize != 0)
    {
        bitmap_header *bitmapHeader = (bitmap_header *)readResult.content;
        uint32 *pixels = (uint32 *)((uint8 *)readResult.content + bitmapHeader->bitmapOffset);

        result.memory = pixels;

        result.width = bitmapHeader->width;
        result.height = bitmapHeader->height;
        result.widthOverHeight = (real32)result.width / (real32)result.height;

        result.alignPercentage = alignPercentage;

        // NOTE : This function is only for bottom up bmps for now.
        Assert(result.height >= 0);
        // TODO : Do this for each compression type?
        Assert(bitmapHeader->compression == 3);

        // If you are using this generically,
        // the height will be negative for top-down
        // please remember that BMP files can go in either direction and
        // (there can be compression, etc... don't think this 
        // is complete BMP loading function)

        // NOTE : Byte Order in memory is determined by the header itself,
        // so we have to read out the masks and convert our piexels ourselves.
        uint32 redMask = bitmapHeader->redMask;
        uint32 greenMask = bitmapHeader->greenMask;
        uint32 blueMask = bitmapHeader->blueMask;
        uint32 alphaMask = ~(redMask | greenMask | blueMask);

        // Find out how many bits we must rightshift for each colormask
        bit_scan_result redScan = FindLeastSignificantSetBit(redMask);
        bit_scan_result greenScan = FindLeastSignificantSetBit(greenMask);
        bit_scan_result blueScan = FindLeastSignificantSetBit(blueMask);
        bit_scan_result alphaScan = FindLeastSignificantSetBit(alphaMask);

        Assert(redScan.found && greenScan.found && blueScan.found && alphaScan.found);

        int32 alphaShift = (int32)alphaScan.index;
        int32 redShift = (int32)redScan.index;
        int32 greenShift = (int32)greenScan.index;
        int32 blueShift = (int32)blueScan.index;
        
        uint32 *sourceDest = pixels;

        for(int32 y = 0;
            y < bitmapHeader->height;
            ++y)
        {
            for(int32 x = 0;
                x < bitmapHeader->width;
                ++x)
            {
                uint32 color = *sourceDest;

                v4 texel = {(real32)((color & redMask) >> redShift),
                    (real32)((color & greenMask) >> greenShift),
                    (real32)((color & blueMask) >> blueShift),
                    (real32)((color & alphaMask) >> alphaShift)};

                    texel = SRGB255ToLinear1(texel);

                // NOTE : Premultiplied Alpha!
                    texel.rgb *= texel.a;

                    texel = Linear1ToSRGB255(texel);

                    *sourceDest++ = (((uint32)(texel.a + 0.5f) << 24) |
                     ((uint32)(texel.r + 0.5f) << 16) |
                     ((uint32)(texel.g + 0.5f) << 8) |
                     ((uint32)(texel.b + 0.5f) << 0));
            }
        }        
    }

// NOTE : Because the usual bmp format is already bottom up, don't need a prestep anymore!
    result.pitch = result.width*BITMAP_BYTES_PER_PIXEL;

// There might be top down bitmaps. Use this method for them
#if 0
// Because we want to go to negative, set it to negative value
result.pitch = -result.width*BITMAP_BYTES_PER_PIXEL;

// Readjust the memory to be the start of the last row
// because the bitmap that was read is upside down. 
// Therefore, we need to read backward
result.memory = (uint8 *)result.memory - result.pitch*(result.height - 1);
#endif
    return result;
}

struct load_bitmap_work
{
    game_assets *assets;
    loaded_bitmap *bitmap;
    bitmap_slot_id bitmapID;
    background_task_with_memory *backgroundTask;

    asset_state finalState;
};

// TODO : Try to find where this should be?
internal background_task_with_memory *
BeginBackgroundTaskWithMemory(transient_state *tranState)
{
    background_task_with_memory *foundEmptyTaskSlot = 0;

    for(int taskID = 0;
        taskID < ArrayCount(tranState->backgroundTasks);
        ++taskID)
    {
        background_task_with_memory *task = tranState->backgroundTasks + taskID;

        if(!task->beingUsed)
        {
            foundEmptyTaskSlot = task;
            foundEmptyTaskSlot->beingUsed = true;
            foundEmptyTaskSlot->tempMemory = BeginTemporaryMemory(&foundEmptyTaskSlot->arena);
            break;
        }
    }

    return foundEmptyTaskSlot;
}

inline void
EndBackgroundTaskWithMemory(background_task_with_memory *task)
{
    EndTemporaryMemory(task->tempMemory);

    CompletePreviousWritesBeforeFutureWrites;
    task->beingUsed = false;
}

// TODO : Have weird bug with the background memory..
internal
PLATFORM_WORK_QUEUE_CALLBACK(LoadBitmapWork)
{
    load_bitmap_work *work = (load_bitmap_work *)data;

	asset_bitmap_info *info = work->assets->bitmapInfos + work->bitmapID.ID;
    *work->bitmap = DEBUGLoadBMP(info->fileName, info->alignPercentage); 

    CompletePreviousWritesBeforeFutureWrites;

    // Actually put inside the bitmap structure 
    work->assets->bitmaps[work->bitmapID.ID].bitmap = work->bitmap;
    work->assets->bitmaps[work->bitmapID.ID].state = work->finalState;

    EndBackgroundTaskWithMemory(work->backgroundTask);
}

internal void
LoadBitmapAsset(game_assets *assets, bitmap_slot_id ID)
{
    if(ID.ID != Asset_Type_Nothing && AtomCompareExchangeUint32((uint32 *)&assets->bitmaps[ID.ID].state, 
                                Asset_State_NotLoaded, 
                                Asset_State_Queued) == Asset_State_NotLoaded)
    {
        background_task_with_memory *backgroundTask = BeginBackgroundTaskWithMemory(assets->tranState);

        if(backgroundTask) 
        {
            load_bitmap_work *work = PushStruct(&backgroundTask->arena, load_bitmap_work);

            work->assets = assets;
            work->bitmapID = ID;
            work->backgroundTask = backgroundTask;
            work->bitmap = PushStruct(&assets->arena, loaded_bitmap);
            work->finalState = Asset_State_Loaded;
            
            PlatformAddEntry(assets->tranState->lowPriorityQueue, LoadBitmapWork, work);
        }
    }
}

struct riff_iterator
{
    uint8 *at;
    uint8 *end;
};

inline riff_iterator
ParseChunkAt(void *at, void *end)
{
    riff_iterator result = {};

    result.at = (uint8 *)at;
    result.end = (uint8 *)end;

    return result;
}

inline riff_iterator
NextChunk(riff_iterator iter)
{
    WAVE_chunk *currentChunk = (WAVE_chunk *)iter.at;

    // NOTE : Because of the padding inside the wav file
    uint32 size = (currentChunk->size + 1) & ~1;
    iter.at += sizeof(WAVE_chunk) + size;

    return iter;
}

inline uint32
GetChunkID(riff_iterator iter)
{
    WAVE_chunk *chunk = (WAVE_chunk *)iter.at;

    return chunk->ID;
}

inline void *
GetChunkData(riff_iterator iter)
{
    void *result = (WAVE_chunk *)(iter.at + sizeof(WAVE_chunk));

    return result;
}

inline uint32
GetChunkDataSize(riff_iterator iter)
{
    WAVE_chunk *chunk = (WAVE_chunk *)iter.at;

    return chunk->size;
}

inline bool32
IsValid(riff_iterator iter)
{
    bool32 result = (iter.at < iter.end);

    return result;
}

internal loaded_sound
DEBUGLoadWAV(char *fileName, uint32 sectionFirstSampleIndex, uint32 sectionSampleCount)
{
    loaded_sound result = {};

    debug_read_file_result readResult = DEBUGPlatformReadEntireFile(fileName);

    if(readResult.contentSize != 0)
    {
        // Master chunk
        WAVE_header *waveHeader = (WAVE_header *)readResult.content;
        Assert(waveHeader->riffID == WAVE_ChunkID_RIFF);
        Assert(waveHeader->waveID == WAVE_ChunkID_WAVE);

        uint32 channelCount = 0;
        int16 *sampleData = 0;
        uint32 sampleDataSize = 0;
        uint32 sampleCount = 0;

        // Loop through sub chunks for the wave file
        for(riff_iterator iter = ParseChunkAt(waveHeader + 1, (uint8 *)(waveHeader + 1) + waveHeader->size - 4);
            IsValid(iter);
            iter = NextChunk(iter))
        {
            switch(GetChunkID(iter))
            {
                case WAVE_ChunkID_fmt:
                {
                    WAVE_fmt *format = (WAVE_fmt *)GetChunkData(iter); 

                    /* NOTE : We will only support file with this format
                    pcm data type
                    stereo channel
                    bits per sample = 16
                    samples per second = 48000
                    */ 
                    Assert(format->wFormatTag == 1);
                    Assert(format->nChannels == 2);
                    Assert(format->wBitsPerSample == 16);
                    Assert(format->nSamplesPerSec == 48000);

                    channelCount = format->nChannels;
                }break;
 
                case WAVE_ChunkID_data:
                {
                    sampleData = (int16 *)GetChunkData(iter);
                    sampleDataSize = GetChunkDataSize(iter);
                }break;
            }
        }

        Assert(channelCount && sampleData);

        result.channelCount = channelCount;
        result.sampleCount = sampleDataSize / (channelCount * sizeof(int16));

        if(result.channelCount == 1)
        {
            result.samples[0] = sampleData;
            result.samples[1] = 0;
        }
        else if(result.channelCount == 2)
        {
            result.samples[0] = sampleData;
            // If the sound was stereo, there will be twice more samples;
            // so we should just add sampleCount.
            result.samples[1] = sampleData + result.sampleCount;

            // TODO : This swapping function makes right channel samples totally busted.
            for(uint32 sampleID = 0;
                sampleID < result.sampleCount;
                ++sampleID)
            {
                int16 source = sampleData[2 * sampleID];
                sampleData[2*sampleID] = sampleData[sampleID];
                sampleData[sampleID] = source;
            }
        }
        else
        {
            InvalidCodePath;
        }

        // TODO : Load right channels!
        result.channelCount = 1;

        // If there is actually a notaion about where to play
        if(sectionSampleCount)
        {
            Assert(sectionFirstSampleIndex + sectionSampleCount <= result.sampleCount);
            result.sampleCount = sectionSampleCount;

            for(uint32 channelIndex = 0;
                channelIndex < result.channelCount;
                ++channelIndex)
            {
                result.samples[channelIndex] += sectionFirstSampleIndex;
            }
        }
    }

    return result;
}

struct load_sound_work
{
    game_assets *assets;

    loaded_sound *sound;
    sound_slot_id soundID;

    background_task_with_memory *backgroundTask;

    asset_state finalState;
};

internal
PLATFORM_WORK_QUEUE_CALLBACK(LoadSoundWork)
{
    load_sound_work *work = (load_sound_work *)data;

    asset_sound_info *info = work->assets->soundInfos + work->soundID.ID;
    *work->sound = DEBUGLoadWAV(info->fileName, info->firstSampleIndex, info->samplesToPlay); 

    CompletePreviousWritesBeforeFutureWrites;

    // Actually put inside the bitmap structure 
    work->assets->sounds[work->soundID.ID].sound = work->sound;
    work->assets->sounds[work->soundID.ID].state = work->finalState;

    EndBackgroundTaskWithMemory(work->backgroundTask);
}

internal void
LoadSoundAsset(game_assets *assets, sound_slot_id ID)
{
    if(ID.ID != Asset_Type_Nothing && AtomCompareExchangeUint32((uint32 *)&assets->sounds[ID.ID].state, 
                                Asset_State_NotLoaded, 
                                Asset_State_Queued) == Asset_State_NotLoaded)
    {
        background_task_with_memory *backgroundTask = BeginBackgroundTaskWithMemory(assets->tranState);

        if(backgroundTask) 
        {
            load_sound_work *work = PushStruct(&backgroundTask->arena, load_sound_work);

            work->assets = assets;
            work->soundID = ID;
            work->backgroundTask = backgroundTask;
            work->sound = PushStruct(&assets->arena, loaded_sound);
            work->finalState = Asset_State_Loaded;
            
            PlatformAddEntry(assets->tranState->lowPriorityQueue, LoadSoundWork, work);
        }
    }
}

internal uint32
GetBestMatchAsset(game_assets *assets, asset_type_id typeID, 
                    asset_match_vector *matchVector, asset_weight_vector *weightVector) // How much I care about the match?
{ 
    real32 bestDiff = Real32Max;

    uint32 bestResult = 0;

    asset_type *assetType = assets->assetTypes + typeID;
    for(uint32 assetID = assetType->firstAssetID;
        assetID < assetType->onePastLastAssetID;
        ++assetID)
    {
        tagged_asset *taggedAsset = assets->taggedAssets + assetID;

        real32 totalWeightedDiff = 0.0f;

        for(uint32 tagID = taggedAsset->firstTagID;
            tagID < taggedAsset->onePastLastTagID;
            ++tagID)
        {
            asset_tag *tag = assets->tags + tagID;

            real32 a = matchVector->e[tag->ID];
            real32 b = tag->value;

            // If the value is positive, this will work
            real32 d0 = AbsoluteValue(a - b);
            real32 range = assets->tagMaxRange[tag->ID];
            // If the value is negative, this will work
            real32 d1 = AbsoluteValue((a - range * SignOf(a)) - b);

            real32 difference = Minimum(d0, d1);

            real32 weightedDiff = weightVector->e[tag->ID] * difference;
            totalWeightedDiff += weightedDiff;
        }

        if(bestDiff > totalWeightedDiff)
        {
            bestDiff = totalWeightedDiff;
            bestResult = taggedAsset->slot;
        }
    }

    return bestResult;
}

internal bitmap_slot_id
GetBestMatchBitmap(game_assets *assets, asset_type_id typeID, 
                    asset_match_vector *matchVector, asset_weight_vector *weightVector) // How much I care about the match?
{
    bitmap_slot_id result = {GetBestMatchAsset(assets, typeID, matchVector, weightVector)};
    return result;
}

internal sound_slot_id
GetBestMatchSound(game_assets *assets, asset_type_id typeID, 
                    asset_match_vector *matchVector, asset_weight_vector *weightVector) // How much I care about the match?
{
    sound_slot_id result = {GetBestMatchAsset(assets, typeID, matchVector, weightVector)};
    return result;
}

internal uint32
GetFirstAssetIDFrom(game_assets *assets, asset_type_id assetType)
{
	uint32 result = 0;

	asset_type *type = assets->assetTypes + assetType;

	if(type->firstAssetID != type->onePastLastAssetID)
	{
		tagged_asset *asset = assets->taggedAssets + type->firstAssetID;
		result = asset->slot;
	}

	return result; 
}

internal bitmap_slot_id
GetFirstBitmapFrom(game_assets *assets, asset_type_id assetType)
{
    bitmap_slot_id result = {GetFirstAssetIDFrom(assets, assetType)};
    return result;
}

internal sound_slot_id
GetFirstSoundFrom(game_assets *assets, asset_type_id assetType)
{
    sound_slot_id result = {GetFirstAssetIDFrom(assets, assetType)};
    return result;
}

internal bitmap_slot_id
DEBUGAddBitmapID(game_assets *assets, char *fileName, v2 alignPercentage)
{
	Assert(assets->DEBUGUsedBitmapCount < assets->bitmapCount); 

	bitmap_slot_id bitmapID = {assets->DEBUGUsedBitmapCount++};

	asset_bitmap_info *info = assets->bitmapInfos + bitmapID.ID; 
	info->alignPercentage = alignPercentage;
	info->fileName = fileName;

	return bitmapID;
}

internal sound_slot_id
DEBUGAddSoundID(game_assets *assets, char *fileName, uint32 sectionFirstSampleIndex,
                uint32 sectionSampleCount)
{
    Assert(assets->DEBUGUsedSoundCount < assets->soundCount); 

    sound_slot_id soundID = {assets->DEBUGUsedSoundCount++};

    asset_sound_info *info = assets->soundInfos + soundID.ID; 

    info->firstSampleIndex = sectionFirstSampleIndex;
    info->samplesToPlay = sectionSampleCount; 

    info->fileName = fileName;

    return soundID;
}

internal void
BeginGettingAssetsBasedOnType(game_assets *assets, asset_type_id assetTypeID)
{
	Assert(assets->DEBUGSelectedType == 0);
	assets->DEBUGSelectedType = assets->assetTypes + assetTypeID;

    // Initially the each asset type has no asset!
	assets->DEBUGSelectedType->firstAssetID = assets->DEBUGUsedAssetCount;
	assets->DEBUGSelectedType->onePastLastAssetID = assets->DEBUGSelectedType->firstAssetID;
}

internal void
AddBitmapInfoToType(game_assets *assets, char *fileName, v2 alignPercentage = V2(0.5f, 0.5f))
{
	Assert(assets->DEBUGSelectedType);
	tagged_asset *taggedAsset = assets->taggedAssets + assets->DEBUGSelectedType->onePastLastAssetID++;

	taggedAsset->firstTagID = assets->DEBUGUsedTagCount;
	taggedAsset->onePastLastTagID = assets->DEBUGUsedTagCount;

    // This is the value for both the bitmap and the bitmapinfo
    bitmap_slot_id newBitmapInfo = DEBUGAddBitmapID(assets, fileName, alignPercentage); 

	taggedAsset->slot = newBitmapInfo.ID;
    assets->DEBUGTaggedAsset = taggedAsset;
}

internal sound_slot_id
AddSoundInfoToType(game_assets *assets, char *fileName, uint32 sectionFirstSampleIndex = 0, 
                    uint32 sectionSampleCount = 0)
{
    Assert(assets->DEBUGSelectedType);
    tagged_asset *taggedAsset = assets->taggedAssets + assets->DEBUGSelectedType->onePastLastAssetID++;

    taggedAsset->firstTagID = assets->DEBUGUsedTagCount;
    taggedAsset->onePastLastTagID = assets->DEBUGUsedTagCount;

    sound_slot_id newSoundInfo = DEBUGAddSoundID(assets, fileName, sectionFirstSampleIndex, sectionSampleCount); 

    taggedAsset->slot = newSoundInfo.ID;
    assets->DEBUGTaggedAsset = taggedAsset;

    return newSoundInfo;
}

internal void
EndGettingAssetsBasedOnType(game_assets *assets)
{
	assets->DEBUGUsedAssetCount = assets->DEBUGSelectedType->onePastLastAssetID; 
	assets->DEBUGSelectedType = 0;
    assets->DEBUGTaggedAsset = 0;
}

internal void
AddTag(game_assets *assets, asset_tag_id tagID, real32 value)
{
    ++assets->DEBUGTaggedAsset->onePastLastTagID;
    asset_tag *tag = assets->tags + assets->DEBUGUsedTagCount++; 

    tag->ID = tagID;
    tag->value = value;
}

internal game_assets *
AllocateGameAssets(transient_state *tranState, memory_arena *arena, memory_index size)
{
	game_assets *result = PushStruct(arena, game_assets);
    SubArena(&result->arena, arena, size);
    result->tranState = tranState;

    result->bitmapCount = 256*Asset_Type_Count;
    result->bitmapInfos = PushArray(&result->arena, result->bitmapCount, asset_bitmap_info);
    result->bitmaps = PushArray(&result->arena, result->bitmapCount, asset_slot);
	result->DEBUGUsedBitmapCount = 1;

    result->soundCount = 256 * Asset_Type_Count;
    result->soundInfos = PushArray(&result->arena, result->soundCount, asset_sound_info);
    result->sounds = PushArray(&result->arena, result->soundCount, asset_slot);
    result->DEBUGUsedSoundCount = 1;

	result->taggedAssetCount = result->bitmapCount;
	result->taggedAssets = PushArray(&result->arena, result->taggedAssetCount, tagged_asset);
	result->DEBUGUsedAssetCount = 1;

    result->tagCount = 1024 * Asset_Type_Count;
    result->tags = PushArray(&result->arena, result->tagCount, asset_tag);

    for(int32 i = 0;
        i < ArrayCount(result->tagMaxRange);
        ++i)
    {
        result->tagMaxRange[i] = Real32Max;
    }

    result->tagMaxRange[Asset_Tag_FacingDirection] = 2.0f * Pi32;

    BeginGettingAssetsBasedOnType(result, Asset_Type_Square);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/square.bmp");
    EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_Number);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/number_0.bmp");
    AddTag(result, Asset_Tag_Numbering, 0.0f);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/number_1.bmp");
    AddTag(result, Asset_Tag_Numbering, 1.0f);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/number_2.bmp");
    AddTag(result, Asset_Tag_Numbering, 2.0f);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/number_3.bmp");
    AddTag(result, Asset_Tag_Numbering, 3.0f);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/number_4.bmp");
    AddTag(result, Asset_Tag_Numbering, 4.0f);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/number_5.bmp");
    AddTag(result, Asset_Tag_Numbering, 5.0f);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/number_6.bmp");
    AddTag(result, Asset_Tag_Numbering, 6.0f);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/number_7.bmp");
    AddTag(result, Asset_Tag_Numbering, 7.0f);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/number_8.bmp");
    AddTag(result, Asset_Tag_Numbering, 8.0f);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/number_9.bmp");
    AddTag(result, Asset_Tag_Numbering, 9.0f);
    EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_Tutorial);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/tutorial.bmp");
    EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_GameLogo);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/game_logo.bmp");
    EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_DigipenLogo);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/digipen_logo.bmp");
    EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_TeamLogo);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/team_logo.bmp");
    EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_Wave);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/wave.bmp");
    EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_NextWaveIn);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/next_wave_in.bmp");
    EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_NextWeapon);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/next_weapon.bmp");
    EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_ExclamationMark);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/exclamation_mark.bmp");
    EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_Arrow);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/arrow.bmp");
    EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_ArrowIndicator);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/arrow_indication.bmp");
    EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_PressR);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/press_r.bmp");
    EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_NextWeaponIndicator);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/laser_single.bmp");
    AddTag(result, Asset_Tag_WeaponType, 0.0f);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/laser_triple.bmp");
    AddTag(result, Asset_Tag_WeaponType, 1.0f);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/laser_backward.bmp");
    AddTag(result, Asset_Tag_WeaponType, 2.0f);
    EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_Credit);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/credit_1.bmp");
    AddTag(result, Asset_Tag_Sequence, 0.0f);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/credit_2.bmp");
    AddTag(result, Asset_Tag_Sequence, 1.0f);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/credit_3.bmp");
    AddTag(result, Asset_Tag_Sequence, 2.0f);
    EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_Thrust);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/test2/thrust.bmp");
    EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_Aim);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/aim.bmp");
    AddTag(result, Asset_Tag_WeaponType, 0.0f);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/aim_shotgun.bmp");
    AddTag(result, Asset_Tag_WeaponType, 1.0f);
    EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_Shield);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/shield.bmp");
    EndGettingAssetsBasedOnType(result);

	BeginGettingAssetsBasedOnType(result, Asset_Type_Tree);
	AddBitmapInfoToType(result, "../laser_is_dangerous/data/test2/tree00.bmp", V2(0.493827164f, 0.295652181f));
	EndGettingAssetsBasedOnType(result);

	BeginGettingAssetsBasedOnType(result, Asset_Type_Shadow);
	AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/thrust.bmp");
	EndGettingAssetsBasedOnType(result);

	BeginGettingAssetsBasedOnType(result, Asset_Type_Sword);
	AddBitmapInfoToType(result, "../laser_is_dangerous/data/test2/attack.bmp", V2(0.5f, 0.65625f));
	EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_RaycastAttack);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/raycast_attack.bmp");
    EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_LaserSurrounding);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/laser_particle.bmp");
    EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_Cloud);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/cloud_2.bmp");
    EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_Sun);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/sun_3.bmp");
    EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_Mountain);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/mountain.bmp");
    EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_Ship);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/test2/ship.bmp", V2(0.493827164f, 0.8f));
    EndGettingAssetsBasedOnType(result);   

    BeginGettingAssetsBasedOnType(result, Asset_Type_Ship);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/ship_side.bmp");
    AddTag(result, Asset_Tag_FacingDirection, 0.0f);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/ship_diagonal.bmp");
    AddTag(result, Asset_Tag_FacingDirection, 1.0f);
    AddBitmapInfoToType(result, "../laser_is_dangerous/data/LIDasset/ship_normal.bmp");
    AddTag(result, Asset_Tag_FacingDirection, 2.0f);
    EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_LaserSound);
    AddSoundInfoToType(result, "../laser_is_dangerous/data/LIDasset/sound/laser_attack_sound2.wav");
    EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_BoostSound);
    AddSoundInfoToType(result, "../laser_is_dangerous/data/LIDasset/sound/boost_sound_2.wav");
    EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_EnemyLaserSound);
    AddSoundInfoToType(result, "../laser_is_dangerous/data/LIDasset/sound/enemy_laser_1.wav");
    EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_EnemyAwareSound);
    AddSoundInfoToType(result, "../laser_is_dangerous/data/LIDasset/sound/enemy_aware_sound.wav");
    EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_EnemyDeathSound);
    AddSoundInfoToType(result, "../laser_is_dangerous/data/LIDasset/sound/enemy_death_3.wav");
    EndGettingAssetsBasedOnType(result);

    BeginGettingAssetsBasedOnType(result, Asset_Type_Music);
    uint32 oneSoundChunk = 10*48000;
    sound_slot_id lastLoadedSoundID = {};

    uint32 totalSoundSamples = 7468095;
#if 0

    for(uint32 firstSampleIndex = 0;
        firstSampleIndex < totalSoundSamples;
        firstSampleIndex += oneSoundChunk)
    {
        uint32 samplesToLoad = totalSoundSamples - firstSampleIndex;
        if(samplesToLoad > oneSoundChunk)
        {
            samplesToLoad = oneSoundChunk;
        }

        // sound_slot_id thisLoadedSoundID = AddSoundInfoToType(result, "../laser_is_dangerous/data/test3/music_test.wav", firstSampleIndex, samplesToLoad);
        sound_slot_id thisLoadedSoundID = AddSoundInfoToType(result, "../laser_is_dangerous/data/LIDasset/sound/normal.wav", firstSampleIndex, samplesToLoad);

        if(lastLoadedSoundID.ID)
        {
            result->soundInfos[lastLoadedSoundID.ID].nextIDToPlay = thisLoadedSoundID;
        }

        lastLoadedSoundID = thisLoadedSoundID;
    }
#else
    sound_slot_id thisLoadedSoundID = AddSoundInfoToType(result, "../laser_is_dangerous/data/LIDasset/sound/main.wav");
#endif
    EndGettingAssetsBasedOnType(result);

    return result;
}
