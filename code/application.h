/*

  This is the header that abstracts away the platform layer away from the application layer.
  Nothing from the platform layer should permiate through into the application layer because 
  the application layer is acting as a set of services that the platform layer can call 
  to get data about the application for drawing etc.

  Author: Justin Morrow
  Created On: 3/4/2021

*/

#if !defined(APPLICATION_H)

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

void GameUpdateAndRender(offscreen_graphics_buffer *buffer, 
                         application_sound_output_buffer *sound_buffer, int tone_hz, int blue_offset, int green_offset);

#define APPLICATION_H
#endif