#ifndef FOX_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

//
//NOTE : compilers
//

#ifndef COMPILER_MSVC
#define COMPILER_MSVC 0 //windows
#endif

#ifndef COMPILER_LLVM
#define COMPILER_LLVM 0 //macOS
#endif

#if !COMPILER_MSVC && !COMPILER_LLVM
    #if _MSC_VER
        #undef COMPILER_MSVC
        #define COMPILER_MSVC 1
    #else
        // TODO : More compilers!
        #undef COMPILER_LLVM
        #define COMPILER_LLVM 1
    #endif
#endif

#if COMPILER_MSVC
    #include "intrin.h"
    #pragma intrinsic(_BitScanForward)
#endif

#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <float.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8; 
typedef uint16_t uint16; 
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8 i8;
typedef int16 i16;
typedef int32 i32;
typedef int64 i64;
typedef int32 b32;

typedef uint8_t u8; 
typedef uint16_t u16; 
typedef uint32_t u32;
typedef uint64_t u64;

typedef uintptr_t uintptr;

// Because the memory can differ from 32bit to 64 bit!
typedef size_t memory_index;

typedef float real32;
typedef double real64;

typedef float r32;
typedef double r64;

#define Real32Max FLT_MAX

#define internal static 
#define local_persist static 
#define global_variable static

#define Align16(value) ((value + 15) & ~15)
#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))

#define Pi32 3.14159265359f
#define Tau32 6.28318530717958647692f
#define Radian 0.0174533f
#define Grav32 6.67300E-2f


union v2
{
    struct
    {
        real32 x, y;
    };

    real32 e[2];
};

union v3
{
    struct
    {
        real32 x, y, z;
    };
    struct
    {
        real32 r, g, b;
    };
    struct
    {
        v2 xy;
        real32 ignored0_;
    };
    real32 e[3];
};

union v4
{
    struct
    {
        real32 x, y, z, w;
    };
    struct
    {
        union
        {
            v3 xyz;
            struct
            {
                real32 x, y, z;
            };
        };
        real32 w;
    };
    struct
    {
        union
        {
            v3 rgb;
            struct
            {
                real32 r, g, b;
            };
        };
        real32 a;
    };
    struct
    {
        v2 xy;
        real32 ignored0_;
        real32 ignored1_;
    };
    struct
    {
        real32 ignored2_;        
        v2 yz;
        real32 ignored3_;
    };
    struct
    {
        real32 ignored4_;
        real32 ignored5_;
        v2 zw;
    };
    
    real32 e[4];
};

#if FOX_SLOW
    // Always remember this! Assert will fire _IF_ the expression is _NOT TRUE_
    #define Assert(expression) if(!(expression)) {*(int *)0 = 0;}
#else
    #define Assert(expression)
#endif

// Use this in case that it's not supposed to happen just like assert,
// but we don't want the game to be straight up crash
#define InvalidCodePath Assert(!"InvalidCodePath")
// For default case in switch statements!
#define InvalidDefaultCase default: {InvalidCodePath;} break

//1024LL is for it works in 64bit
#define Kilobytes(value) (value * 1024LL)
#define Megabytes(value) (Kilobytes(value) * 1024LL)
#define Gigabytes(value) (Megabytes(value) * 1024LL)
#define Terabytes(value) (Gigabytes(value) * 1024LL)

/*Thread handle
For multi thread??*/
typedef struct thread_context
{
    int placeHolder;
} thread_context;

#define BITMAP_BYTES_PER_PIXEL 4

#if FOX_DEBUG

