#include "drv.h"
#include <brassert.h>

/*
 * Resize/recreate the back/colour and depth buffers to match the screen.
 */
br_error BrSDLUtilResizeBuffers(br_pixelmap *screen, br_pixelmap **colour_buffer, br_pixelmap **depth_buffer)
{
    br_pixelmap *tmp;

    UASSERT(screen != NULL);

    /*
     * Try to resize the framebuffer directly. Fall back to recreation if we can't.
     */
    if(*colour_buffer != NULL && *depth_buffer != NULL) {
        tmp = *colour_buffer;
        if(tmp->width != screen->width || tmp->height != screen->height) {
            if((tmp = BrPixelmapResize(tmp, screen->width, screen->height)) == NULL)
                goto full_cleanup;

            *colour_buffer = tmp;
            tmp->origin_x  = screen->origin_x;
            tmp->origin_y  = screen->origin_y;
        }

        tmp = *depth_buffer;
        if(tmp->width != screen->width || tmp->height != screen->height) {
            if((tmp = BrPixelmapResize(*depth_buffer, screen->width, screen->height)) == NULL)
                goto full_cleanup;

            *depth_buffer = tmp;
            tmp->origin_x = screen->origin_x;
            tmp->origin_y = screen->origin_y;
        }
        return BRE_OK;
    }

full_cleanup:

    /*
     * Clear the screen, just in case.
     */
    if(*colour_buffer != NULL) {
        BrPixelmapFill(*colour_buffer, 0);
        BrPixelmapDoubleBuffer(screen, *colour_buffer);
    }

    /*
     * Delete everything.
     */
    if(*depth_buffer != NULL) {
        BrPixelmapFree(*depth_buffer);
        *depth_buffer = NULL;
    }

    if(*colour_buffer != NULL) {
        BrPixelmapFree(*colour_buffer);
        *colour_buffer = NULL;
    }

    if((tmp = BrPixelmapMatchTyped(screen, BR_PMMATCH_OFFSCREEN, BR_PMT_RGB_565)) == NULL)
        return BRE_FAIL;

    *colour_buffer = tmp;
    tmp->origin_x  = screen->origin_x;
    tmp->origin_y  = screen->origin_y;

    if((tmp = BrPixelmapMatch(*colour_buffer, BR_PMMATCH_DEPTH_16)) == NULL) {
        BrPixelmapFree(*colour_buffer);
        *colour_buffer = NULL;
        return BRE_FAIL;
    }

    *depth_buffer = tmp;
    tmp->origin_x = screen->origin_x;
    tmp->origin_y = screen->origin_y;

    return BRE_OK;
}

br_boolean BrSDLUtilIsAltEnter(const SDL_KeyboardEvent *evt)
{
    if(evt->repeat)
        return BR_FALSE;

    if(evt->state != SDL_PRESSED)
        return BR_FALSE;

    if(!(evt->keysym.mod & KMOD_ALT))
        return BR_FALSE;

    return evt->keysym.scancode == SDL_SCANCODE_RETURN;
}

SDL_Window *BrSDLUtilGetWindow(br_pixelmap *pm)
{
    if(pm == NULL)
        return NULL;

    if(DevicePixelmapSDL2IsOurs(pm))
        return ((br_device_pixelmap *)pm)->window;

    return DevicePixelmapSDL2GetWindowGL(pm);
}
