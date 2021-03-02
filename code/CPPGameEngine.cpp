#include <windows.h>
#include <stdint.h>
#include <Xinput.h>
#include <dsound.h>

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

struct win32_offscreen_buffer{
    /*
        4 bytes ... pixel, pixel+1 etc 
        pixel in register: 0x xxRRGGBB
        pixel in memory: BB GG RR xx -> REVERSED! little endian arch.
        xx is padding
    */
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

struct win32_window_dimension
{
    int Width;
    int Height;
};

global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackBuffer;

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

//
// load the XInput library functions required or 
// passthrough on a the stub in library doesnt exist
//
internal void Win32LoadXInput() {
    HMODULE x_input_library = LoadLibrary("xinput1_4.dll");
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

//
// Direct Sound
//

global_variable IDirectSoundBuffer *SecondaryBuffer;

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(dsound_create);


internal void Win32InitDSound(HWND window, int32 samples_per_sound, int32 buffer_size) { 
    // intialize the sound and start playing

    // load the library
    HMODULE dsound_library = LoadLibrary("dsound.dll");

    if(dsound_library) {

        // get direct sound object
        dsound_create *DirectSoundCreate = (dsound_create *)GetProcAddress(dsound_library, "DirectSoundCreate");
        IDirectSound *dsound; 

        if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &dsound, 0))) {

            WAVEFORMATEX wave_format = {};
            wave_format.wFormatTag = WAVE_FORMAT_PCM;
            wave_format.nChannels = 2;
            wave_format.wBitsPerSample = 16;
            wave_format.cbSize = 0;
            wave_format.nSamplesPerSec = samples_per_sound;
            wave_format.nBlockAlign = (wave_format.nChannels * wave_format.wBitsPerSample) / 8;
            wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nBlockAlign;

            if(SUCCEEDED(dsound->SetCooperativeLevel(window, DSSCL_PRIORITY))) {
                
                // create a primary buffer
                DSBUFFERDESC buffer_description = {};
                buffer_description.dwFlags = DSBCAPS_PRIMARYBUFFER;
                buffer_description.dwSize = sizeof(buffer_description);

                IDirectSoundBuffer *primary_buffer;

                if(SUCCEEDED(dsound->CreateSoundBuffer(&buffer_description, &primary_buffer, 0))) {
                    
                    HRESULT error = primary_buffer->SetFormat(&wave_format);
                    if(SUCCEEDED(error)) {
                        // format has been set
                        OutputDebugStringA("Primary buffer format was set\n");
                    }
                    else {
                        // TODO: diagnostic
                    }
                }
            }
            else {
                // TODO: diagnostic
            }
            
            // create secondary buffer
            DSBUFFERDESC buffer_description = {};
            buffer_description.dwFlags = 0;
            buffer_description.dwSize = sizeof(buffer_description);
            buffer_description.dwBufferBytes = buffer_size;
            buffer_description.lpwfxFormat = &wave_format;

            if(SUCCEEDED(dsound->CreateSoundBuffer(&buffer_description, &SecondaryBuffer, 0))) {
                OutputDebugStringA("Secondary buffer created\n");
            }
            else {
                // TODO: diagnostic
            }

            

            // start playing sound
        }
        else {
            // TODO: diagnostics
        }
    }
    else {
        // TODO: diagnostics
    }
}

//
// Graphics
//

