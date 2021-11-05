#ifndef FOX_RENDER_GROUP_H
#define FOX_RENDER_GROUP_H

/* NOTE :

   1) Everywhere outside the renderer, Y _always_ goes upward, X to the right.
   
   2) All bitmaps including the render target are assumed to be bottom-up
      (meaning that the first row pointer points to the bottom-most row
       when viewed on screen).

   3) It is mandatory that all inputs to the renderer are in world
      coordinates ("meters"), NOT pixels.  If for some reason something
      absolutely has to be specified in pixels, that will be explicitly
      marked in the API, but this should occur exceedingly sparingly.

   4) Z is a special coordinate because it is broken up into discrete slices,
      and the renderer actually understands these slices (potentially).

    // TODO: ZHANDLING

   5) All color values specified to the renderer as V4's are in
      NON-premulitplied alpha.
*/
struct loaded_bitmap
{
    v2 alignPercentage;
    real32 widthOverHeight;

    int32 width;
    int32 height;
    int32 pitch;
    void *memory;
};

struct enviromnet_map
{
    // level of detail
    loaded_bitmap lod[4];

    real32 pZ;
};

// NOTE : This is the position in world space.
struct render_basis
{
    v3 pos;
};

enum render_group_entry_type
{
    RenderGroupEntryType_render_group_entry_clear,

    RenderGroupEntryType_render_group_entry_coordinate_system,
    RenderGroupEntryType_render_group_entry_bitmap,
    RenderGroupEntryType_render_group_entry_rectangle,
};

// TODO : Get rid of this! This is another sloppy code!
struct render_group_entry_header
{
    render_group_entry_type type;
};

struct render_group_entry_clear
{
    v4 color;
};

struct render_group_entry_coordinate_system
{
    v2 origin;
    v2 xAxis;
    v2 yAxis;
    v4 color;

    loaded_bitmap *bitmap;
    loaded_bitmap *normalMap;

    enviromnet_map *top;
    enviromnet_map *middle;
    enviromnet_map *bottom;
    
    // v2 points[16];
};
    
struct render_group_entry_bitmap
{
    loaded_bitmap *bitmap;
    
    v2 pos;
    v2 xAxis;
    v2 yAxis;
    
    v2 size;
    v4 color;

    b32 pixelPerfect;
};

struct render_group_entry_rectangle
{
    v2 pos;
    v4 color;
    v2 scale;
    // NOTE : This is not the actual meter based dim that entity has!
    // This is pixel based rendering dim(how big it is in the screen!!)
    // Therefore, it is always multiplied by metersToPixels
    v2 dim;
};

struct render_transform
{
    real32 focalLength;
    real32 distanceAboveTarget;

    v2 screenCenter;
    v2 monitorHalfDimInMeters;
    
    // NOTE : This translate meters on the monitor into pixels on the monitor
    real32 metersToPixels;

    // TODO : Change this to offsetPos? That's being used in HM
    v3 entitySimPos;
    real32 scale;

    bool32 orthographic;
};

struct render_group
{
    // TODO : Does this need to be in here?
    struct game_assets *assets;
    
    render_transform transform;

    real32 globalAlpha;

    v2 monitorHalfDimInMeters;

    uint32 maxPushBufferSize;
    uint32 pushBufferSize;
    uint8 *pushBufferBase;
};

#endif