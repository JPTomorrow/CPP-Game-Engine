#if !defined(WIN32_PLATFORM_LAYER_H)

struct win32_window_dimension
{
    int Width;
    int Height;
};

struct win32_sound_output
{
    int SamplesPerSecond;
    int ToneHz;
    int16 ToneVolume;
    uint32 RunningSampleIndex;
    int WavePeriod;
    int BytesPerSample;
    DWORD SecondaryBufferSize;
    real32 tSine;
    int LatencySampleCount;
    DWORD SafetyBytes;
};

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

struct win32_debug_time_marker
{
    DWORD OutputPlayCursor;
    DWORD OutputWriteCursor;
    DWORD OutputLocation;
    DWORD OutputByteCount;


    DWORD FlipPlayCursor;
    DWORD FlipWriteCursor;
};

#define WIN32_PLATFORM_LAYER_H
#endif