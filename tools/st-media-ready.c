/* SPDX-License-Identifier: MIT
 * Register and instantiate the selected PE32 media classes before gameplay.
 * This validates COM/loader readiness, not a speaker or audio-device output.
 */
#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#include <windows.h>
#include <objbase.h>
#include <mmsystem.h>
#include <dsound.h>
#include <dmusici.h>
#include <mmdeviceapi.h>
static const GUID media_device_enumerator={0xbcde0395,0xe52f,0x467c,
    {0x8e,0x3d,0xc4,0x57,0x92,0x91,0x69,0x2e}};
static const GUID media_vbscript={0xb54f3741,0x5b07,0x11cf,
    {0xa4,0xb0,0x00,0xaa,0x00,0x4a,0x55,0xe8}};
static void report(const char *text) {
    DWORD written;
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),text,(DWORD)lstrlenA(text),&written,NULL);
}
static void fail(const char *name,HRESULT status) {
    char hex[11]="0x00000000";
    for(unsigned i=0;i<8;i++)hex[9-i]="0123456789abcdef"[((unsigned)status>>(4*i))&15];
    report("Media readiness failed: ");report(name?name:"(default)");report(" ");report(hex);report("\r\n");
    ExitProcess(1);
}
static void ensure_value(const char *path,const char *name,const char *value) {
    HKEY key=NULL;
    LONG status=RegCreateKeyExA(HKEY_LOCAL_MACHINE,path,0,NULL,0,KEY_QUERY_VALUE|KEY_SET_VALUE,NULL,&key,NULL);
    if(status!=ERROR_SUCCESS)fail(path,HRESULT_FROM_WIN32(status));
    DWORD bytes=0;
    status=RegQueryValueExA(key,name,NULL,NULL,NULL,&bytes);
    if(status==ERROR_FILE_NOT_FOUND)status=RegSetValueExA(key,name,0,REG_SZ,(const BYTE *)value,(DWORD)lstrlenA(value)+1);
    RegCloseKey(key);
    if(status!=ERROR_SUCCESS)fail(name,HRESULT_FROM_WIN32(status));
}
void media_start(void) {
    static const char *libraries[]={"dmloader.dll","dmime.dll","dmusic.dll","dmsynth.dll",
        "dmband.dll","dmcompos.dll","dmstyle.dll","dmscript.dll","dswave.dll","dsound.dll","mmdevapi.dll","vbscript.dll"};
    for(unsigned i=0;i<sizeof(libraries)/sizeof(libraries[0]);i++) {
        report("Media registration: ");report(libraries[i]);report("\r\n");
        HMODULE module=LoadLibraryA(libraries[i]);
        if(!module)fail(libraries[i],HRESULT_FROM_WIN32(GetLastError()));
        HRESULT (WINAPI *registration)(void)=(HRESULT (WINAPI *)(void))(void *)GetProcAddress(module,"DllRegisterServer");
        if(!registration && !lstrcmpA(libraries[i],"dsound.dll")) {
            /* The pinned native DirectSound exports DllGetClassObject but no
             * DllRegisterServer. Populate only missing WoW64 COM mappings. */
            static const char *keys[]={
                "Software\\Classes\\CLSID\\{47D4D946-62E8-11CF-93BC-444553540000}\\InprocServer32",
                "Software\\Classes\\CLSID\\{3901CC3F-84B5-4FA4-BA35-AA8172B8A09B}\\InprocServer32"};
            for(unsigned j=0;j<sizeof(keys)/sizeof(keys[0]);j++) {
                ensure_value(keys[j],NULL,"dsound.dll");
                ensure_value(keys[j],"ThreadingModel","Both");
            }
            FreeLibrary(module);continue;
        }
        if(!registration)fail(libraries[i],HRESULT_FROM_WIN32(GetLastError()));
        HRESULT status=registration();
        if(FAILED(status))fail(libraries[i],status);
        FreeLibrary(module);
    }
    /* wine.inf's native view does not necessarily populate the WoW64 view.
     * Match its two non-proprietary synth defaults, preserving existing values.
     * No nonexistent GM/DLS file is installed or advertised. */
    ensure_value("Software\\Microsoft\\DirectMusic\\Defaults","DefaultOutputPort",
        "{58C2B4D0-46E7-11D1-89AC-00A0C9054129}");
    ensure_value("Software\\Microsoft\\DirectMusic\\SoftwareSynths\\{58C2B4D0-46E7-11D1-89AC-00A0C9054129}",
        "Description","Microsoft Software Synthesizer");
    HRESULT status=CoInitialize(NULL);
    if(FAILED(status))fail("CoInitialize",status);
    static const struct {const GUID *id;const char *name;} classes[]={
        {&CLSID_DirectMusicLoader,"DirectMusicLoader"},
        {&CLSID_DirectMusicPerformance,"DirectMusicPerformance"},
        {&CLSID_DirectMusicSegment,"DirectMusicSegment"},
        {&CLSID_DirectMusicCollection,"DirectMusicCollection"},
        {&CLSID_DirectMusicBand,"DirectMusicBand"},
        {&CLSID_DirectMusicStyle,"DirectMusicStyle"},
        {&CLSID_DirectMusicComposer,"DirectMusicComposer"},
        {&CLSID_DirectMusicScript,"DirectMusicScript"},
        {&CLSID_DirectSoundWave,"DirectSoundWave"},
        {&CLSID_DirectMusicSynth,"DirectMusicSynth"},
        {&CLSID_DirectSound8,"DirectSound8"},
        {&media_device_enumerator,"MMDeviceEnumerator"},
        {&media_vbscript,"VBScript"}
    };
    for(unsigned i=0;i<sizeof(classes)/sizeof(classes[0]);i++) {
        report("Media class: ");report(classes[i].name);report("\r\n");
        IUnknown *object=NULL;
        status=CoCreateInstance(classes[i].id,NULL,CLSCTX_INPROC_SERVER,&IID_IUnknown,(void **)&object);
        if(FAILED(status))fail(classes[i].name,status);
        if(!object)fail(classes[i].name,E_POINTER);
        IUnknown_Release(object);
    }
    CoUninitialize();report("ST-PREFIX-MEDIA-PASS\r\n");ExitProcess(0);
}
