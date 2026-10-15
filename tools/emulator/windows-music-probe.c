/* SPDX-License-Identifier: MIT. Own DirectMusic initialization fixture. */
#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#include <windows.h>
#include <objbase.h>
#include <dmusici.h>
static void report(const char *text) {
    DWORD written;
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),text,(DWORD)lstrlenA(text),&written,NULL);
}
static void result(const char *name,HRESULT status) {
    char hex[11]="0x00000000";
    for(unsigned i=0;i<8;i++)hex[9-i]="0123456789abcdef"[((unsigned)status>>(4*i))&15];
    report(name);report(": ");report(hex);report("\r\n");
    if(FAILED(status))ExitProcess(1);
}
void music_start(void) {
    result("CoInitialize",CoInitialize(NULL));
    IDirectMusicPerformance8 *performance=NULL;
    result("Performance8",CoCreateInstance(&CLSID_DirectMusicPerformance,NULL,
        CLSCTX_INPROC_SERVER,&IID_IDirectMusicPerformance8,(void **)&performance));
    HWND window=CreateWindowExA(0,"STATIC","School Tycoon music probe",WS_OVERLAPPED,
        0,0,1,1,NULL,NULL,GetModuleHandleA(NULL),NULL);
    if(!window)ExitProcess(2);
    ShowWindow(window,SW_SHOW);SetForegroundWindow(window);
    result("InitAudio",IDirectMusicPerformance8_InitAudio(performance,NULL,NULL,window,
        DMUS_APATH_DYNAMIC_STEREO,64,DMUS_AUDIOF_ALL,NULL));
    report("ST-MUSIC-INIT-PASS\r\n");
    WCHAR directory[MAX_PATH];
    DWORD length=GetEnvironmentVariableW(L"ST_AUDIO_DIRECTORY",directory,MAX_PATH);
    if(length && length<MAX_PATH) {
        IDirectMusicLoader8 *loader=NULL;
        result("Loader8",CoCreateInstance(&CLSID_DirectMusicLoader,NULL,
            CLSCTX_INPROC_SERVER,&IID_IDirectMusicLoader8,(void **)&loader));
        result("SearchDirectory",IDirectMusicLoader8_SetSearchDirectory(loader,&GUID_DirectMusicAllTypes,directory,TRUE));
        DMUS_OBJECTDESC desc={0};desc.dwSize=sizeof(desc);
        desc.dwValidData=DMUS_OBJ_CLASS|DMUS_OBJ_FILENAME;
        desc.guidClass=CLSID_DirectMusicScript;
        lstrcpyW(desc.wszFileName,L"SchoolTycoon.spt");
        IDirectMusicScript *script=NULL;
        result("LoadScript",IDirectMusicLoader8_GetObject(loader,&desc,&IID_IDirectMusicScript,(void **)&script));
        DMUS_SCRIPT_ERRORINFO error={0};error.dwSize=sizeof(error);
        HRESULT status=script->lpVtbl->Init(script,(IDirectMusicPerformance *)performance,&error);
        if(FAILED(status)) {
            char text[1024];
            int bytes=WideCharToMultiByte(CP_UTF8,0,error.wszDescription,-1,text,sizeof(text),NULL,NULL);
            if(bytes>0)report(text);
            report("\r\n");
        }
        result("ScriptInit",status);
        result("ScriptVolume",script->lpVtbl->SetVariableNumber(script,L"volume",0,&error));
        result("ScriptCall",script->lpVtbl->CallRoutine(script,L"DM_music_menu",&error));
        report("ST-MUSIC-SCRIPT-CALL-PASS\r\n");
        DWORD started=GetTickCount();
        while(GetTickCount()-started<15000) {
            MSG message;
            while(PeekMessageA(&message,NULL,0,0,PM_REMOVE)) {
                TranslateMessage(&message);DispatchMessageA(&message);
            }
            Sleep(10);
        }
        script->lpVtbl->Release(script);IDirectMusicLoader8_Release(loader);
    } else Sleep(1000);
    IDirectMusicPerformance8_CloseDown(performance);
    IDirectMusicPerformance8_Release(performance);
    DestroyWindow(window);CoUninitialize();ExitProcess(0);
}
