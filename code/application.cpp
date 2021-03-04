/*

    This is the actual implementation layer for the application
    you write UI / Game code starting in this layer.
    
    The platform that you compile to will call into this layer
    and this layer will respond with releveant information 
    to get the platform layer to do things

    Author: Justin Morrow
*/

#include "application.h"

internal void GameOutputSound(application_sound_output_buffer *SoundBuffer, int ToneHz)
{
    local_persist real32 tSine;
    int16 ToneVolume = 3000;
    int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;

    int16 *SampleOut = SoundBuffer->Samples;
    for(int SampleIndex = 0;
        SampleIndex < SoundBuffer->SampleCount;
        ++SampleIndex)
    {
        // TODO(casey): Draw this out for people
        real32 SineValue = sinf(tSine);
        int16 SampleValue = (int16)(SineValue * ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        tSine += 2.0f*Pi32*1.0f/(real32)WavePeriod;
    }
}

void RenderWeirdGradient(offscreen_graphics_buffer *buffer, int x_offset, int y_offset)
{
    // all the casting is for byte alignment in windows
    // we are going to derefrence the pixel memory and write a color value to it.
    int width = buffer->Width;

    uint8 *row = (uint8 *)buffer->Memory;
    for (int y = 0; y < buffer->Height; ++y) 
    {
        uint32 *pixel = (uint32 *)row;
        for (int x = 0; x < buffer->Width; ++x) 
        {
            

            uint8 blue = (x + x_offset);
            uint8 green = (y + y_offset);
            *pixel++ = (green << 8) | blue;
        }

        row += buffer->Pitch;
    }
}

void GameUpdateAndRender(offscreen_graphics_buffer *buffer, application_sound_output_buffer *sound_buffer, int tone_hz, int blue_offset, int green_offset)
{
    GameOutputSound(sound_buffer, tone_hz);
    RenderWeirdGradient(buffer, blue_offset, green_offset);
}
