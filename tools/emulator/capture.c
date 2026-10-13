/* Capture the VM's X11 root window as a PPM for private visual verification. */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned char channel(unsigned long pixel, unsigned long mask) {
    if (!mask) return 0;
    while (!(mask & 1)) { mask >>= 1; pixel >>= 1; }
    return (unsigned char)(((pixel & mask) * 255) / mask);
}
int main(int argc, char **argv) {
    if (argc != 2) { fprintf(stderr, "Usage: capture output.ppm\n"); return 2; }
    Display *display = XOpenDisplay(NULL);
    if (!display) { fprintf(stderr, "Cannot open X display\n"); return 1; }
    Window root = DefaultRootWindow(display);
    XWindowAttributes attributes;
    if (!XGetWindowAttributes(display, root, &attributes)) return 1;
    XImage *frame = XGetImage(display, root, 0, 0, attributes.width,
                             attributes.height, AllPlanes, ZPixmap);
    if (!frame) return 1;
    FILE *out = fopen(argv[1], "wb");
    if (!out) { perror(argv[1]); return 1; }
    fprintf(out, "P6\n%d %d\n255\n", frame->width, frame->height);
    for (int y = 0; y < frame->height; y++) for (int x = 0; x < frame->width; x++) {
        unsigned long pixel = XGetPixel(frame, x, y);
        unsigned char rgb[3] = {channel(pixel, frame->red_mask),
            channel(pixel, frame->green_mask), channel(pixel, frame->blue_mask)};
        if (fwrite(rgb, 1, 3, out) != 3) { fclose(out); return 1; }
    }
    int status = fclose(out) != 0;
    XDestroyImage(frame);
    XCloseDisplay(display);
    return status;
}
