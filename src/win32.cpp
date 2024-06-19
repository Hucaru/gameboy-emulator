#include "win32.h"
#include <windows.h>
#include <cstdlib>
#include <cstdint>
#include <cstdio>

struct Frame {
    u32 width;
    u32 height;
    u32 *pixels;
};

struct Window {
    // Render
    Frame frame;
    BITMAPINFO frame_bitmap_info;
    HBITMAP frame_bitmap;
    HDC frame_device_context;

    Input_events input_events;
    HWND window_handle;
};

Input_events::STATE check_input(Input_events *events, Input_events::CODES code)
{
    return events->event[static_cast<int>(code)];
}

LRESULT CALLBACK 
WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // static Window *window = NULL;

    switch(msg)
    {
        case WM_CREATE: 
        {
            CREATESTRUCT *create_struct = (CREATESTRUCT*)lParam;
            Window* window = (Window*)create_struct->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)window);
        } break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        case WM_PAINT: 
        {
            LONG_PTR ptr = GetWindowLongPtr(hwnd, GWLP_USERDATA);
            Window *window = reinterpret_cast<Window*>(ptr);

            static PAINTSTRUCT paint;
            static HDC device_context;

            device_context = BeginPaint(hwnd, &paint);
            
            BitBlt(device_context, 
                paint.rcPaint.left, paint.rcPaint.top, 
                paint.rcPaint.right - paint.rcPaint.left, paint.rcPaint.bottom - paint.rcPaint.top, 
                window->frame_device_context,
                paint.rcPaint.left, paint.rcPaint.top,
                SRCCOPY);

            EndPaint(hwnd, &paint);
        } break;
        case WM_SIZE: 
        {
            LONG_PTR ptr = GetWindowLongPtr(hwnd, GWLP_USERDATA);
            Window *window = reinterpret_cast<Window*>(ptr);

            window->frame_bitmap_info.bmiHeader.biWidth = LOWORD(lParam);
            window->frame_bitmap_info.bmiHeader.biHeight = HIWORD(lParam);

            if (window->frame_bitmap) 
            {
                DeleteObject(window->frame_bitmap);
            }

            window->frame_bitmap = CreateDIBSection(NULL, 
                &window->frame_bitmap_info, 
                DIB_RGB_COLORS, 
                reinterpret_cast<void**>(&window->frame.pixels), 
                0, 0);

            SelectObject(window->frame_device_context, window->frame_bitmap);

            window->frame.width = LOWORD(lParam);
            window->frame.height = HIWORD(lParam);
        } break;
        case WM_KEYDOWN:
        {
            LONG_PTR ptr = GetWindowLongPtr(hwnd, GWLP_USERDATA);
            Window *window = reinterpret_cast<Window*>(ptr);

            Input_events::STATE button_state = lParam & (1 << 30) ? Input_events::STATE::HELD : Input_events::STATE::DOWN;

            if (wParam == VK_ESCAPE)
            {
                window->input_events.event[static_cast<int>(Input_events::CODES::ESC)] = button_state;
            }
            else if (wParam == VK_SPACE)
            {
                window->input_events.event[static_cast<int>(Input_events::CODES::SPACE)] = button_state;
            }
            else if (wParam == VK_RETURN)
            {
                window->input_events.event[static_cast<int>(Input_events::CODES::RETURN)] = button_state;
            }
            else if (wParam == VK_BACK)
            {
                window->input_events.event[static_cast<int>(Input_events::CODES::BACK)] = button_state;
            }
            else if (wParam == VK_LEFT)
            {
                window->input_events.event[static_cast<int>(Input_events::CODES::LEFT)] = button_state;
            }
            else if (wParam == VK_RIGHT)
            {
                window->input_events.event[static_cast<int>(Input_events::CODES::RIGHT)] = button_state;
            }
            else if (wParam == VK_UP)
            {
                window->input_events.event[static_cast<int>(Input_events::CODES::UP)] = button_state;
            }
            else if (wParam == VK_DOWN)
            {
                window->input_events.event[static_cast<int>(Input_events::CODES::DOWN)] = button_state;
            }
            else if (wParam >= 0x30 && wParam <= 0x39)
            {
                window->input_events.event[wParam - 0x30] = button_state;
            }
            else if (wParam >= 0x41 && wParam <= 0x5A)
            {
                window->input_events.event[wParam - 0x30 - 7] = button_state;
            }
        } break;
        case WM_KEYUP:
        {
            LONG_PTR ptr = GetWindowLongPtr(hwnd, GWLP_USERDATA);
            Window *window = reinterpret_cast<Window*>(ptr);

            if (wParam == VK_ESCAPE)
            {
                window->input_events.event[static_cast<int>(Input_events::CODES::ESC)] = Input_events::STATE::UP;
            }
            else if (wParam == VK_SPACE)
            {
                window->input_events.event[static_cast<int>(Input_events::CODES::SPACE)] = Input_events::STATE::UP;
            }
            else if (wParam == VK_RETURN)
            {
                window->input_events.event[static_cast<int>(Input_events::CODES::RETURN)] = Input_events::STATE::UP;
            }
            else if (wParam == VK_BACK)
            {
                window->input_events.event[static_cast<int>(Input_events::CODES::BACK)] = Input_events::STATE::UP;
            }
            else if (wParam == VK_LEFT)
            {
                window->input_events.event[static_cast<int>(Input_events::CODES::LEFT)] = Input_events::STATE::UP;
            }
            else if (wParam == VK_RIGHT)
            {
                window->input_events.event[static_cast<int>(Input_events::CODES::RIGHT)] = Input_events::STATE::UP;
            }
            else if (wParam == VK_UP)
            {
                window->input_events.event[static_cast<int>(Input_events::CODES::UP)] = Input_events::STATE::UP;
            }
            else if (wParam == VK_DOWN)
            {
                window->input_events.event[static_cast<int>(Input_events::CODES::DOWN)] = Input_events::STATE::UP;
            }
            if (wParam >= 0x30 && wParam <= 0x39)
            {
                window->input_events.event[wParam - 0x30] = Input_events::STATE::UP;
            }
            else if (wParam >= 0x41 && wParam <= 0x5A)
            {
                window->input_events.event[wParam - 0x30 - 7] = Input_events::STATE::UP;
            }
        } break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

