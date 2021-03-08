/*
    This is the platform layer of this engine.

    Author:Justin Morrow
    Created On: 2/26/2021
*/

/*
  - Saved state locations
  - Getting a handle to our own executable file
  - Asset loading path
  - Threading (launch a thread)
  - Raw Input (support for multiple keyboards)
  - Sleep/timeBeginPeriod
  - ClipCursor() (for multimonitor support)
  - Fullscreen support
  - WM_SETCURSOR (control cursor visibility)
  - QueryCancelAutoplay
  - WM_ACTIVATEAPP (for when we are not the active application)
  - Blit speed improvements (BitBlt)
  - Hardware acceleration (OpenGL or Direct3D or BOTH??)
  - GetKeyboardLayout (for French keyboards, international WASD support)
*/

#include <windows.h>
#include <stdint.h>
#include <Xinput.h>
#include <dsound.h>
#include <math.h>
#include <string>
#include <stdio.h>
#include <malloc.h>

#define internal static  // use for functions that are private to file scope
#define local_persist static  // use for local scope static variables
#define global_variable static  // use for static global variables

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;    
typedef uint64_t uint64;

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef int32 bool32;

typedef float real32;
typedef double real64;

#define Pi32 3.14159265359f // large approx

#include "application.h"
#include "application.cpp"
#include "win32_platform_layer.h"

//
// Input
//

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) {
    return ERROR_DEVICE_NOT_CONNECTED; // error returned if library can not be found to avoid crashes
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub; // dynamically loaded functions
#define XInputGetState XInputGetState_ // remap to avoid naming conflict

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) {
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

// load XInput library
internal void Win32LoadXInput() {
    HMODULE x_input_library = LoadLibrary("xinput1_4.dll");
    if(!x_input_library) {
        // TODO: maybe diagnostic
        x_input_library = LoadLibrary("xinput9_1_0.dll");
    }

    if(!x_input_library) {
        // TODO: maybe diagnostic
        x_input_library = LoadLibrary("xinput1_3.dll");
    }
    
    if(x_input_library) {
        // load the specific functions that we need, stubs are above this function
        XInputGetState = (x_input_get_state *)GetProcAddress(x_input_library, "XInputGetState");
        if(!XInputGetState) { XInputGetState = XInputGetStateStub; }
         
        XInputSetState = (x_input_set_state *)GetProcAddress(x_input_library, "XInputSetState");
        if(!XInputSetState) { XInputSetState = XInputSetStateStub; }

        // TODO: diagnostics
    }
    else {
        // TODO: diagnostics
    }
}

// Process a digital button press from an XInput controller
internal void Win32ProcessXInputDigitalButton(
    DWORD x_input_button_state,
    application_button_state *old_state, DWORD button_bit,
    application_button_state *new_state)
{
    new_state->EndedDown = ((x_input_button_state & button_bit) == button_bit);
    new_state->HalfTransitionCount = (old_state->EndedDown != new_state->EndedDown) ? 1 : 0;
}

// Process a digital button press from an XInput controller
internal void Win32ProcessKeyboardMessage(application_button_state *new_state, bool32 is_down)
{
    Assert(new_state->EndedDown != is_down);
    new_state->EndedDown = is_down;
    ++new_state->HalfTransitionCount;
}

internal real32 Win32ProcessXInputStickPosition(SHORT thumb_stick_value, SHORT deadzone)
{
    real32 result = 0;
    if(thumb_stick_value < -deadzone)
    {
        result = (real32)thumb_stick_value / 32768.0f;
    }
    else if(thumb_stick_value > deadzone)
    {
        result = (real32)thumb_stick_value / 32767.0f;
    }

    return result;
}

//
// Sound
//

global_variable IDirectSoundBuffer *SecondaryBuffer;

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(dsound_create);

// Initialize DirectSound
internal void Win32InitDSound(HWND window, int32 samples_per_sound, int32 buffer_size) 
{ 
    // intialize the sound and start playing

    // load the library
    HMODULE dsound_library = LoadLibrary("dsound.dll");

    if(dsound_library) {

        // get direct sound object
        dsound_create *DirectSoundCreate = (dsound_create *)GetProcAddress(dsound_library, "DirectSoundCreate");
        IDirectSound *dsound; 

        if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &dsound, 0))) 
        {

            WAVEFORMATEX wave_format = {};
            wave_format.wFormatTag = WAVE_FORMAT_PCM;
            wave_format.nChannels = 2;
            wave_format.nSamplesPerSec = samples_per_sound;
            wave_format.wBitsPerSample = 16;
            wave_format.nBlockAlign = (wave_format.nChannels * wave_format.wBitsPerSample) / 8;
            wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nBlockAlign;
            wave_format.cbSize = 0;

            if(SUCCEEDED(dsound->SetCooperativeLevel(window, DSSCL_PRIORITY))) 
            {
                
                // create a primary buffer
                DSBUFFERDESC buffer_description = {};
                buffer_description.dwFlags = DSBCAPS_PRIMARYBUFFER;
                buffer_description.dwSize = sizeof(buffer_description);

                IDirectSoundBuffer *primary_buffer;

                if(SUCCEEDED(dsound->CreateSoundBuffer(&buffer_description, &primary_buffer, 0))) 
                {
                    
                    HRESULT error = primary_buffer->SetFormat(&wave_format);
                    if(SUCCEEDED(error)) 
                    {
                        // format has been set
                        OutputDebugStringA("Primary buffer format was set\n");
                    }
                    else 
                    {
                        // TODO: diagnostic
                    }
                }
            }
            else 
            {
                // TODO: diagnostic
            }
            
            // create secondary buffer
            DSBUFFERDESC buffer_description = {};
            buffer_description.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
            buffer_description.dwSize = sizeof(buffer_description);
            buffer_description.dwBufferBytes = buffer_size;
            buffer_description.lpwfxFormat = &wave_format;

            if(SUCCEEDED(dsound->CreateSoundBuffer(&buffer_description, &SecondaryBuffer, 0))) 
            {
                OutputDebugStringA("Secondary buffer created\n");
            }
            else 
            {
                // TODO: diagnostic
            }

            

            // start playing sound
        }
        else 
        {
            // TODO: diagnostics
        }
    }
    else 
    {
        // TODO: diagnostics
    }
}

