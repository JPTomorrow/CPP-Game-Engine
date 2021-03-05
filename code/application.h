/*

  This is the header that abstracts away the platform layer away from the application layer.
  Nothing from the platform layer should permiate through into the application layer because 
  the application layer is acting as a set of services that the platform layer can call 
  to get data about the application for drawing etc.

  Author: Justin Morrow
  Created On: 3/4/2021

*/

#if !defined(APPLICATION_H)

#if HANDMADE_SLOW
// TODO: Complete assertion macro
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

// easy count the elements in an array
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0])) 

/*
  TODO: Services that the platform layer provides to the game
*/

/*
  NOTE: Services that the game provides to the platform layer.
*/

// FOUR THINGS - timing, controller/keyboard input, bitmap buffer to use, sound buffer to use
// TODO: In the future, rendering _specifically_ will become a three-tiered abstraction!!!
struct offscreen_graphics_buffer 
{ 
    /*
        4 bytes ... pixel, pixel+1 etc 
        pixel in register: 0x xxRRGGBB
        pixel in memory: BB GG RR xx -> REVERSED! little endian arch.
        xx is padding
    */

    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

struct application_sound_output_buffer 
{
  int SamplesPerSecond;
  int SampleCount;
  int16 *Samples;
};

struct application_button_state
{
    int HalfTransitionCount;
    bool32 EndedDown;
};

struct application_controller_input
{
    bool32 IsAnalog;
    
    real32 StartX;
    real32 StartY;

    real32 MinX;
    real32 MinY;

    real32 MaxX;
    real32 MaxY;
    
    real32 EndX;
    real32 EndY;
    
    union
    {
        application_button_state Buttons[6];
        struct
        {
            application_button_state Up;
            application_button_state Down;
            application_button_state Left;
            application_button_state Right;
            application_button_state LeftShoulder;
            application_button_state RightShoulder;
        };
    };
};

struct application_input
{
    application_controller_input Controllers[4];
};

// persistent memory so that we never have to allocate memory during runtime 
struct game_memory
{
    bool32 IsInitialized;

    uint64 PermanentStorageSize;
    void *PermanentStorage; // NOTE: REQUIRED to be cleared to zero at startup

    uint64 TransientStorageSize;
    void *TransientStorage; // NOTE: REQUIRED to be cleared to zero at startup
};

struct game_state
{
    int ToneHz;
    int GreenOffset;
    int BlueOffset;
};

void GameUpdateAndRender(game_memory *memory, application_input *input, offscreen_graphics_buffer *buffer, 
                         application_sound_output_buffer *sound_buffer);



#define APPLICATION_H
#endif