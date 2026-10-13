/* SPDX-License-Identifier: MIT
 * D3D8 textured quad/readback regression; also requires successful Present.
 * A visible-window capture is separately needed to validate presentation. */
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
static IDirect3DTexture8 *texture;
struct vertex { float x,y,z,rhw,u,v; };
static struct vertex vertices[4]={{0,0,0.5f,1,0,0},{128,0,0.5f,1,1,0},{0,128,0.5f,1,0,1},{128,128,0.5f,1,1,1}};

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
    if (texture) IDirect3DTexture8_Release(texture);
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
    hr = IDirect3DDevice8_CreateTexture(device, 2, 2, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture);
    if (FAILED(hr)) finish("CreateTexture: FAIL\r\n", hr, 1);
    D3DLOCKED_RECT texture_rect;
    hr = IDirect3DTexture8_LockRect(texture, 0, &texture_rect, NULL, 0);
    if (FAILED(hr)) finish("Texture lock: FAIL\r\n", hr, 1);
    for(int y=0;y<2;y++) for(int x=0;x<2;x++) ((DWORD *)((char*)texture_rect.pBits+y*texture_rect.Pitch))[x]=D3DCOLOR_XRGB(224,64,32);
    IDirect3DTexture8_UnlockRect(texture, 0);
    IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    IDirect3DDevice8_SetRenderState(device, D3DRS_ZENABLE, FALSE);
    IDirect3DDevice8_SetRenderState(device, D3DRS_CULLMODE, D3DCULL_NONE);
    IDirect3DDevice8_SetTexture(device, 0, (IDirect3DBaseTexture8 *)texture);
    IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZRHW | D3DFVF_TEX1);
    if (FAILED(hr)) finish("FVF: FAIL\r\n", hr, 1);
    hr = IDirect3DDevice8_BeginScene(device);
    if (FAILED(hr)) finish("BeginScene: FAIL\r\n", hr, 1);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(struct vertex));
    if (FAILED(hr)) finish("Texture draw: FAIL\r\n", hr, 1);
    IDirect3DDevice8_EndScene(device);
    hr = IDirect3DDevice8_GetBackBuffer(device, 0, D3DBACKBUFFER_TYPE_MONO, &buffer);
    if (FAILED(hr)) finish("GetBackBuffer: FAIL\r\n", hr, 1);
    D3DLOCKED_RECT locked;
    hr = IDirect3DSurface8_LockRect(buffer, &locked, NULL, D3DLOCK_READONLY);
    if (FAILED(hr)) finish("LockRect: FAIL\r\n", hr, 1);
    DWORD pixel = *(const DWORD *)((const char *)locked.pBits + 64 * locked.Pitch + 64 * sizeof(DWORD));
    IDirect3DSurface8_UnlockRect(buffer);
    if ((pixel & 0x00ffffff) != (D3DCOLOR_XRGB(224,64,32) & 0x00ffffff)) finish("D3D8 textured readback: FAIL\r\n", E_FAIL, 1);
    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    if (FAILED(hr)) finish("Present: FAIL\r\n", hr, 1);
    finish("ST-D3D8-TEXTURE-PASS\r\n", S_OK, 0);
}