// clear the sound buffer buffer
internal void Win32ClearBuffer(win32_sound_output *sound_output)
{
    VOID *region1;
    DWORD region1_size;
    VOID *region2;
    DWORD region2_size;
    if(SUCCEEDED(SecondaryBuffer->Lock(0, sound_output->SecondaryBufferSize,
                                             &region1, &region1_size,
                                             &region2, &region2_size,
                                             0)))
    {
        // TODO(casey): assert that Region1Size/Region2Size is valid
        uint8 *dest_sample = (uint8 *)region1;
        for(DWORD byte_idx = 0;
            byte_idx < region1_size;
            ++byte_idx)
        {
            *dest_sample++ = 0;
        }
        
        dest_sample = (uint8 *)region2;
        for(DWORD byte_idx = 0;
            byte_idx < region2_size;
            ++byte_idx)
        {
            *dest_sample++ = 0;
        }

        SecondaryBuffer->Unlock(region1, region1_size, region2, region2_size);
    }
}

// fill the sound buffer with the bytes provided
internal void Win32FillSoundBuffer(win32_sound_output *sound_output, DWORD byte_to_lock, DWORD bytes_to_write,
                     application_sound_output_buffer *source_buffer)
{
    VOID *region1;
    DWORD region1_size;
    VOID *region2;
    DWORD region2_size;
    if(SUCCEEDED(SecondaryBuffer->Lock(byte_to_lock, bytes_to_write,
                                             &region1, &region1_size,
                                             &region2, &region2_size,
                                             0)))
    {
        DWORD region1_sample_count = region1_size/sound_output->BytesPerSample;
        int16 *dest_sample = (int16 *)region1;
        int16 *source_sample = source_buffer->Samples;
        for(DWORD sample_idx = 0;
            sample_idx < region1_sample_count;
            ++sample_idx)
        {
            *dest_sample++ = *source_sample++;
            *dest_sample++ = *source_sample++;
            ++sound_output->RunningSampleIndex;
        }

        DWORD region2_sample_count = region2_size/sound_output->BytesPerSample;
        dest_sample = (int16 *)region2;
        for(DWORD sample_idx = 0;
            sample_idx < region2_sample_count;
            ++sample_idx)
        {
            *dest_sample++ = *source_sample++;
            *dest_sample++ = *source_sample++;
            ++sound_output->RunningSampleIndex;
        }

        SecondaryBuffer->Unlock(region1, region1_size, region2, region2_size);
    }
}

