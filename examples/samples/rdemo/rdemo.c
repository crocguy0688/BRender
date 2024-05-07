/* Teapot in Space */
/* Simulated reflections by rendering into environment maps */

#include <stdio.h>
#include <brdemo.h>

/* This demo simulates a giant reflecting teapot hanging above a */
/* spinning earth */

static br_material *CreateMaterial(const char *identifier)
{
    br_material *mat;

    if((mat = BrMaterialAllocate(identifier)) == NULL)
        return NULL;

    mat->colour  = BR_COLOUR_RGB(255, 255, 255);
    mat->opacity = 255;
    mat->ka      = BR_UFRACTION(0.05);
    mat->kd      = BR_UFRACTION(0.45);
    mat->ks      = BR_UFRACTION(0.50);
    mat->power   = BR_SCALAR(20);

    mat->flags         = BR_MATF_GOURAUD | BR_MATF_LIGHT | BR_MATF_DISABLE_COLOUR_KEY | BR_MATF_SUBDIVIDE;
    mat->subdivide_tolerance = 1;
    mat->map_transform = (br_matrix23){
        {BR_VECTOR2(1, 0), BR_VECTOR2(0, 1), BR_VECTOR2(0, 0)}
    };
    mat->index_base  = 0;
    mat->index_range = 63;
    return mat;
}

typedef struct br_demo_reflection {
    br_pixelmap *envmap1_pm;
    br_pixelmap *earth_pm;
    br_pixelmap *planet_pm;
    br_pixelmap *moon_pm;
    br_pixelmap *ast_pm;

    br_actor *teapot;
    br_actor *earth;
    br_actor *moon;
    br_actor *mirror_view;

    br_pixelmap *mirror_pm;
    br_pixelmap *mirror_depth;

    br_float theta;
} br_demo_reflection;