internal win32_window_dimension Win32GetWindowDimension(HWND window)
{
    win32_window_dimension Result;
    
    RECT ClientRect;
    GetClientRect(window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return(Result);
}

internal void RenderWeirdGradient(win32_offscreen_buffer *buffer, int x_offset, int y_offset)
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

// 
// Resize the window buffer that is passed in
internal void Win32ResizeDIBSection(win32_offscreen_buffer *buffer, int width, int height) 
{

    // TODO: If you run into bliting problems later remeber 
    // it was in episode 3 where you didnt get a window that blited entirely black
    if(buffer->Memory) {
        VirtualFree(buffer->Memory, 0, MEM_RELEASE);
    }

    buffer->Width = width;
    buffer->Height = height;
    int bytes_per_pixel = 4;

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

//
// diplay the passed buffer to the screen
//
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

//
// Message Pump
//
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
            uint32 vkcode = wParam;
            bool was_down = (lParam & (1 << 30)) != 0;
            bool is_down =  (lParam & (1 << 31)) == 0;
            // eat key repeats
            if(is_down == was_down)
                break;

            if(vkcode == 'W') {

            }
            else if(vkcode == 'A') {

            }
            else if(vkcode == 'S') {

            }
            else if(vkcode == 'D') {

            }
            else if(vkcode == VK_UP) {

            }
            else if(vkcode == VK_DOWN) {

            }
            else if(vkcode == VK_RIGHT) {

            }
            else if(vkcode == VK_LEFT) {

            }
            else if(vkcode == VK_ESCAPE) {
                OutputDebugStringA("Escape: ");
                OutputDebugStringA(is_down ? "IsDown" : "");
                OutputDebugStringA((was_down ? "WasDown" : ""));
                OutputDebugStringA("\n");
            }
            else if(vkcode == VK_SPACE) {

            }

            bool32 alt_key_was_down = (lParam & (1 << 29));
            if(vkcode == VK_F4 && alt_key_was_down) {
                Running = false;
            }
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

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCode)
{
    Win32LoadXInput();

    WNDCLASS window_class = {};

    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

    window_class.style = CS_VREDRAW|CS_HREDRAW|CS_OWNDC;
    window_class.lpfnWndProc = Win32MainWindowCallback; 
    window_class.hInstance = instance;
    window_class.lpszClassName = "CPP Game Engine Window Class";
    // maybe add icon

    // register the window
    if(RegisterClassA(&window_class)) {
        HWND window = CreateWindowExA(
            0,
            window_class.lpszClassName,
            "CPP Game Engine",
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

            // sound test
            int samples_per_second = 48000;
            int tone_hz = 256;
            int running_sample_idx = 0;
            int square_wave_period = samples_per_second / tone_hz;
            int half_square_wave_period = square_wave_period / 2;
            int bytes_per_sample = sizeof(int16) * 2;
            int secondary_buffer_size = samples_per_second * bytes_per_sample;
            int tone_volume = 1000;

            Win32InitDSound(window, samples_per_second, secondary_buffer_size);
            SecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            Running = true;
            while(Running) {
                MSG message;
                while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
                     if(message.message == WM_QUIT)
                        Running = false;
                    
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }

                // xinput controller
                for (DWORD controller_idx = 0; controller_idx < XUSER_MAX_COUNT; ++controller_idx) {
                    XINPUT_STATE controller_state;
                    if(XInputGetState(controller_idx, &controller_state) == ERROR_SUCCESS) {
                        XINPUT_GAMEPAD *pad = &controller_state.Gamepad;

                        bool up =               (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool down =             (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool left =             (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool right =            (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool start =            (pad->wButtons & XINPUT_GAMEPAD_START);
                        bool back =             (pad->wButtons & XINPUT_GAMEPAD_BACK);
                        bool left_shoulder =    (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool right_shoulder =   (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        bool a_button =         (pad->wButtons & XINPUT_GAMEPAD_A);
                        bool b_button =         (pad->wButtons & XINPUT_GAMEPAD_B);
                        bool x_button =         (pad->wButtons & XINPUT_GAMEPAD_X);
                        bool y_button =         (pad->wButtons & XINPUT_GAMEPAD_Y);

                        int16 stick_x = pad->sThumbLX;
                        int16 stick_y = pad->sThumbLY;

                        XINPUT_VIBRATION vib;
                        if(up) {
                            y_off -= 1;
                            
                            vib.wLeftMotorSpeed = 60000;
                            vib.wRightMotorSpeed = 60000;
                        }
                        else {
                            vib.wLeftMotorSpeed = 0;
                            vib.wRightMotorSpeed = 0;
                        }

                        XInputSetState(controller_idx, &vib);

                        if(down) y_off += 1;
                        if(left) x_off -= 1; 
                        if(right) x_off += 1;
                            
                        
                    }
                    else {
                        // controller is unavailable
                    }
                }

                DWORD play_cursor;
                DWORD write_cursor;
                if(SUCCEEDED(SecondaryBuffer->GetCurrentPosition(&play_cursor, &write_cursor)))
                {
                    DWORD byte_to_lock = running_sample_idx * bytes_per_sample % secondary_buffer_size;
                    DWORD bytes_to_write;
                    if(byte_to_lock > play_cursor) 
                    {
                        bytes_to_write = (secondary_buffer_size - byte_to_lock);
                        bytes_to_write += play_cursor;
                    }
                    else
                    {
                        bytes_to_write = play_cursor - byte_to_lock; 
                    }

                    void *region1;
                    DWORD region1_size;
                    void *region2;
                    DWORD region2_size;

                    if(SUCCEEDED(SecondaryBuffer->Lock(
                                        byte_to_lock, bytes_to_write,
                                        &region1, &region1_size,
                                        &region2, &region2_size, 0))) 
                    {
                        // assert that region sizes are valid
                        int16 *sample_out = (int16 *)region1;
                        DWORD region1_sample_count = region1_size / bytes_per_sample;
                        
                        for (DWORD sample_idx = 0; sample_idx < region1_sample_count; ++sample_idx)
                        {
                            int16 sample_value = ((running_sample_idx++ / half_square_wave_period) % 2) ? tone_volume : -tone_volume;
                            *sample_out++ = sample_value;
                            *sample_out++ = sample_value;
                        }

                        sample_out = (int16 *)region2;
                        DWORD region2_sample_count = region2_size / bytes_per_sample;

                        for(DWORD sample_idx = 0; sample_idx < region2_sample_count; ++sample_idx)
                        {
                            int16 sample_value = ((running_sample_idx++ / half_square_wave_period) % 2) ? tone_volume : -tone_volume;
                            *sample_out++ = sample_value;
                            *sample_out++ = sample_value;
                        }

                        SecondaryBuffer->Unlock(region1, region1_size, region2, region2_size);
                    }
                }

                RenderWeirdGradient(&GlobalBackBuffer, x_off,y_off);
                win32_window_dimension dim = Win32GetWindowDimension(window);
                Win32DisplayBufferInWindow(&GlobalBackBuffer, device_context, 
                                                dim.Width, dim.Height);
                ReleaseDC(window, device_context);
            }
        }
        else {
            // TODO: Logging
        }
    }
    else {
        // TODO: Logging
    }

    return 0;
}