internal void Win32DebugDrawVertical(
    win32_offscreen_buffer *back_buffer, int x, 
    int top, int bottom, uint32 color)
{
    uint8 *pixel = (uint8 *)back_buffer->Memory + x * back_buffer->BytesPerPixel + top * back_buffer->Pitch;
    for (int y = top; y < bottom; ++y)
    {
        *(uint32 *)pixel = color;
        pixel += back_buffer->Pitch;
    }
}

inline void Win32DrawSoundBufferMarker(
    win32_offscreen_buffer *back_buffer, win32_sound_output *sound_output,
    real32 coefficient, int pad_x, int top, int bottom, DWORD value, uint32 color)
{
    Assert(value < sound_output->SecondaryBufferSize);
    real32 x_real32 = (coefficient * (real32)value);
    int x = pad_x + (int)x_real32;
    Win32DebugDrawVertical(back_buffer, x, top, bottom, color);
}

// display a debug visual of the sound buffer for testing
internal void Win32DebugSyncSound(
    win32_offscreen_buffer *back_buffer, 
    int marker_count, win32_debug_time_marker *markers, 
    win32_sound_output *sound_output, real32 target_seconds_per_frame)
{
    int pad_x = 16;
    int pad_y = 16;

    int top = pad_y;
    int bottom = back_buffer->Height - pad_y; 

    real32 cof = (real32)(back_buffer->Width - 2 * pad_x) / (real32)sound_output->SecondaryBufferSize;
    for (int marker_idx = 0; marker_idx < marker_count; ++marker_idx)
    {
        win32_debug_time_marker *this_marker = &markers[marker_idx];
        Win32DrawSoundBufferMarker(
            back_buffer, sound_output, cof, pad_x, 
            top, bottom, this_marker->PlayCursor, 0xFFFFFFFF);
        Win32DrawSoundBufferMarker(
            back_buffer, sound_output, cof, pad_x, 
            top, bottom, this_marker->WriteCursor, 0xFFFF0000);        
    }
}



//
// File IO
//

