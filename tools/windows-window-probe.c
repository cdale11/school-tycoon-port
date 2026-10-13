/* A Windows x86 GUI/GDI test, not a Direct3D or gameplay test. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static const COLORREF probe_color = RGB(32, 128, 224);
static WNDCLASSA window_class;

static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == WM_PAINT) {
        PAINTSTRUCT paint;
        HDC dc = BeginPaint(window, &paint);
        HBRUSH brush = CreateSolidBrush(probe_color);
        FillRect(dc, &paint.rcPaint, brush);
        DeleteObject(brush);
        EndPaint(window, &paint);
        return 0;
    }
    return DefWindowProcA(window, message, wparam, lparam);
}

static void finish(const char *message, DWORD status) {
    HANDLE output = CreateFileA("window-result.txt", GENERIC_WRITE, 0, NULL,
                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD written = 0;
    if (output == INVALID_HANDLE_VALUE) ExitProcess(2);
    if (!WriteFile(output, message, (DWORD)lstrlenA(message), &written, NULL)) {
        CloseHandle(output);
        ExitProcess(2);
    }
    CloseHandle(output);
    ExitProcess(status);
}

void probe_start(void) {
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = GetModuleHandleA(NULL);
    window_class.lpszClassName = "SchoolTycoonWindowProbe";
    if (!RegisterClassA(&window_class)) finish("RegisterClass: FAIL\r\n", 1);
    HWND window = CreateWindowExA(0, window_class.lpszClassName,
        "School Tycoon Window Probe", WS_OVERLAPPEDWINDOW,
        0, 0, 240, 160, NULL, NULL, window_class.hInstance, NULL);
    if (!window) finish("CreateWindow: FAIL\r\n", 1);
    ShowWindow(window, SW_SHOWNORMAL);
    UpdateWindow(window);
    HDC dc = GetDC(window);
    COLORREF pixel = GetPixel(dc, 8, 8);
    ReleaseDC(window, dc);
    DestroyWindow(window);
    if (pixel != probe_color) finish("Windows GDI pixel readback: FAIL\r\n", 1);
    finish("ST-WINDOW-PASS\r\n", 0);
}