//Because we need to know the pointer AND the fileSize 
//for the DEBUGPlatformWriteEntireFile,
//we should be able to save these in DEBUGPlatformReadEntireFile function.
typedef struct debug_read_file_result
{
    uint32 contentSize;
    void *content;
}debug_read_file_result;

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(char *fileName)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(void *memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_WRTIE_ENTIRE_FILE(name) bool32 name(char *fileName, uint32 memorySize, void *memory)
typedef DEBUG_PLATFORM_WRTIE_ENTIRE_FILE(debug_platform_write_entire_file);

enum
{
    /* 0 */ DebugCycleCounter_GameUpdateAndRender,
   	/* 1 */ DebugCycleCounter_RenderGroupToOutputBuffer,
    /* 2 */ DebugCycleCounter_DrawSomethingSlowly,
    /* 3 */ DebugCycleCounter_DrawRectangleQuickly,
    /* 4 */ DebugCycleCounter_ProcessPixel,
    /* 5 */ DebugCycleCounter_FillPixel,
    // This DebugCycleCounter_Count indicates how many elements should be in the counter array
    // because this value is always all the Cycle Counter we need + 1!!
    DebugCycleCounter_Count,
};

typedef struct debug_cycle_counter
{
    uint64 cycleCount;
    // How many times have this been called?
    // Even if the cycleCount is high, there is a possibility that it it 
    // because it just has been called multiple times
    uint32 hitCount;
} debug_cycle_counter;

struct game_memory;

// Doing this so that we can use counters inside any function
// without passing the gameMemory all the time!
extern game_memory *debugGlobalMemory;
#if _MSC_VER
    
    // ID is just for in case we want multiple cycle counter and the name can be differ from each one
    // This can be anything - number, function name, ....
    #define BEGIN_TIMED_BLOCK(ID) uint64 startCycleCount##ID = __rdtsc();
    // We are += it because the same counter can be called in the same place multiple times, and we want the TOTAL counter
    #define END_TIMED_BLOCK(ID) debugGlobalMemory->counters[DebugCycleCounter_##ID].cycleCount += __rdtsc() - startCycleCount##ID; debugGlobalMemory->counters[DebugCycleCounter_##ID].hitCount++;
    #define END_TIMED_BLOCK_COUNTED(ID, counter) debugGlobalMemory->counters[DebugCycleCounter_##ID].cycleCount += __rdtsc() - startCycleCount##ID; debugGlobalMemory->counters[DebugCycleCounter_##ID].hitCount += counter;
#else
    #define BEGIN_TIMED_BLOCK(ID)
    #define END_TIMED_BLOCK(ID)
    #define END_TIMED_BLOCK_COUNTED(ID)
#endif

#endif

typedef struct game_button_state
{
    //No matter what crazy stuff this button has passed
    //between the last time we pulled the button state and now
    //like up->down->up->down->up->down....
    //ultimately, is it down or not?
    // NOTE : This means ended to be down.
    bool32 endedDown;
    bool32 wasDown;
    int halfTransitionCount;
} game_button_state;

typedef struct game_controller
{
    bool isConnected;
    bool isAnalog;

    //In case we need to know the actual value
    real32 averageStickX;
    real32 averageStickY;
    
    union//so that we can both approach as like buttons[0] and moveup
    {
        game_button_state buttons[14];
        struct
        {
            //move buttons
            /*
            For example, if we just want to know that the stick value is enough 
            to move the character, we can just use these buttons
            because stick value(analog value) are also converted to this digital buttons!!
            */
            game_button_state moveUp;
            game_button_state moveDown;
            game_button_state moveLeft;
            game_button_state moveRight;
            
            //a,x,y,b buttons
            game_button_state actionUp;
            game_button_state actionDown;
            game_button_state actionLeft;
            game_button_state actionRight;   

            game_button_state leftShoulder;
            game_button_state rightShoulder;

            game_button_state start;
            game_button_state back;

            // TODO : Remove this!
            game_button_state credit;
            game_button_state mute;
            game_button_state nextWave;
            game_button_state killPlayer;
        };
    };
} game_controller;

typedef struct game_input
{
    game_button_state mouseButtons[5];
    r32 mouseX, mouseY, mouseZ;

    real32 dtForFrame;

    game_controller controllers[5];
} game_input;

struct platform_work_queue;
#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(platform_work_queue *queue, void *data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(platform_work_queue_callback);

typedef void platform_add_entry(platform_work_queue *queue, platform_work_queue_callback *callback, void *data);
typedef void platform_complete_all_work(platform_work_queue *queue);

typedef struct game_memory
{
    uint64 permanentStorageSize;
    void *permanentStorage;

    uint64 transientStorageSize;
    void *transientStorage;

    debug_platform_read_entire_file *debugPlatformReadEntireFile;
    debug_platform_write_entire_file *debugPlatformWriteEntireFile;
    debug_platform_free_file_memory *debugPlatformFreeFileMemory;

    platform_work_queue *highPriorityQueue;
    platform_work_queue *lowPriorityQueue;

    // Multithread functions
    platform_add_entry *PlatformAddEntry;
    platform_complete_all_work *PlatformCompleteAllWork;

#if FOX_DEBUG
    debug_cycle_counter counters[DebugCycleCounter_Count];
#endif
} game_memory;

typedef struct game_offscreen_buffer
{
    void *memory;
    int width;
    int height;
    int pitch;
    // TODO : Do we really need this?
    int bytesPerPixel;
} game_offscreen_buffer;

typedef struct game_sound_output_buffer
{
    int samplesPerSecond;
    int sampleCount;
    int16 *samples;
} game_sound_output_buffer;

#define GAME_UPDATE_AND_RENDER(name) void name(thread_context *thread, game_memory *memory, game_offscreen_buffer *buffer, game_input *input, game_input *oldInput)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_GET_SOUND_SAMPLES(name) void name(thread_context *thread, game_memory *memory, game_sound_output_buffer *soundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

#ifdef __cplusplus
}
#endif


#define FOX_PLATFORM_H
#endif
