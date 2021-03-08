/*

  This is the header that abstracts away the platform layer away from the application layer.
  Nothing from the platform layer should permiate through into the application layer because 
  the application layer is acting as a set of services that the platform layer can call 
  to get data about the application for drawing etc.

  Author: Justin Morrow
  Created On: 3/4/2021

*/

#if !defined(APPLICATION_H)

#if APPLICATION_SLOW
// TODO: Complete assertion macro
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

// define hard memory values with this
#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

// easy count the elements in an array
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

inline uint32 SafeTruncateUInt64(uint64 value)
{
    // TODO: Defines for maximum values
    Assert(value <= 0xFFFFFFFF);
    uint32 result = (uint32)value;
    return(result);
}

/*
  TODO: Services that the platform layer provides to the application
*/

struct debug_read_file_result
{
    uint32 ContentsSize;
    void *Contents;
};

#if APPLICATION_INTERNAL
/* IMPORTANT:

   These are NOT for doing anything in the shipping application - they are
   blocking and the write doesn't protect against lost data!
*/

internal debug_read_file_result DEBUGPlatformReadEntireFile(char *filename);
internal void DEBUGPlatformFreeFileMemory(void *memory);
internal bool32 DEBUGPlatformWriteEntireFile(char *filename, uint32 memory_size, void *memory);
#endif

/*
  NOTE: Services that the application provides to the platform layer.
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
    bool32 IsConnected;
    bool32 IsAnalog;
    real32 StickAverageX;
    real32 StickAverageY;
    
    union
    {
        application_button_state Buttons[12];
        struct
        {
            application_button_state MoveUp;
            application_button_state MoveDown;
            application_button_state MoveLeft;
            application_button_state MoveRight;

            application_button_state ActionUp;
            application_button_state ActionDown;
            application_button_state ActionLeft;
            application_button_state ActionRight;

            application_button_state LeftShoulder;
            application_button_state RightShoulder;

            application_button_state Back;
            application_button_state Start;
        };
    };
};

struct application_input
{
    application_controller_input Controllers[5];
};

// get a controller / keyboard and check to see if it is avalailable
inline application_controller_input *GetController(application_input *input, int controller_idx)
{
    Assert(controller_idx < ArrayCount(input->Controllers));
    application_controller_input *result = &input->Controllers[controller_idx];
    return result;
}

// persistent memory so that we never have to allocate memory during runtime 
struct application_memory
{
    bool32 IsInitialized;

    uint64 PermanentStorageSize; // permanent storage for the application
    void *PermanentStorage; // NOTE: REQUIRED to be cleared to zero at startup

    uint64 TransientStorageSize; // storage for carrying over information from a previous frame
    void *TransientStorage; // NOTE: REQUIRED to be cleared to zero at startup
};

struct application_state
{
    int ToneHz;
    int GreenOffset;
    int BlueOffset;
};

void ApplicationUpdateAndRender(application_memory *memory, application_input *input, offscreen_graphics_buffer *buffer, 
                         application_sound_output_buffer *sound_buffer);

#define APPLICATION_H
#endif