br_error ReflectionInit(br_demo *demo)
{
    br_demo_reflection *ref;
    br_material        *earth_mat, *moon_mat, *mirror_mat;
    br_model           *teapot_model, *earth_model;
    br_actor           *pair, *system, *light1, *light2, *env;
    br_camera          *camera_data, *mirror_camera_data;

    ref        = BrResAllocate(demo, sizeof(br_demo_reflection), BR_MEMORY_APPLICATION);
    demo->user = ref;

    /*
     * Load the palette.
     */
    if((demo->palette = BrPixelmapLoad("std.pal")) == NULL) {
        BrLogError("DEMO", "Error loading std.pal.");
        return BRE_FAIL;
    }
    BrMapAdd(demo->palette);

    /*
     * If the teapot simply reflected the earth with no other source
     * for reflection then it would mostly look black so rather than simply/
     * reflect the earth into the teapot we'll also have an environment map
     * to provide a plain background that is also reflected.
     */
    if((ref->envmap1_pm = BrMapFind("testenv1.pix")) == NULL) {
        BrLogError("DEMO", "Error loading testenv1.pix");
        return BRE_FAIL;
    }
    BrMapAdd(ref->envmap1_pm);

    /*
     * Load the earth texture.
     */
    if((ref->earth_pm = BrMapFind("earth.pix")) == NULL) {
        BrLogError("DEMO", "Unable to load earth.pix");
        return BRE_FAIL;
    }
    BrMapAdd(ref->earth_pm);

    if((ref->planet_pm = BrMapFind("planet.pix")) == NULL) {
        BrLogError("DEMO", "Unable to load planet.pix");
        return BRE_FAIL;
    }
    BrMapAdd(ref->planet_pm);

    if((ref->moon_pm = BrMapFind("moon.pix")) == NULL) {
        BrLogError("DEMO", "Unable to load moon.pix");
        return BRE_FAIL;
    }
    BrMapAdd(ref->moon_pm);

    if((ref->ast_pm = BrMapFind("ast.pix")) == NULL) {
        BrLogError("DEMO", "Unable to load ast.pix");
        return BRE_FAIL;
    }
    BrMapAdd(ref->ast_pm);

    if((teapot_model = BrModelLoad("teapot.dat")) == NULL) {
        BrLogError("DEMO", "Unable to load teapot.dat");
        return BRE_FAIL;
    }
    BrModelAdd(teapot_model);

    if((earth_model = BrModelLoad("sph32.dat")) == NULL) {
        BrLogError("DEMO", "Unable to load sph32.dat");
        return BRE_FAIL;
    }
    BrModelAdd(earth_model);

    /*
     * Complete definition of mirror texture. Note that the colour_map
     * is a newly allocated pixelmap into which we'll render later.
     */
    ref->mirror_pm = BrPixelmapMatchTypedSized(demo->colour_buffer, BR_PMMATCH_OFFSCREEN, BR_PMT_INDEX_8, 256, 256);
    ref->mirror_pm->origin_x = (br_int_16)(ref->mirror_pm->width >> 1);
    ref->mirror_pm->origin_y = (br_int_16)(ref->mirror_pm->height >> 1);
    ref->mirror_pm->map      = demo->palette;
    BrMapAdd(ref->mirror_pm);

    /*
     * The teapot model is subdivided into many small triangles. This
     * means we can use BR_MATF_ENVIRONMENT_I even though it is
     * less accurate than BR_MATF_ENVIRONMENT_L.
     * */

    /*
     * Firstly `mirror' is the structure defining the material from which
     * the teapot is made...an environment map.
     */
    mirror_mat = CreateMaterial("mirror");
    mirror_mat->flags |= BR_MATF_ENVIRONMENT_L;
    mirror_mat->colour_map = ref->mirror_pm;
    BrMaterialAdd(mirror_mat);

    ref->mirror_depth = BrPixelmapMatch(ref->mirror_pm, BR_PMMATCH_DEPTH_16);

    /*
     * Now the material from which the earth is made.
     */
    earth_mat             = CreateMaterial("earth");
    earth_mat->colour_map = ref->earth_pm;
    BrMaterialAdd(earth_mat);

    moon_mat             = CreateMaterial("moon");
    moon_mat->colour_map = ref->moon_pm;
    BrMaterialAdd(moon_mat);

    /*
     * In the beginning...
     * The earth-teapot system forms a single `pair' unit
     */

    pair         = BrActorAdd(demo->world, BrActorAllocate(BR_ACTOR_NONE, NULL));
    pair->t.type = BR_TRANSFORM_MATRIX34;
    BrMatrix34Identity(&pair->t.t.mat);

    system         = BrActorAdd(pair, BrActorAllocate(BR_ACTOR_NONE, NULL));
    system->t.type = BR_TRANSFORM_MATRIX34;
    BrMatrix34PostTranslate(&system->t.t.mat, BR_SCALAR(-0.7), BR_SCALAR(0), BR_SCALAR(0));

    /*
     * Load teapot and place into `pair'
     */

    ref->teapot        = BrActorAdd(pair, BrActorAllocate(BR_ACTOR_MODEL, NULL));
    ref->teapot->model = teapot_model;

    ref->teapot->material = BrMaterialFind("mirror");
    ref->teapot->t.type   = BR_TRANSFORM_MATRIX34;
    BrMatrix34Scale(&ref->teapot->t.t.mat, BR_SCALAR(0.7), BR_SCALAR(0.7), BR_SCALAR(0.7));
    BrMatrix34PostRotateX(&ref->teapot->t.t.mat, BR_ANGLE_DEG(-70));
    BrMatrix34PostRotateY(&ref->teapot->t.t.mat, BR_ANGLE_DEG(-45));
    BrMatrix34PostTranslate(&ref->teapot->t.t.mat, BR_SCALAR(0.7), BR_SCALAR(0), BR_SCALAR(0));

    /*
     * Load and place spheres into earth-moon system.
     */

    ref->earth           = BrActorAdd(system, BrActorAllocate(BR_ACTOR_MODEL, NULL));
    ref->earth->model    = earth_model;
    ref->earth->material = BrMaterialFind("earth");
    ref->earth->t.type   = BR_TRANSFORM_MATRIX34;

    ref->moon           = BrActorAdd(system, BrActorAllocate(BR_ACTOR_MODEL, NULL));
    ref->moon->model    = ref->earth->model;
    ref->moon->material = BrMaterialFind("moon");
    ref->moon->t.type   = BR_TRANSFORM_MATRIX34;

    /* Now place the main camera from which we will view the system */

    demo->camera         = BrActorAdd(demo->world, BrActorAllocate(BR_ACTOR_CAMERA, NULL));
    demo->camera->t.type = BR_TRANSFORM_MATRIX34;

    camera_data                = demo->camera->type_data;
    camera_data->type          = BR_CAMERA_PERSPECTIVE;
    camera_data->field_of_view = BR_ANGLE_DEG(45.0);
    camera_data->hither_z      = BR_SCALAR(0.1);
    camera_data->yon_z         = BR_SCALAR(20.0);

    BrMatrix34Translate(&demo->camera->t.t.mat, BR_SCALAR(0.0), BR_SCALAR(0.0), BR_SCALAR(2.0));

    demo->order_table->min_z = camera_data->hither_z;
    demo->order_table->max_z = camera_data->yon_z;

    /* This is the camera that will be used to provide the reflection */
    /* of the earth into the teapot */

    ref->mirror_view         = BrActorAdd(pair, BrActorAllocate(BR_ACTOR_CAMERA, NULL));
    ref->mirror_view->t.type = BR_TRANSFORM_MATRIX34;

    mirror_camera_data                = ref->mirror_view->type_data;
    mirror_camera_data->type          = BR_CAMERA_PERSPECTIVE;
    mirror_camera_data->field_of_view = BR_ANGLE_DEG(90.0);
    mirror_camera_data->hither_z      = BR_SCALAR(0.1);
    mirror_camera_data->yon_z         = BR_SCALAR(20.0);
    mirror_camera_data->aspect        = BR_SCALAR(1.46);

    /*
     * Rotate the camera to face the earth...
     */
    BrMatrix34RotateY(&ref->mirror_view->t.t.mat, BR_ANGLE_DEG(90));

    /*
     * ...and move it into the centre of the teapot.
     */

    BrMatrix34PostTranslate(&ref->mirror_view->t.t.mat, BR_SCALAR(0.5), BR_SCALAR(0), BR_SCALAR(0));

    /*
     * Let there be light...so we can see what we are doing.
     */
    light1                                         = BrActorAdd(demo->world, BrActorAllocate(BR_ACTOR_LIGHT, NULL));
    ((br_light *)light1->type_data)->type          = BR_LIGHT_DIRECT;
    ((br_light *)light1->type_data)->attenuation_c = BR_SCALAR(1.0);
    light1->t.type                                 = BR_TRANSFORM_MATRIX34;
    BrMatrix34RotateY(&light1->t.t.mat, BR_ANGLE_DEG(35));
    BrMatrix34RotateX(&light1->t.t.mat, BR_ANGLE_DEG(35));

    light2                                         = BrActorAdd(demo->world, BrActorAllocate(BR_ACTOR_LIGHT, NULL));
    ((br_light *)light2->type_data)->type          = BR_LIGHT_DIRECT;
    ((br_light *)light2->type_data)->attenuation_c = BR_SCALAR(1.0);
    light2->t.type                                 = BR_TRANSFORM_MATRIX34;
    BrMatrix34RotateY(&light2->t.t.mat, BR_ANGLE_DEG(-35));
    BrMatrix34RotateX(&light2->t.t.mat, BR_ANGLE_DEG(-90));

    /*
     * Mustn't forget to turn the light on.
     */
    BrLightEnable(light1);
    BrLightEnable(light2);

    /*
     * The environment map is attached to the earth-teapot pair so that
     * when rotated the reflection rotates accordingly.
     */
    env         = BrActorAdd(pair, BrActorAllocate(BR_ACTOR_NONE, NULL));
    env->t.type = BR_TRANSFORM_MATRIX34;

    /*
     * Rotate environment map so that the centre of it is in the same
     * direction as the earth.
     */
    BrMatrix34RotateY(&env->t.t.mat, BR_ANGLE_DEG(-90));
    BrEnvironmentSet(env);

    return BRE_OK;
}

