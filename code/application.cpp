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

void GameUpdateAndRender(
    application_memory *memory, application_input *input,  
    offscreen_graphics_buffer * buffer, 
    application_sound_output_buffer *sound_buffer)
{
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
    
    application_state *app_state = (application_state *)memory->PermanentStorage;
    if(!memory->IsInitialized)
    {
        app_state->ToneHz = 256;
        app_state->BlueOffset = 0;
        app_state->GreenOffset = 0;

        // TODO: This may be more appropriate to do in the platform layer
        memory->IsInitialized = true;
    }

    // player 1 input
    application_controller_input *input0 = &input->Controllers[0];
    
    if(input0->IsAnalog)
    {
        real32 x_offset_speed = 4.0f;
        real32 y_offset_speed = 128.0f;
        
        app_state->BlueOffset += (int)x_offset_speed*(input0->EndX);
        app_state->GreenOffset = 256 + (int)y_offset_speed*(input0->EndY);
    }
    else
    {
        // NOTE: Use digital movement tuning
    }

    // Input.AButtonEndedDown;
    // Input.AButtonHalfTransitionCount;
    if(input0->Down.EndedDown)
    {
        app_state->GreenOffset += 1;
    }
    
    GameOutputSound(sound_buffer, app_state->ToneHz);
    RenderWeirdGradient(buffer, app_state->BlueOffset, app_state->GreenOffset);
}
