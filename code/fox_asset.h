
#ifndef FOX_ASSET_H
#define FOX_ASSET_H

struct bitmap_slot_id
{
    uint32 ID;
};

struct sound_slot_id
{
    uint32 ID;
};

struct loaded_sound
{
    uint32 channelCount;
    uint32 sampleCount;
    // NOTE : This is array so that we can later support multiple channels
    // Dolby atmosphere?? 
    // samples are int not uint because they can be negative values!
    int16 *samples[2];
};

enum asset_state
{
    Asset_State_NotLoaded,
    Asset_State_Queued,
    Asset_State_Loaded,
};
struct asset_slot // NOTE : Struct that actually holds the asset
{
    asset_state state;

    // NOTE : What a nice way to hold both asset!
    union
    {
        loaded_bitmap *bitmap;
        loaded_sound *sound;
    };
};

enum asset_type_id
{
    // For making empty bitmap
    Asset_Type_Nothing,

    Asset_Type_Square,
    //
    // NOTE : Bitmap types
    //
    Asset_Type_Number,
    Asset_Type_Tree,
    Asset_Type_Shadow,
    Asset_Type_Stairwell,

    Asset_Type_Sword,
    Asset_Type_RaycastAttack,
    Asset_Type_LaserSurrounding,
    // Asset_Type_Attack,

    Asset_Type_Cloud,
    Asset_Type_Sun,
    Asset_Type_Mountain,

    Asset_Type_Head,
    Asset_Type_Cape,
    Asset_Type_Torso,

    Asset_Type_Tutorial,
    Asset_Type_GameLogo,
    Asset_Type_DigipenLogo,
    Asset_Type_TeamLogo,
    Asset_Type_Wave,
    Asset_Type_NextWaveIn,
    Asset_Type_NextWeapon,
    Asset_Type_NextWeaponIndicator,
    Asset_Type_PressR,
    Asset_Type_Credit,

    Asset_Type_ExclamationMark,
    Asset_Type_Arrow,
    Asset_Type_ArrowIndicator,

    Asset_Type_Ship,
    Asset_Type_Propell,
    Asset_Type_Thrust,
    Asset_Type_Aim,
    Asset_Type_Shield,

    //
    // NOTE : Sound Types
    // TODO : Make sound to have 'sound' at the end so that I can recognize more easily.
    //
    
    Asset_Type_Music,
    Asset_Type_BoostSound,
    Asset_Type_LaserSound,
    Asset_Type_EnemyLaserSound,
    Asset_Type_EnemyDeathSound,
    Asset_Type_EnemyAwareSound,

    Asset_Type_Count,
};

enum asset_tag_id
{
    Asset_Tag_FacingDirection,
    Asset_Tag_AnimationIndex,
    Asset_Tag_Sequence,
    Asset_Tag_Numbering,
    Asset_Tag_WeaponType,

    Asset_Tag_Count,
};

struct asset_match_vector
{
    real32 e[Asset_Tag_Count];
};

struct asset_weight_vector
{
    real32 e[Asset_Tag_Count];
};

struct asset_tag
{
    // This means any character of this asset
    // tallness, shiness... and the value means how much it is 'character'
    uint32 ID;
    real32 value;
};
struct tagged_asset
{
    uint32 firstTagID;
    uint32 onePastLastTagID;
    uint32 slot;
};

struct playing_sound
{
    sound_slot_id ID;
    real32 volume[2];
    uint32 samplesPlayed;

    playing_sound *next;
};

struct asset_type
{
    uint32 firstAssetID;
    uint32 onePastLastAssetID;
};

struct asset_type_struct
{
    uint32 alignPercentage;
    char *fileName;
};

struct asset_bitmap_info
{
    v2 alignPercentage;
    char *fileName;
};

struct asset_sound_info
{
    char *fileName;

    uint32 firstSampleIndex;
    uint32 samplesToPlay; 

    sound_slot_id nextIDToPlay;
};

struct game_assets
{
    memory_arena arena;

    // TODO : Not thrilled about this back-pointer
    struct transient_state *tranState;

    uint32 bitmapCount;
    asset_bitmap_info *bitmapInfos;
    asset_slot *bitmaps;
    uint32 DEBUGUsedBitmapCount;

    // We can use this to look up all the assets with same types
    asset_type assetTypes[Asset_Type_Count];

    uint32 soundCount;
    asset_sound_info *soundInfos;
    asset_slot *sounds;
    uint32 DEBUGUsedSoundCount;

    uint32 tagCount;
    asset_tag *tags;
    // Value of the tag should not be bigger than this one
    real32 tagMaxRange[Asset_Tag_Count];

    uint32 taggedAssetCount;
    // NOTE : This can be any assets
    tagged_asset *taggedAssets;
    tagged_asset *DEBUGTaggedAsset;
    uint32 DEBUGUsedTagCount;

    // TODO : Get rid of these debug items
    asset_type *DEBUGSelectedType;
    uint32 DEBUGUsedAssetCount;
};

inline loaded_bitmap *
GetLoadedBitmap(game_assets *assets, bitmap_slot_id ID)
{
    loaded_bitmap *result = assets->bitmaps[ID.ID].bitmap;
    return result;
}

inline asset_bitmap_info *
GetBitmapInfo(game_assets *assets, bitmap_slot_id ID)
{
    asset_bitmap_info *result = assets->bitmapInfos + ID.ID;

    return result;
}

inline asset_sound_info *
GetSoundInfo(game_assets *assets, sound_slot_id ID)
{
    asset_sound_info *result = assets->soundInfos + ID.ID;

    return result;
}

inline loaded_sound *
GetLoadedSound(game_assets *assets, sound_slot_id ID)
{
    loaded_sound *result = assets->sounds[ID.ID].sound;
    return result;
}

internal void 
FreeSound(game_assets *assets, sound_slot_id ID)
{
// #define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(void *memory)
}

// These functions should be called externally
internal void LoadBitmapAsset(game_assets *assets, bitmap_slot_id ID);
inline void PrefetchBitmapAsset(game_assets *assets, bitmap_slot_id ID)
{
    LoadBitmapAsset(assets, ID);
}
internal void LoadSoundAsset(game_assets *assets, sound_slot_id ID);
inline void PrefetchSoundAsset(game_assets *assets, sound_slot_id ID)
{
    LoadSoundAsset(assets, ID);
}

#endif