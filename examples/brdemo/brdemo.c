#include <SDL.h>
#include <brender.h>
#include <brglrend.h>
#include <brsdl2dev.h>

#include "brdemo.h"

#define USE_SOFT 0

/* begin hook */
void _BrBeginHook(void) // NOLINT(*-reserved-identifier)
{
    BrDevAddStatic(NULL, BrDrv1SDL2Begin, NULL);
    BrDevAddStatic(NULL, BrDrv1GLBegin, NULL);

#if USE_SOFT
    struct br_device *BR_EXPORT BrDrv1SoftPrimBegin(const char *arguments);
    struct br_device *BR_EXPORT BrDrv1SoftRendBegin(const char *arguments);

    BrDevAddStatic(NULL, BrDrv1SoftPrimBegin, NULL);
    BrDevAddStatic(NULL, BrDrv1SoftRendBegin, NULL);
#endif
}

/* end hook */
void _BrEndHook(void) // NOLINT(*-reserved-identifier)
{
}

br_error BrDemoDefaultInit(br_demo *demo)
{
    (void)demo;
    return BRE_OK;
}

void BrDemoDefaultProcessEvent(br_demo *demo, const SDL_Event *evt)
{
    (void)demo;
    (void)evt;
}

void BrDemoDefaultUpdate(br_demo *demo, br_scalar dt)
{
    (void)demo;
    (void)dt;
}

void BrDemoDefaultRender(br_demo *demo)
{
    BrRendererFrameBegin();
    BrPixelmapFill(demo->colour_buffer, demo->clear_colour);
    BrPixelmapFill(demo->depth_buffer, 0xFFFFFFFF);

    if(demo->camera != NULL)
        BrZbSceneRender(demo->world, demo->camera, demo->colour_buffer, demo->depth_buffer);

    BrRendererFrameEnd();
}

void BrDemoDefaultOnResize(br_demo *demo, br_uint_16 width, br_uint_16 height)
{
    /*
     * Update the camera's aspect ratio to match the camera.
     */
    if(demo->camera != NULL) {
        br_camera *camera_data = demo->camera->type_data;
        camera_data->aspect    = BR_DIV(BR_SCALAR(width), BR_SCALAR(height));
    }
}

void BrDemoDefaultDestroy(br_demo *demo)
{
    (void)demo;
}

const static br_demo_dispatch default_dispatch = {
    .init          = BrDemoDefaultInit,
    .process_event = BrDemoDefaultProcessEvent,
    .update        = BrDemoDefaultUpdate,
    .render        = BrDemoDefaultRender,
    .on_resize     = BrDemoDefaultOnResize,
    .destroy       = BrDemoDefaultDestroy,
};

/*
 * Custom load function to de-CLUT a pixelmap upon loading.
 */

/* FIXME: de-global this */
static br_demo *current_demo = NULL;

static br_pixelmap *BR_CALLBACK MapFindFailedLoadDeCLUT(const char *name)
{
    br_pixelmap *pm, *pm2;
    br_pixelmap *palette = current_demo != NULL ? current_demo->palette : NULL;

    if((pm = BrPixelmapLoad(name)) != NULL) {
        if(pm->type == BR_PMT_INDEX_8 && pm->map == NULL)
            pm->map = palette;

#if !USE_SOFT
        if((pm2 = BrPixelmapDeCLUT(pm)) != NULL) {
            if(pm->map != palette)
                BrResFree(pm->map);

            BrResFree(pm);
            pm = pm2;
        }

        if(pm->identifier != NULL)
            BrResFree(pm->identifier);

        pm->identifier = BrResStrDup(pm, name);
#endif
        BrMapAdd(pm);
    }

    return pm;
}