const char window_class[] = "emulatorClass";

int CALLBACK 
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 
{
#if 1
    AllocConsole();
    freopen("CON", "w", stdout);
    freopen("CON", "r", stdin);
    char title[128];
    sprintf_s(title, "Attached to: %i", GetCurrentProcessId());
    SetConsoleTitleA(title);
#endif

    int argc = 1;
    char *arg = lpCmdLine;

    while(arg[0] != NULL)
    {
        while(arg[0] != NULL && arg[0] == ' ')
        {
            arg++;
        }

        char end_char = ' ';

        if (arg[0] == '"')
        {
            arg++;
            end_char = '"';
        }

        argc++;

        while(arg[0] != NULL & arg[0] != end_char)
        {
            arg++;
        }
        
        arg++;
    }

    char **argv = reinterpret_cast<char**>(malloc(argc * sizeof(char*)));
    int index = 1;
    arg = lpCmdLine;

    while(arg[0] != NULL)
    {
        while(arg[0] != NULL && arg[0] == ' ')
        {
            arg++;
        }

        char end_char = ' ';

        if (arg[0] == '"')
        {
            arg++;
            end_char = '"';
        }

        argv[index] = arg;
        index++;

        while(arg[0] != NULL & arg[0] != end_char)
        {
            arg++;
        }
        
        arg[0] = 0;
        arg++;
    }

    char filename[_MAX_PATH];
    
    GetModuleFileNameA(NULL, filename, _MAX_PATH);
    argv[0] = filename;

    WNDCLASSEXA wcex = {};
    wcex.cbSize = sizeof(wcex);
    wcex.style = 0 ;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
    wcex.lpszClassName = window_class;

    if (!RegisterClassExA(&wcex))
    {
        MessageBoxExA(NULL, "Failed to register window class", "Error", MB_ICONERROR | MB_OK, 0);
        return -1;
    }

    App app;
    app.window_title = "Window";
    app.window_width = CW_USEDEFAULT;
    app.window_height = CW_USEDEFAULT;         
    app.application = NULL;
    app.running = false;

    if (!init_application(argc, argv, &app) || !app.application) 
    {
        MessageBoxExA(NULL, "Failed to start application", "Error", MB_ICONERROR | MB_OK, 0);
        return -1;
    }

    RECT rect = {};
    rect.bottom = app.window_height;
    rect.right = app.window_width;

    if (!AdjustWindowRectEx(&rect, WS_TILED | WS_CAPTION, false, 0))
    {
        MessageBoxExA(NULL, "Failed to adjust window rectangle", "Error", MB_ICONERROR | MB_OK, 0);
        return -1;
    }

    Window window = {};
    window.frame_bitmap_info.bmiHeader.biSize = sizeof(window.frame_bitmap_info.bmiHeader);
    window.frame_bitmap_info.bmiHeader.biPlanes = 1;
    window.frame_bitmap_info.bmiHeader.biBitCount = 32;
    window.frame_bitmap_info.bmiHeader.biCompression = BI_RGB;
    window.frame_device_context = CreateCompatibleDC(0);

    HWND hwnd = CreateWindowExA(
        0,
        window_class,
        app.window_title,
        WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 
        rect.right - rect.left, rect.bottom - rect.top,
        NULL,
        NULL,
        hInstance,
        &window
        );

    if (!hwnd) 
    {
        MessageBoxExA(NULL, "Failed to create window", "Error", MB_ICONERROR | MB_OK, 0);
        return -1;
    }

    window.window_handle = hwnd;

    app.window_handle = &window;

    MSG msg = {};

    LARGE_INTEGER start_time, end_time;
    LARGE_INTEGER frequency;

    i64 delta_time = 0;

    while (app.running) 
    {
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) 
        {
            switch (msg.message)
            {
                case WM_QUIT:
                    app.running = false;
                    break;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (!app.running)
        {
            PostQuitMessage(0);
        }

        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&start_time);

        handle_input(&app, &window.input_events);
        update_application(&app, delta_time);
        ZeroMemory(&window.input_events.event, sizeof(window.input_events.event));
        render_application(&app, window.frame.pixels, window.frame.width, window.frame.height);

        QueryPerformanceCounter(&end_time);
        delta_time = end_time.QuadPart - start_time.QuadPart;
        delta_time *= 1e9; // nanosecond precision
        delta_time /= frequency.QuadPart;

        start_time = end_time;
    }

    return 0;
}