void ReflectionUpdate(br_demo *demo, float dt)
{
    br_demo_reflection *ref = demo->user;

    /*
     * Place rotated earth and moon.
     */
    ref->theta = fmodf(ref->theta + dt * 25.0f, 360.0f);

    BrMatrix34Scale(&ref->earth->t.t.mat, BR_SCALAR(0.35), BR_SCALAR(0.35), BR_SCALAR(0.35));
    BrMatrix34PostRotateX(&ref->earth->t.t.mat, BR_ANGLE_DEG(90));
    BrMatrix34PreRotateZ(&ref->earth->t.t.mat, BR_ANGLE_DEG(3 * ref->theta));

    BrMatrix34Scale(&ref->moon->t.t.mat, BR_SCALAR(0.1), BR_SCALAR(0.1), BR_SCALAR(0.1));
    BrMatrix34PostTranslate(&ref->moon->t.t.mat, BR_SCALAR(-0.7), BR_SCALAR(0), BR_SCALAR(0));
    BrMatrix34PostRotateY(&ref->moon->t.t.mat, BR_ANGLE_DEG(-2 * ref->theta));
}

void ReflectionRender(br_demo *demo)
{
    br_demo_reflection *ref = demo->user;

    BrRendererFrameBegin();

    /*
     * Copy plain environment map into mirror pixelmap. We will render
     * the earth over this. Also clear the Z buffer.
     */
    BrPixelmapCopy(ref->mirror_pm, ref->envmap1_pm);
    BrPixelmapFill(ref->mirror_depth, 0xffffffff);

    /*
     * When rendering the earth into the reflection we don't want to
     * see a view of the inside of the teapot. Turn it off for now,
     */
    ref->teapot->render_style = BR_RSTYLE_NONE;

    /*
     * Render reflection and turn back on teapot.
     */
    BrZbSceneRender(demo->world, ref->mirror_view, ref->mirror_pm, ref->mirror_depth);
    ref->teapot->render_style = BR_RSTYLE_DEFAULT;

    /*
     * And now render the earth along with the shiny teapot.
     */
    BrPixelmapFill(demo->colour_buffer, demo->clear_colour);
    BrPixelmapFill(demo->depth_buffer, 0xFFFFFFFF);
    BrZbSceneRender(demo->world, demo->camera, demo->colour_buffer, demo->depth_buffer);
    BrRendererFrameEnd();
}