int BrDemoRun(const char *title, br_uint_16 width, br_uint_16 height, const br_demo_dispatch *dispatch)
{
    br_error    err;
    int         ret           = 1;
    br_demo    *demo          = NULL;
    SDL_Window *window        = NULL;
    br_boolean  is_fullscreen = BR_FALSE;
    br_uint_64  ticks_now;
    br_uint_64  ticks_last;

    if(dispatch == NULL)
        dispatch = &default_dispatch;

    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");

    BrBegin();

    BrModelFindHook(BrModelFindFailedLoad);
    BrMapFindHook(MapFindFailedLoadDeCLUT);
    BrMaterialFindHook(BrMaterialFindFailedLoad);
    BrTableFindHook(BrTableFindFailedLoad);

    demo           = BrResAllocate(NULL, sizeof(br_demo), BR_MEMORY_APPLICATION);
    demo->dispatch = dispatch;

    /* FIXME: hack to work around no args in the find hooks. */
    current_demo = demo;

    /*
     * Create the window.
     */
    // clang-format off
    err = BrDevBeginVar(&demo->_screen, "SDL2",
                        BRT_WIDTH_I32,        width,
                        BRT_HEIGHT_I32,       height,
                        BRT_WINDOW_NAME_CSTR, title,
                        BRT_HIDPI_B,          BR_TRUE,
#if !USE_SOFT
                        BRT_PIXEL_TYPE_U8,    BR_PMT_RGB_888,
                        BRT_RESIZABLE_B,      BR_TRUE,
                        BRT_OPENGL_B,         BR_TRUE,
#endif
                        BR_NULL_TOKEN);
    // clang-format on
    if(err != BRE_OK) {
        BrLogError("DEMO", "Unable to create window.");
        goto cleanup;
    }

    demo->_screen->origin_x = (br_int_16)(demo->_screen->width >> 1);
    demo->_screen->origin_y = (br_int_16)(demo->_screen->height >> 1);

    /*
     * Windowed mode initially.
     */
    window = BrSDLUtilGetWindow(demo->_screen);
    SDL_SetWindowFullscreen(window, 0);
    is_fullscreen = BR_FALSE;

    if(BrSDLUtilResizeBuffers(demo->_screen, &demo->colour_buffer, &demo->depth_buffer) != BRE_OK) {
        BrLogError("DEMO", "Error resizing window buffers.");
        goto cleanup;
    }

    BrRendererBegin(demo->colour_buffer, NULL, NULL, demo->_primitive_heap, sizeof(demo->_primitive_heap));

    demo->order_table = BrZsOrderTableAllocate(1024, BR_ORDER_TABLE_LEAVE_BOUNDS | BR_ORDER_TABLE_BUCKET_SORT, BR_SORT_AVERAGE);

    demo->world = BrActorAllocate(BR_ACTOR_NONE, NULL);
    BrZsActorOrderTableSet(demo->world, demo->order_table);

    if(demo->dispatch->init(demo) != BRE_OK) {
        BrLogError("DEMO", "Initialisation failed.");
        goto cleanup;
    }

    demo->dispatch->on_resize(demo, demo->_screen->width, demo->_screen->height);

    ticks_last = SDL_GetTicks64();

    for(SDL_Event evt;;) {
        float dt;

        ticks_now  = SDL_GetTicks64();
        dt         = (float)(ticks_now - ticks_last) / 1000.0f;
        ticks_last = ticks_now;

        while(SDL_PollEvent(&evt) > 0) {
            switch(evt.type) {
                case SDL_QUIT:
                    ret = 0;
                    goto cleanup;

                case SDL_WINDOWEVENT:
                    /*
                     * Window event, pass it to the driver.
                     */
                    if(BrPixelmapHandleWindowEvent(demo->_screen, &evt.window) != BRE_OK) {
                        BrLogError("DEMO", "Error handling window event.");
                        goto cleanup;
                    }

                    switch(evt.window.event) {
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                            /*
                             * The main screen should have been resized above.
                             * Update its origin and resize the framebuffer.
                             */
                            demo->_screen->origin_x = (br_int_16)(demo->_screen->width >> 1);
                            demo->_screen->origin_y = (br_int_16)(demo->_screen->height >> 1);

                            if(BrSDLUtilResizeBuffers(demo->_screen, &demo->colour_buffer, &demo->depth_buffer) != BRE_OK) {
                                BrLogError("DEMO", "Error resizing window buffers.");
                                goto cleanup;
                            }

                            demo->dispatch->on_resize(demo, demo->_screen->width, demo->_screen->height);
                            break;
                    }
                    break;

                case SDL_KEYDOWN: {
                    if(BrSDLUtilIsAltEnter(&evt.key)) {
                        if(is_fullscreen) {
                            SDL_SetWindowFullscreen(window, 0);
                        } else {
                            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                        }
                        is_fullscreen = !is_fullscreen;
                        break;
                    }
                    break;
                }
            }

            demo->dispatch->process_event(demo, &evt);
        }

        demo->dispatch->update(demo, BR_SCALAR(dt));
        demo->dispatch->render(demo);

        BrPixelmapDoubleBuffer(demo->_screen, demo->colour_buffer);
    }

cleanup:

    demo->dispatch->destroy(demo);

    if(BrV1dbRendererQuery() != NULL)
        BrRendererEnd();

    if(demo->depth_buffer != NULL)
        BrPixelmapFree(demo->depth_buffer);

    if(demo->colour_buffer != NULL)
        BrPixelmapFree(demo->colour_buffer);

    if(demo->_screen != NULL)
        BrPixelmapFree(demo->_screen);

    if(demo->world != NULL)
        BrActorFree(demo->world);

    BrResFree(demo);
    BrEnd();
    return ret;
}