// DEBUG: read the contents of a file 
internal debug_read_file_result DEBUGPlatformReadEntireFile(char *filename)
{
    debug_read_file_result result = {};
    
    HANDLE file_handle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(file_handle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER file_size;
        if(GetFileSizeEx(file_handle, &file_size))
        {
            uint32 file_size32 = SafeTruncateUInt64(file_size.QuadPart);
            result.Contents = VirtualAlloc(0, file_size32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            if(result.Contents)
            {
                DWORD bytes_read;
                if(ReadFile(file_handle, result.Contents, file_size32, &bytes_read, 0) &&
                   (file_size32 == bytes_read))
                {
                    // File read successfully
                    result.ContentsSize = file_size32;
                }
                else
                {                    
                    // TODO: Logging
                    DEBUGPlatformFreeFileMemory(result.Contents);
                    result.Contents = 0;
                }
            }
            else
            {
                // TODO: Logging
            }
        }
        else
        {
            // TODO: Logging
        }

        CloseHandle(file_handle);
    }
    else
    {
        // TODO: Logging
    }

    return(result);
}

// DEBUG: free file memory
internal void DEBUGPlatformFreeFileMemory(void *memory)
{
    if(memory)
    {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
}

// DEBUG: write bytes into a file
internal bool32 DEBUGPlatformWriteEntireFile(char *filename, uint32 memory_size, void *memory)
{
    bool32 result = false;
    
    HANDLE file_handle = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if(file_handle != INVALID_HANDLE_VALUE)
    {
        DWORD bytes_written;
        if(WriteFile(file_handle, memory, memory_size, &bytes_written, 0))
        {
            // File read successfully
            result = (bytes_written == memory_size);
        }
        else
        {
            // TODO: Logging
        }

        CloseHandle(file_handle);
    }
    else
    {
        // TODO: Logging
    }

    return(result);
}

//
// Graphics
//

global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackBuffer;

// get the dimensions of the provided window handle
internal win32_window_dimension Win32GetWindowDimension(HWND window)
{
    win32_window_dimension Result;
    
    RECT ClientRect;
    GetClientRect(window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return(Result);
}

// Resize the window buffer that is passed in
internal void Win32ResizeDIBSection(win32_offscreen_buffer *buffer, int width, int height) 
{

    // TODO: If you run into bliting problems later remeber 
    // it was in episode 3 where you didnt get a window that blited entirely black
    if(buffer->Memory) 
    {
        VirtualFree(buffer->Memory, 0, MEM_RELEASE);
    }

    buffer->Width = width;
    buffer->Height = height;
    int bytes_per_pixel = 4;
    buffer->BytesPerPixel = bytes_per_pixel;

    buffer->Info.bmiHeader.biSize = sizeof(buffer->Info.bmiHeader);
    buffer->Info.bmiHeader.biWidth = buffer->Width;
    buffer->Info.bmiHeader.biHeight = -buffer->Height; // for top-down row drawing must be negative
    buffer->Info.bmiHeader.biPlanes = 1;
    buffer->Info.bmiHeader.biBitCount = 32;
    buffer->Info.bmiHeader.biCompression = BI_RGB;

    int bit_map_memory_size = (buffer->Width * buffer->Height) * bytes_per_pixel;

    buffer->Memory = VirtualAlloc(0, bit_map_memory_size, MEM_COMMIT, PAGE_READWRITE);
    buffer->Pitch = width * bytes_per_pixel;    
    // probably clear to black
}

// diplay the passed buffer to the screen
internal void Win32DisplayBufferInWindow(
    win32_offscreen_buffer *buffer, HDC device_context, 
    int window_width, int window_height)
{

    StretchDIBits(device_context,
                  0, 0, window_width, window_height,
                  0, 0, buffer->Width, buffer->Height,
                  buffer->Memory,
                  &buffer->Info,
                  DIB_RGB_COLORS, SRCCOPY);
}

// message pump
LRESULT CALLBACK Win32MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    switch(message) {
        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        case WM_DESTROY:
        {
            Running = false;
        } break;
        case WM_CLOSE:
        {
            Running = false;
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            Assert(!"You are recieving keyboard input in the message pump callback. Dis is baaad.");
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC device_context = BeginPaint(window, &paint);

            win32_window_dimension dim = Win32GetWindowDimension(window);
            Win32DisplayBufferInWindow(&GlobalBackBuffer, device_context, dim.Width, 
                                       dim.Height);
            EndPaint(window, &paint);
        } break;
        
        default:
        {
            result = DefWindowProc(window, message, wParam, lParam);
        } break;
    }

    return result;
}

// process a message and handle with default dispatch if nessesary
internal void Win32ProcessPendingMessages(application_controller_input *kbd_controller)
{
    MSG message;
    while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) 
    {
        switch(message.message) 
        {
            case WM_QUIT:
            {
                Running = false;
            } break;

            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                uint32 vkcode = (uint32)message.wParam;
                bool was_down = (message.lParam & (1 << 30)) != 0;
                bool is_down =  (message.lParam & (1 << 31)) == 0;

                // eat key repeats
                if(was_down != is_down)
                {
                    if(vkcode == 'W') 
                    {
                        Win32ProcessKeyboardMessage(&kbd_controller->MoveUp, is_down);
                    }
                    else if(vkcode == 'A') 
                    {
                        Win32ProcessKeyboardMessage(&kbd_controller->MoveLeft, is_down);
                    }
                    else if(vkcode == 'S') 
                    {
                        Win32ProcessKeyboardMessage(&kbd_controller->MoveDown, is_down);
                    }
                    else if(vkcode == 'D') 
                    {
                        Win32ProcessKeyboardMessage(&kbd_controller->MoveRight, is_down);
                    }
                    else if(vkcode == 'Q') 
                    {
                        Win32ProcessKeyboardMessage(&kbd_controller->LeftShoulder, is_down);
                    }
                    else if(vkcode == 'E') 
                    {
                        Win32ProcessKeyboardMessage(&kbd_controller->RightShoulder, is_down);
                    }
                    else if(vkcode == VK_UP) 
                    {
                        Win32ProcessKeyboardMessage(&kbd_controller->ActionUp, is_down);
                    }
                    else if(vkcode == VK_DOWN) 
                    {
                        Win32ProcessKeyboardMessage(&kbd_controller->ActionDown, is_down);
                    }
                    else if(vkcode == VK_RIGHT) 
                    {
                        Win32ProcessKeyboardMessage(&kbd_controller->ActionRight, is_down);
                    }
                    else if(vkcode == VK_LEFT) 
                    {
                        Win32ProcessKeyboardMessage(&kbd_controller->ActionLeft, is_down);
                    }
                    else if(vkcode == VK_SPACE) 
                    {
                        Win32ProcessKeyboardMessage(&kbd_controller->Start, is_down);
                    }
                    else if(vkcode == VK_BACK)
                    {
                        Win32ProcessKeyboardMessage(&kbd_controller->Back, is_down);
                    }
                    else if(vkcode == VK_ESCAPE) 
                    {
                        Running = false;
                    }
                }

                bool32 alt_key_was_down = (message.lParam & (1 << 29));
                if((vkcode == VK_F4) && alt_key_was_down) 
                {
                    Running = false;
                }
            } break;

            default:
            {
                TranslateMessage(&message);
                DispatchMessage(&message);
            } break;
        }
    }
}

