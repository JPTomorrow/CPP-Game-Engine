#include <windows.h>
#include <stdint.h>

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

global_variable bool Running;

global_variable BITMAPINFO BitMapInfo;
global_variable void *BitMapMemory;
global_variable int BitMapWidth;
global_variable int BitMapHeight;
global_variable int BytesPerPixel = 4;

internal void RenderWeirdGradient(int x_offset, int y_offset)
{
    // all the casting is for byte alignment in windows
    // we are going to derefrence the pixel memory and write a color value to it.
    int width = BitMapWidth;

    int pitch = width * BytesPerPixel;
    uint8 *row = (uint8 *)BitMapMemory;
    for (int y = 0; y < BitMapHeight; ++y) 
    {
        uint32 *pixel = (uint32 *)row;
        for (int x = 0; x < BitMapWidth; ++x) 
        {
            /*
                4 bytes ... pixel, pixel+1 etc 
                pixel in register: 0x xxRRGGBB
                pixel in memory: BB GG RR xx -> REVERSED! little endian arch.
                xx is padding
            */

            uint8 blue = (x + x_offset);
            uint8 green = (y + y_offset);
            *pixel++ = (green << 8) | blue;
        }

        row += pitch;
    }
}
internal void Win32ResizeDIBSection(int width, int height) 
{

    // TODO: If you run into bliting problems later remeber 
    // it was in episode 3 where you didnt get a window that blited entirely black
    if(BitMapMemory) {
        VirtualFree(BitMapMemory, 0, MEM_RELEASE);
    }

    BitMapHeight = height;
    BitMapWidth = width;

    BitMapInfo.bmiHeader.biSize = sizeof(BitMapInfo.bmiHeader);
    BitMapInfo.bmiHeader.biWidth = BitMapWidth;
    BitMapInfo.bmiHeader.biHeight = -BitMapHeight; // for top-down row drawing must be negative
    BitMapInfo.bmiHeader.biPlanes = 1;
    BitMapInfo.bmiHeader.biBitCount = 32;
    BitMapInfo.bmiHeader.biCompression = BI_RGB;

    int bit_map_memory_size = (BitMapWidth * BitMapHeight) * BytesPerPixel;

    BitMapMemory = VirtualAlloc(0, bit_map_memory_size, MEM_COMMIT, PAGE_READWRITE);

    // probably clear to black
}

internal void Win32UpdateWindow(HDC device_context, RECT *client_rect, int x, int y, int width, int height)
{
    int window_width = client_rect->right - client_rect->left;
    int window_height = client_rect->bottom - client_rect->top;

    StretchDIBits(device_context,
                  /*
                  x, y, width, height,
                  x, y, width, height,
                  */
                  0, 0, BitMapWidth, BitMapHeight,
                  0, 0, window_width, window_height,
                  BitMapMemory,
                  &BitMapInfo,
                  DIB_RGB_COLORS, SRCCOPY);
}

//
// Message Pump
//
LRESULT CALLBACK Win32MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    switch(message) {
        case WM_SIZE:
        {
            RECT client_rect;
            GetClientRect(window, &client_rect);

            int width = client_rect.right - client_rect.left;
            int height = client_rect.bottom - client_rect.top;

            Win32ResizeDIBSection(width, height);
        } break;
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
        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC device_context = BeginPaint(window, &paint);

            int x = paint.rcPaint.left;
            int y = paint.rcPaint.top;
            int width = paint.rcPaint.right - paint.rcPaint.left;
            int height = paint.rcPaint.bottom - paint.rcPaint.top;

            RECT client_rect;
            GetClientRect(window, &client_rect);

            Win32UpdateWindow(device_context, &client_rect, x, y, width, height);
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
    WNDCLASS window_class = {};
    window_class.lpfnWndProc = Win32MainWindowCallback; 
    window_class.hInstance = instance;
    window_class.lpszClassName = "CPP Game Engine Window Class";

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
            int x_off = 0;
                int y_off = 0;
            Running = true;
            while(Running) {
                MSG message;
                while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
                    if(message.message == WM_QUIT)
                        Running = false;
                    
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }

                RenderWeirdGradient(x_off,y_off);

                HDC device_context = GetDC(window);

                RECT client_rect;
                GetClientRect(window, &client_rect);
             
                int window_width = client_rect.right - client_rect.left;
                int window_height = client_rect.bottom - client_rect.top;
                Win32UpdateWindow(device_context, &client_rect, 0, 0, window_width, window_height);
                ReleaseDC(window, device_context);
                x_off += 1;
                y_off += 1;
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

