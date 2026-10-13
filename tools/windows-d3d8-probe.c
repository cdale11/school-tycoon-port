/* Minimal original-game-era Direct3D 8 device, clear, readback and present test. */
#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#include <windows.h>
#include <d3d8.h>

static WNDCLASSA window_class;
static D3DPRESENT_PARAMETERS parameters;
static IDirect3D8 *d3d;
static IDirect3DDevice8 *device;
static IDirect3DSurface8 *buffer;
static HWND window;
static HMODULE d3d_module;

static void finish(const char *message, HRESULT error, DWORD status) {
    HANDLE output = CreateFileA("d3d8-result.txt", GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD written;
    if (output == INVALID_HANDLE_VALUE) ExitProcess(2);
    WriteFile(output, message, (DWORD)lstrlenA(message), &written, NULL);
    if (status) {
        char code[] = "HRESULT: 0x00000000\r\n";
        const char digits[] = "0123456789abcdef";
        for (int i = 0; i < 8; i++) code[11 + i] = digits[((DWORD)error >> ((7 - i) * 4)) & 15];
        WriteFile(output, code, sizeof(code) - 1, &written, NULL);
    }
    CloseHandle(output);
    if (buffer) IDirect3DSurface8_Release(buffer);
    if (device) IDirect3DDevice8_Release(device);
    if (d3d) IDirect3D8_Release(d3d);
    if (d3d_module) FreeLibrary(d3d_module);
    if (window) DestroyWindow(window);
    ExitProcess(status);
}

void probe_start(void) {
    window_class.lpfnWndProc = DefWindowProcA;
    window_class.hInstance = GetModuleHandleA(NULL);
    window_class.lpszClassName = "SchoolTycoonD3D8Probe";
    if (!RegisterClassA(&window_class)) finish("RegisterClass: FAIL\r\n", HRESULT_FROM_WIN32(GetLastError()), 1);
    window = CreateWindowExA(0, window_class.lpszClassName, "School Tycoon Direct3D 8 Probe",
        WS_OVERLAPPEDWINDOW, 0, 0, 240, 180, NULL, NULL, window_class.hInstance, NULL);
    if (!window) finish("CreateWindow: FAIL\r\n", HRESULT_FROM_WIN32(GetLastError()), 1);
    ShowWindow(window, SW_SHOWNORMAL);
    d3d_module = LoadLibraryA("d3d8.dll");
    if (!d3d_module) finish("LoadLibrary d3d8: FAIL\r\n", HRESULT_FROM_WIN32(GetLastError()), 1);
    typedef IDirect3D8 *(WINAPI *create8_fn)(UINT);
    create8_fn create8 = (create8_fn)(void *)GetProcAddress(d3d_module, "Direct3DCreate8");
    if (!create8) finish("Direct3DCreate8 export: FAIL\r\n", HRESULT_FROM_WIN32(GetLastError()), 1);
    d3d = create8(D3D_SDK_VERSION);
    if (!d3d) finish("Direct3DCreate8: FAIL\r\n", E_FAIL, 1);
    parameters.BackBufferWidth = 128;
    parameters.BackBufferHeight = 128;
    parameters.BackBufferFormat = D3DFMT_X8R8G8B8;
    parameters.BackBufferCount = 1;
    parameters.SwapEffect = D3DSWAPEFFECT_COPY;
    parameters.hDeviceWindow = window;
    parameters.Windowed = TRUE;
    parameters.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
    HRESULT hr = IDirect3D8_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
        window, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &parameters, &device);
    if (FAILED(hr)) finish("CreateDevice HAL: FAIL\r\n", hr, 1);
    const DWORD color = D3DCOLOR_XRGB(32, 128, 224);
    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, color, 1.0f, 0);
    if (FAILED(hr)) finish("Clear: FAIL\r\n", hr, 1);
    hr = IDirect3DDevice8_GetBackBuffer(device, 0, D3DBACKBUFFER_TYPE_MONO, &buffer);
    if (FAILED(hr)) finish("GetBackBuffer: FAIL\r\n", hr, 1);
    D3DLOCKED_RECT locked;
    hr = IDirect3DSurface8_LockRect(buffer, &locked, NULL, D3DLOCK_READONLY);
    if (FAILED(hr)) finish("LockRect: FAIL\r\n", hr, 1);
    DWORD pixel = *(const DWORD *)locked.pBits;
    IDirect3DSurface8_UnlockRect(buffer);
    if ((pixel & 0x00ffffff) != (color & 0x00ffffff)) finish("D3D8 pixel readback: FAIL\r\n", E_FAIL, 1);
    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    if (FAILED(hr)) finish("Present: FAIL\r\n", hr, 1);
    finish("ST-D3D8-PASS\r\n", S_OK, 0);
}