global_variable int64 PerfCountFrequency;

// get realtime clock
inline LARGE_INTEGER Win32GetWallClock()
{
    LARGE_INTEGER end_counter;
    QueryPerformanceCounter(&end_counter);
    return end_counter;
}

// get elapsed seconds
inline real32 Win32GetSecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end)
{
    real32 seconds_elapsed_for_work = (real32)(end.QuadPart - start.QuadPart) / (real32)PerfCountFrequency;
    return seconds_elapsed_for_work;
}

//
// ENTRY POINT
//

// the main entry point for this program
int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCode)
{
    // timer stuff
    LARGE_INTEGER perf_count_frequency_result;
    QueryPerformanceFrequency(&perf_count_frequency_result);
    PerfCountFrequency = perf_count_frequency_result.QuadPart;

    // set windows scheduler granularity to 1ms 
    // so the sleep at end can be more granular
    UINT desired_scheuler_ms = 1;
    bool32 sleep_is_granular = (timeBeginPeriod(desired_scheuler_ms) == TIMERR_NOCANDO);
    
    Win32LoadXInput();
    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);
    
    // window class
    WNDCLASS window_class = {};
    window_class.style = CS_VREDRAW|CS_HREDRAW|CS_OWNDC;
    window_class.lpfnWndProc = Win32MainWindowCallback; 
    window_class.hInstance = instance;
    window_class.lpszClassName = "CPP Graphics Engine Window Class";
    // maybe add icon