Window * 
create_window(u32 height, u32 width, char *title)
{
    Window *window = reinterpret_cast<Window*>(malloc(sizeof(Window)));

    window->frame_bitmap_info.bmiHeader.biSize = sizeof(window->frame_bitmap_info.bmiHeader);
    window->frame_bitmap_info.bmiHeader.biPlanes = 1;
    window->frame_bitmap_info.bmiHeader.biBitCount = 32;
    window->frame_bitmap_info.bmiHeader.biCompression = BI_RGB;
    window->frame_device_context = CreateCompatibleDC(0);

    RECT rect = {};
    rect.bottom = height;
    rect.right = width;

    if (!AdjustWindowRectEx(&rect, WS_TILED | WS_CAPTION, false, 0))
    {
        MessageBoxExA(NULL, "Failed to adjust window rectangle", "Error", MB_ICONERROR | MB_OK, 0);
        return NULL;
    }

    HWND hwnd = CreateWindowExA(
        0,
        window_class,
        title,
        WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 
        rect.right - rect.left, rect.bottom - rect.top,
        NULL,
        NULL,
        GetModuleHandle(NULL),
        window
        );

    if (!hwnd)
    {
        MessageBoxExA(NULL, "Failed to create window", "Error", MB_ICONERROR | MB_OK, 0);
        return NULL;
    }

    window->window_handle = hwnd;

    return window;
}

u32 *
window_get_frame(Window *window, u32 *width, u32 *height)
{
    *width = window->frame.width;
    *height = window->frame.height;
    return window->frame.pixels;
}

void
window_redraw(Window *window)
{
    // Trigger a window paint
    InvalidateRect(reinterpret_cast<HWND>(window->window_handle), NULL, false);
    UpdateWindow(reinterpret_cast<HWND>(window->window_handle));
}

void
update_window_title(void *handle, char *title)
{
    if (!SetWindowTextA(reinterpret_cast<HWND>(handle), title))
    {
        message_box("Error", "Unable to set window title");
    }
}

u8 * 
read_file(char *filename, uint64_t *file_size) 
{
    HANDLE handle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (!handle) 
    {
        return NULL;
    }

    *file_size = GetFileSize(handle, NULL);

    if (file_size == 0) 
    {
        CloseHandle(handle);
        return NULL;
    }

    u8 *file_data = reinterpret_cast<uint8_t*>(malloc(*file_size));
    DWORD bytes_read;

    if (!ReadFile(handle, file_data, *file_size, &bytes_read, NULL) || bytes_read != *file_size) 
    {
        CloseHandle(handle);
        return NULL;
    }

    CloseHandle(handle);
    return file_data;
}

void
message_box(char *title, char *msg)
{
    MessageBoxExA(NULL, msg, title, MB_OK, 0);
}

void
clear_console()
{
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

    COORD pos = { 0, 0};
    DWORD written;
    unsigned size;

    size = csbi.srWindow.Bottom - csbi.srWindow.Top + 1 * csbi.srWindow.Right - csbi.srWindow.Left + 1;;

    FillConsoleOutputCharacter(console, ' ', size, pos, &written);
    FillConsoleOutputAttribute(console, 0, size, pos, &written);
    SetConsoleCursorPosition(console, pos);
}