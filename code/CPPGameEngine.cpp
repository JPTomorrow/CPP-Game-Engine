#include <windows.h>

LRESULT CALLBACK MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    switch(message) {
        case WM_SIZE:
        {
            OutputDebugStringA("WM_SIZE\n");
        } break;
        case WM_DESTROY:
        {
            OutputDebugStringA("WM_DESTROY\n");
        } break;
        case WM_CLOSE:
        {   
            OutputDebugStringA("WM_CLOSE\n");
        } break;
        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        default:
        {
            // OutputDebugStringA("default\n");
            result = DefWindowProc(window, message, wParam, lParam);
        } break;
    }

    return result;
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCode)
{
    WNDCLASSA window_class = {};
    window_class.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    window_class.lpfnWndProc = MainWindowCallback;
    window_class.hInstance = instance;
    window_class.lpszClassName = "CPP Game Engine Window Class";

    // register the window
    if(RegisterClassA(&window_class)) {
        HWND window_handle = CreateWindowExA(
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

        if(window_handle) {
            MSG message;
            for (;;) {
                BOOL message_result = GetMessageA(&message, 0, 0, 0);
                if(message_result > 0) {
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }
                else {
                    break;
                }
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