// define to make constant
#define audio_latency_frames 3
#define monitor_refresh_hz 60
#define application_update_hz  (monitor_refresh_hz / 2)
    real32 target_seconds_per_frame = 1.0f / (real32)application_update_hz;

    // register the window
    if(RegisterClassA(&window_class)) {
        HWND window = CreateWindowExA(
            0,
            window_class.lpszClassName,
            "CPP Graphics Engine",
            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            instance,
            0);

        if(window) {

            HDC device_context = GetDC(window);

            // graphics test
            int x_off = 0;
            int y_off = 0;

            // sound init
            win32_sound_output sound_output = {};

            sound_output.SamplesPerSecond = 48000;
            sound_output.ToneHz = 256;
            sound_output.ToneVolume = 3000;
            sound_output.WavePeriod = sound_output.SamplesPerSecond/sound_output.ToneHz;
            sound_output.BytesPerSample = sizeof(int16)*2;
            sound_output.SecondaryBufferSize = sound_output.SamplesPerSecond*sound_output.BytesPerSample;
            sound_output.LatencySampleCount = audio_latency_frames * (sound_output.SamplesPerSecond / application_update_hz);

            Win32InitDSound(window, sound_output.SamplesPerSecond, sound_output.SecondaryBufferSize);
            Win32ClearBuffer(&sound_output);
            SecondaryBuffer->Play(0,0,DSBPLAY_LOOPING);

            int16 *samples = (int16 *)VirtualAlloc(0, sound_output.SecondaryBufferSize,
                                                   MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

#if APPLICATION_INTERNAL
            LPVOID base_address = (LPVOID)Terabytes(2);
#else
            LPVOID base_address = 0;
#endif

            application_memory app_memory = {};
            app_memory.PermanentStorageSize = Megabytes(64);
            app_memory.TransientStorageSize = Gigabytes(1);

            // TODO: Handle various memory footprints (USING SYSTEM METRICS)
            uint64 total_size = app_memory.PermanentStorageSize + app_memory.TransientStorageSize;
            app_memory.PermanentStorage = VirtualAlloc(base_address, (size_t)total_size,
                                                       MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            app_memory.TransientStorage = ((uint8 *)app_memory.PermanentStorage +
                                           app_memory.PermanentStorageSize);

            if(samples && app_memory.PermanentStorage && app_memory.TransientStorage)
            {
                application_input input[2] = {};
                application_input *new_input = &input[0];
                application_input *old_input = &input[1]; 

                Running = true;

                int debug_time_marker_idx = 0;
                win32_debug_time_marker debug_time_markers[application_update_hz / 2] = {0};

                // more timer stuff
                LARGE_INTEGER last_counter = Win32GetWallClock();
                uint64 last_cycle_count = __rdtsc();

                // sound debug
                DWORD last_play_cursor = 0;
                bool32 is_sound_valid = false;

                // main loop
                while(Running) 
                {
                    // process messages
                    application_controller_input *old_kbd_controller = GetController(old_input, 0);
                    application_controller_input *new_kbd_controller = GetController(new_input, 0);
                    application_controller_input zero_controller = {};

                    *new_kbd_controller = zero_controller;
                    new_kbd_controller->IsConnected = true;
                    for (int button_idx = 0; button_idx < ArrayCount(new_kbd_controller->Buttons); ++button_idx)
                    {
                        new_kbd_controller->Buttons[button_idx].EndedDown =
                            old_kbd_controller->Buttons[button_idx].EndedDown;
                    }

                    Win32ProcessPendingMessages(new_kbd_controller);

                    // application input
                    DWORD max_controller_count = XUSER_MAX_COUNT; // adjusted for keyboard at 0 idx
                    if(max_controller_count > (ArrayCount(new_input->Controllers)) - 1)
                    {
                        max_controller_count = (ArrayCount(new_input->Controllers)) - 1;
                    }
                    
                    for (DWORD controller_idx = 0;
                        controller_idx < max_controller_count;
                        ++controller_idx)
                    {
                        DWORD our_controller_idx = controller_idx + 1; // skip 0 idx for keyboard
                        application_controller_input *old_controller = GetController(old_input, our_controller_idx);
                        application_controller_input *new_controller = GetController(new_input, our_controller_idx);
                        
                        XINPUT_STATE controller_state;
                        if(XInputGetState(controller_idx, &controller_state) == ERROR_SUCCESS)
                        {
                            new_controller->IsConnected = true;

                            // NOTE: This controller is plugged in
                            // TODO: See if ControllerState.dwPacketNumber increments too rapidly
                            XINPUT_GAMEPAD *pad = &controller_state.Gamepad;

                            // thumb stick and deadzone
                            new_controller->IsAnalog = true;
                            new_controller->StickAverageX = Win32ProcessXInputStickPosition(
                                pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                            new_controller->StickAverageY = Win32ProcessXInputStickPosition(
                                pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                            
                            // dpad
                            if(pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
                            {
                                new_controller->StickAverageY = 1.0f;
                            }
                            if(pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
                            {
                                new_controller->StickAverageY = -1.0f;
                            }
                            if(pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
                            {
                                new_controller->StickAverageX = -1.0f;
                            }
                            if(pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
                            {
                                new_controller->StickAverageX = 1.0f;
                            }

                            // sticks are turned into single full direction presses
                            // the stick averageX/Y will still let you do granular movement
                            // if desired
                            real32 threshold = 0.5;
                            Win32ProcessXInputDigitalButton(
                                (new_controller->StickAverageX < -threshold) ? 1 : 0,
                                &old_controller->MoveLeft, 1,
                                &new_controller->MoveLeft);
                            Win32ProcessXInputDigitalButton(
                                (new_controller->StickAverageX > threshold) ? 1 : 0,
                                &old_controller->MoveLeft, 1,
                                &new_controller->MoveLeft);
                            Win32ProcessXInputDigitalButton(
                                (new_controller->StickAverageY < -threshold) ? 1 : 0,
                                &old_controller->MoveLeft, 1,
                                &new_controller->MoveLeft);
                            Win32ProcessXInputDigitalButton(
                                (new_controller->StickAverageY > threshold) ? 1 : 0,
                                &old_controller->MoveLeft, 1,
                                &new_controller->MoveLeft);

                            Win32ProcessXInputDigitalButton(pad->wButtons,
                                                            &old_controller->ActionDown, XINPUT_GAMEPAD_A,
                                                            &new_controller->ActionDown);
                            Win32ProcessXInputDigitalButton(pad->wButtons,
                                                            &old_controller->ActionRight, XINPUT_GAMEPAD_B,
                                                            &new_controller->ActionRight);
                            Win32ProcessXInputDigitalButton(pad->wButtons ,
                                                            &old_controller->ActionLeft, XINPUT_GAMEPAD_X,
                                                            &new_controller->ActionLeft);
                            Win32ProcessXInputDigitalButton(pad->wButtons,
                                                            &old_controller->ActionUp, XINPUT_GAMEPAD_Y,
                                                            &new_controller->ActionUp);
                            Win32ProcessXInputDigitalButton(pad->wButtons,
                                                            &old_controller->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER,
                                                            &new_controller->LeftShoulder);
                            Win32ProcessXInputDigitalButton(pad->wButtons,
                                                            &old_controller->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER,
                                                            &new_controller->RightShoulder);

                            Win32ProcessXInputDigitalButton(pad->wButtons,
                                                            &old_controller->Start, XINPUT_GAMEPAD_START,
                                                            &new_controller->Start);
                            Win32ProcessXInputDigitalButton(pad->wButtons,
                                                            &old_controller->Back, XINPUT_GAMEPAD_BACK,
                                                            &new_controller->Back);
                        }
                        else
                        {
                            // NOTE: The controller is not available
                            new_controller->IsConnected = false;
                        }
                    }

                    DWORD byte_to_lock = 0;
                    DWORD target_cursor = 0;
                    DWORD bytes_to_write = 0;
                    // TODO: Tighten up sound logic so that we know where we should be
                    // writing to and can anticipate the time spent in the application update.
                    if(is_sound_valid)
                    {
                        byte_to_lock = ((sound_output.RunningSampleIndex*sound_output.BytesPerSample) %
                                            sound_output.SecondaryBufferSize);

                        target_cursor =
                            ((last_play_cursor +
                            (sound_output.LatencySampleCount*sound_output.BytesPerSample)) %
                            sound_output.SecondaryBufferSize);

                        if(byte_to_lock > target_cursor)
                        {
                            bytes_to_write = (sound_output.SecondaryBufferSize - byte_to_lock);
                            bytes_to_write += target_cursor;
                        }
                        else
                        {
                            bytes_to_write = target_cursor - byte_to_lock;
                        }
                    }

                    // render and update
                    application_sound_output_buffer sound_buffer = {};
                    sound_buffer.SamplesPerSecond = sound_output.SamplesPerSecond;
                    sound_buffer.SampleCount = bytes_to_write / sound_output.BytesPerSample;
                    sound_buffer.Samples = samples;

                    offscreen_graphics_buffer b = {};
                    b.Memory = GlobalBackBuffer.Memory;
                    b.Width = GlobalBackBuffer.Width; 
                    b.Height = GlobalBackBuffer.Height;
                    b.Pitch = GlobalBackBuffer.Pitch; 
                    ApplicationUpdateAndRender(&app_memory, new_input, &b, &sound_buffer);

                    if(is_sound_valid)
                    {
#if APPLICATION_INTERNAL
                        DWORD play_cursor;
                        DWORD write_cursor;
                        SecondaryBuffer->GetCurrentPosition(&play_cursor, &write_cursor);
                    char text_buffer[256];
                        _snprintf_s(text_buffer, sizeof(text_buffer),
                                    "LPC:%u BTL:%u TC:%u BTW:%u - PC:%u WC:%u\n",
                                    last_play_cursor, byte_to_lock, target_cursor, bytes_to_write,
                                    play_cursor, write_cursor);
                        OutputDebugStringA(text_buffer);
#endif   

                        // we render the application and then fill the sound buffer with the 
                        // buffer data passed back from the render loop
                        Win32FillSoundBuffer(&sound_output, byte_to_lock, bytes_to_write, &sound_buffer);
                    }

                    // timer stuff
                    LARGE_INTEGER work_counter = Win32GetWallClock();
                    real32 work_seconds_elapsed = Win32GetSecondsElapsed(last_counter, work_counter);

                    real32 seconds_elapsed_for_frame = work_seconds_elapsed;
                    if(seconds_elapsed_for_frame < target_seconds_per_frame)
                    {
                        while(seconds_elapsed_for_frame < target_seconds_per_frame)
                        {
                            if(sleep_is_granular)
                            {
                                DWORD sleep_ms = (DWORD)(1000.0f * (target_seconds_per_frame - seconds_elapsed_for_frame));
                                if(sleep_ms > 0)
                                {
                                    Sleep(sleep_ms);
                                }
                            }
                            seconds_elapsed_for_frame = Win32GetSecondsElapsed(last_counter, Win32GetWallClock());
                        }
                    }
                    else
                    {
                        // TODO: missed frame rate!
                        // TODO: logging
                    }

                    LARGE_INTEGER end_counter = Win32GetWallClock();
                    real64 ms_per_frame = 1000.0f * Win32GetSecondsElapsed(last_counter, end_counter);
                    last_counter = end_counter;

                    win32_window_dimension dim = Win32GetWindowDimension(window);

#if APPLICATION_INTERNAL // NOTE: debug sound code
                    Win32DebugSyncSound(
                        &GlobalBackBuffer, ArrayCount(debug_time_markers), 
                        debug_time_markers, &sound_output, target_seconds_per_frame);
#endif

                    Win32DisplayBufferInWindow(&GlobalBackBuffer, device_context, 
                                                    dim.Width, dim.Height);

                    DWORD play_cursor;
                    DWORD write_cursor;
                    if(SecondaryBuffer->GetCurrentPosition(&play_cursor, &write_cursor) == DS_OK)
                    {
                        last_play_cursor = play_cursor;
                        if(!is_sound_valid)
                        {
                            sound_output.RunningSampleIndex = write_cursor / sound_output.BytesPerSample;
                            is_sound_valid = true;
                        }
                    }
                    else
                    {
                        is_sound_valid = false;
                    }
#if APPLICATION_INTERNAL //NOTE: debug sound code
                    {
                        Assert(debug_time_marker_idx < ArrayCount(debug_time_markers))
                        win32_debug_time_marker *marker = &debug_time_markers[debug_time_marker_idx++];
                        if(debug_time_marker_idx == ArrayCount(debug_time_markers))
                        {
                            debug_time_marker_idx = 0;
                        }
                        marker->PlayCursor = play_cursor;
                        marker->WriteCursor = write_cursor;
                    }
#endif

                    application_input *temp = new_input;
                    new_input = old_input;
                    old_input = temp;

                    uint64 end_cycle_count = __rdtsc();
                    uint64 cycles_elapsed = end_cycle_count - last_cycle_count;
                    last_cycle_count = end_cycle_count; 

#if 0

                    real64 fps = 0.0f; // (real64)PerfCountFrequency / (real64)counter_elapsed;
                    real64 mcpf = ((real64)cycles_elapsed / (1000.0f * 1000.0f));

                    char fps_buffer[256];
                    sprintf_s(fps_buffer, "%.02fms/f,  %.02ff/s,  %.02fmc/f\n", ms_per_frame, fps, mcpf);
                    OutputDebugStringA(fps_buffer);
#endif
                }
            }
            else 
            {
                // TODO: logging for memory
            }
        }
        else 
        {
            // TODO: Logging
        }
    }
    else 
    {
        // TODO: Logging
    }

    return 0;
}
