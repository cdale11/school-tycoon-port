# Source inside the prepared ARM64 VM before a GL4ES/Crusty test.
# These paths are VM dependencies, not handheld installation paths.
export DISPLAY=${DISPLAY:-:95}
export __EGL_VENDOR_LIBRARY_FILENAMES=/userdata/50_mesa.json
export LIBGL_ALWAYS_SOFTWARE=1 GALLIUM_DRIVER=llvmpipe
export LIBGL_DRIVERS_PATH=/userdata/graphics/usr/lib/aarch64-linux-gnu/dri
export CRUSTY_LIBEGL=/userdata/graphics/usr/lib/aarch64-linux-gnu/libEGL.so.1
export CRUSTY_LIBSDL=/userdata/graphics/usr/lib/aarch64-linux-gnu/libSDL2-2.0.so.0
export CRUSTY_GL4ES=1 CRUSTY_WLMODE=0 CRUSTY_GLES=20
export SDL_VIDEODRIVER=x11 SDL_OPENGL_ES_DRIVER=1 SDL_VIDEO_X11_FORCE_EGL=1
export SDL_VIDEO_EGL_DRIVER="$CRUSTY_LIBEGL"
export SDL_VIDEO_GL_DRIVER=/userdata/graphics/usr/lib/aarch64-linux-gnu/libGLESv2.so.2
export LIBGL_EGL="$CRUSTY_LIBEGL" LIBGL_GLES="$SDL_VIDEO_GL_DRIVER" LIBGL_ES=2
export BOX64_LIBGL=/userdata/weston/lib_aarch64/graphics/gl4es_glxpass/libGL.so.1
export ST_CRUSTY_LIBRARY=/userdata/weston/lib_aarch64/graphics/crusty_glx/libcrusty.so
export LD_LIBRARY_PATH=/userdata/weston/lib_aarch64/graphics/gl4es_glxpass:/userdata/weston/lib_aarch64/graphics/crusty_glx:/userdata/weston/lib_aarch64:/userdata/graphics/usr/lib/aarch64-linux-gnu
export LD_PRELOAD=/userdata/libst-glx-config.so:/userdata/libst-egl-prime.so:$ST_CRUSTY_LIBRARY
