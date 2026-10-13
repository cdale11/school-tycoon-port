# Original executable assessment

Target: Anbernic RG34XXSP running KNULLI Scarab, with PortMaster installed.
Goal: run the original game and its original assets.

## Inspected input

- File: user-supplied `SchoolTycoon.exe`, 2,461,696 bytes.
- SHA-256: `4c4372be0372223864a8648aacfc488fc9ed93d45c9e364609efa11f471b698a`.
- Format: PE32, Intel i386, Windows GUI; native machine code, no CLR header.
- PE timestamp: 2005-04-06 00:18:11. This is header metadata, not proof of release version.

Inspection used `file`, `objdump -p`, and `strings -a`. The executable was not run.

## CatCore archive inspection

User-supplied `CatCore.zip` is 295,304 bytes; SHA-256:
`eeb61429fa073592f591773f03e89bae184e9c12e9b819be8350bb4ffc9a37db`.
All file entries were read with ZIP CRC verification, outside the source checkout.
It contains 19 entries, including directories, totaling 652,783 uncompressed bytes.

- `DX8/nvlinker.dll`: PE32 i386, exports `CreateLinker`; imports only
  `KERNEL32.dll`. SHA-256:
  `96fc9c92d4724afd0899a79a7b84a9ac454472ea6aae8b578c97457232cf59c2`.
- `MASSIVEAD/m4d.dll`: PE32 i386, exports 25 `MMT_*` functions for advertising
  session/media handling; imports `WS2_32.dll`, `KERNEL32.dll`, `ADVAPI32.dll`.
  SHA-256: `5a35bb31f8a1d113312bce99d3ade82746c4ed63596767611e14dd49f197da31`.
- Seven `.nvo` files: spacewarps, lighting, miscvertex, skinning, basisskinning,
  eyespace, and objectspace.
- Six `compiled_pc/VShader003-*.bin` files. Their headers contain game-specific
  metadata; inspection does not establish that they are standalone shader bytecode.

Neither DLL's static import table adds another third-party DLL requirement.
Dynamic dependencies and runtime behavior remain untested. Preserve the supplied
folder names; the executable uses different capitalization in its path strings.

`tools/device-report.sh` collects read-only device and runtime metadata. Run it
on the handheld, not the cloud machine, and provide its output for runtime selection.

## Compatibility requirements

The import table includes `d3d8.dll` (`Direct3DCreate8`), `DDRAW.dll`
(`DirectDrawCreate`), Win32 UI/GDI/registry APIs, WinMM, Winsock, and AVI APIs.
Strings reference dynamically loaded DirectSound and these bundled components:

- `data/catcore/dx8/nvlinker.dll`
- `data/catcore/dx8/*.nvo`
- `data/catcore/dx8/compiled_pc/VShader*.bin`
- `data/catcore/MassiveAd/m4d.dll`

The presence of strings does not establish that every component is required on
every startup path. Inspect the supplied data tree and test execution to decide.

The binary references loose INI, TGA, WAV, OGG and other game data, including
`data/Language.ini`, `data/terrain/Levels.ini`, `data/simulation.ini`, and
`data/scenario/Farmland.ini`. References use inconsistent path capitalization;
preserve the original tree and use Windows-compatible path handling.

`Insert School Tycoon CD` and volume-query imports suggest a possible disc check.
Its actual behavior has not been tested.

## Approach and remaining evidence

PortMaster installation alone does not provide Windows or x86 compatibility.
Running this executable on the ARM target requires both x86 execution support
and Windows API support, plus a viable Direct3D 8 graphics path. A Wine-based
stack with suitable x86 translation is a candidate, not a verified solution.
Do not assume that Box64 alone runs this 32-bit executable, or that the device
provides Vulkan or desktop OpenGL.

Before building a launcher:

1. Obtain the installed game's file listing and bundled DLLs, especially the
   `data/catcore` subtree, and configuration files.
2. Record the exact device firmware, architecture, graphics drivers, and
   available PortMaster compatibility runtimes.
3. Validate a supported runtime with a small representative Windows graphics
   application, then launch the game with its complete asset tree and capture logs.
4. Resolve graphics, audio, input, and any legitimate disc/media requirements.
5. Validate the menu, a loaded scenario, controller-to-mouse mapping, saving and
   loading, and performance on the actual handheld.

No playable build, device compatibility, or performance claim is established by
this static inspection. Original executable and game assets are not committed
to the repository.