void ReflectionProcessEvent(br_demo *demo, const SDL_Event *evt)
{
    br_demo_reflection *ref = demo->user;

    switch(evt->type) {
        case SDL_KEYDOWN:
            switch(evt->key.keysym.sym) {
                case SDLK_m:
                    if(ref->moon->material->colour_map == ref->moon_pm)
                        ref->moon->material->colour_map = ref->ast_pm;
                    else
                        ref->moon->material->colour_map = ref->moon_pm;

                    BrMaterialUpdate(ref->moon->material, BR_MATU_ALL);
                    break;

                case SDLK_e:
                    if(ref->earth->material->colour_map == ref->earth_pm)
                        ref->earth->material->colour_map = ref->planet_pm;
                    else
                        ref->earth->material->colour_map = ref->earth_pm;

                    BrMaterialUpdate(ref->earth->material, BR_MATU_ALL);
                    break;
            }

            break;
    }
}

const static br_demo_dispatch dispatch = {
    .init          = ReflectionInit,
    .process_event = ReflectionProcessEvent,
    .update        = ReflectionUpdate,
    .render        = ReflectionRender,
    .on_resize     = BrDemoDefaultOnResize,
    .destroy       = BrDemoDefaultDestroy,
};

int main(int argc, char **argv)
{
    return BrDemoRun("BRender Reflection Demo", 1280, 720, &dispatch);
}
