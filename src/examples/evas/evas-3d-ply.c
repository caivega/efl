/**
 * Simple Evas example illustrating import/export of .ply format.
 *
 * Read meshes from "tested_man_all_with_mods.ply", "tested_man_only_geometry.ply" and "tested_man_without_UVs.ply".
 * After that cheange some properties of material.
 * After that save material to "saved_man.mtl"
 * and geometry to "saved_man_all_with_mods.ply", "saved_man_only_geometry.ply" and "saved_man_without_UVs.ply".
 *
 * @verbatim
 * gcc -o evas-3d-ply evas-3d-ply.c `pkg-config --libs --cflags efl evas ecore ecore-evas ecore-file eo`
 * @endverbatim
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define PACKAGE_EXAMPLES_DIR "."
#define EFL_EO_API_SUPPORT
#define EFL_BETA_API_SUPPORT
#endif

#include <Eo.h>
#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Ecore_File.h>
#include "evas-common.h"

#define  WIDTH 1024
#define  HEIGHT 1024

#define NUMBER_OF_MESHES 32

static const char *image_path = PACKAGE_EXAMPLES_DIR EVAS_IMAGE_FOLDER "/star.jpg";
static const char *input_template = PACKAGE_EXAMPLES_DIR EVAS_MODEL_FOLDER "/";
static const char *output_template = PACKAGE_EXAMPLES_DIR EVAS_SAVED_FILES "/";
static const char *file_name[8] = {"Normal_UVs_Colors.ply",
                                   "Normal_UVs_NoColors.ply",
                                   "Normal_NoUVs_Colors.ply",
                                   "Normal_NoUVs_NoColors.ply",
                                   "NoNormal_UVs_Colors.ply",
                                   "NoNormal_UVs_NoColors.ply",
                                   "NoNormal_NoUVs_Colors.ply",
                                   "NoNormal_NoUVs_NoColors.ply"};

int draw_mode[2] = {EVAS_3D_SHADE_MODE_PHONG, EVAS_3D_SHADE_MODE_VERTEX_COLOR};

Ecore_Evas *ecore_evas = NULL;
Evas *evas = NULL;
Eo *background = NULL;
Eo *image = NULL;

Eo *scene = NULL;
Eo *root_node = NULL;
Eo *camera_node = NULL;
Eo *light_node = NULL;
Eo *camera = NULL;

Eo *mesh_node[NUMBER_OF_MESHES];
Eo *mesh[NUMBER_OF_MESHES];

Eo *material = NULL;
Eo *texture = NULL;
Eo *light = NULL;
Ecore_Animator *anim = NULL;

static float angle = 0;

static Eina_Bool
_animate_scene(void *data)
{
   angle += 0.2;

   eo_do((Evas_3D_Node *)data, evas_3d_node_orientation_angle_axis_set(angle, 1.0, 1.0, 1.0));

   /* Rotate */
   if (angle > 360.0) angle -= 360.0f;

   return EINA_TRUE;
}

static void
_on_delete(Ecore_Evas *ee EINA_UNUSED)
{
   ecore_main_loop_quit();
}

static void
_on_canvas_resize(Ecore_Evas *ee)
{
   int w, h;

   ecore_evas_geometry_get(ee, NULL, NULL, &w, &h);
   eo_do(background, efl_gfx_size_set(w, h));
   eo_do(image, efl_gfx_size_set(w, h));
}

int
main(void)
{
   char buffer[PATH_MAX];
   int i;

   for (i = 0; i < NUMBER_OF_MESHES; i++)
     {
        mesh_node[i] = NULL;
        mesh[i] = NULL;
     }

   //Unless Evas 3D supports Software renderer, we set gl backened forcely.
   setenv("ECORE_EVAS_ENGINE", "opengl_x11", 1);

   if (!ecore_evas_init()) return 0;

   ecore_evas = ecore_evas_new(NULL, 10, 10, WIDTH, HEIGHT, NULL);

   if (!ecore_evas) return 0;

   ecore_evas_callback_delete_request_set(ecore_evas, _on_delete);
   ecore_evas_callback_resize_set(ecore_evas, _on_canvas_resize);
   ecore_evas_show(ecore_evas);

   evas = ecore_evas_get(ecore_evas);

   /* Add a scene object .*/
   scene = eo_add(EVAS_3D_SCENE_CLASS, evas);

   /* Add the root node for the scene. */
   root_node = eo_add(EVAS_3D_NODE_CLASS, evas,
                      evas_3d_node_constructor(EVAS_3D_NODE_TYPE_NODE));

   /* Add the camera. */
   camera = eo_add(EVAS_3D_CAMERA_CLASS, evas);
   eo_do(camera,
         evas_3d_camera_projection_perspective_set(60.0, 1.0, 1.0, 500.0));

   camera_node = eo_add(EVAS_3D_NODE_CLASS, evas,
                        evas_3d_node_constructor(EVAS_3D_NODE_TYPE_CAMERA));
   eo_do(camera_node,
         evas_3d_node_camera_set(camera));
   eo_do(root_node,
         evas_3d_node_member_add(camera_node));
   eo_do(camera_node,
         evas_3d_node_position_set(15.0, 0.0, 0.0),
         evas_3d_node_look_at_set(EVAS_3D_SPACE_PARENT, 0.0, 0.0, 0.0,
                                  EVAS_3D_SPACE_PARENT, 0.0, 0.0, 1.0));
   /* Add the light. */
   light = eo_add(EVAS_3D_LIGHT_CLASS, evas);
   eo_do(light,
         evas_3d_light_ambient_set(1.0, 1.0, 1.0, 1.0),
         evas_3d_light_diffuse_set(1.0, 1.0, 1.0, 1.0),
         evas_3d_light_specular_set(1.0, 1.0, 1.0, 1.0),
         evas_3d_light_directional_set(EINA_TRUE));

   light_node = eo_add(EVAS_3D_NODE_CLASS, evas,
                       evas_3d_node_constructor(EVAS_3D_NODE_TYPE_LIGHT));
   eo_do(light_node,
         evas_3d_node_light_set(light),
         evas_3d_node_position_set(1000.0, 0.0, 1000.0),
         evas_3d_node_look_at_set(EVAS_3D_SPACE_PARENT, 0.0, 0.0, 0.0,
                                  EVAS_3D_SPACE_PARENT, 0.0, 1.0, 0.0));
   eo_do(root_node,
         evas_3d_node_member_add(light_node));

   material = eo_add(EVAS_3D_MATERIAL_CLASS, evas);
   texture = eo_add(EVAS_3D_TEXTURE_CLASS, evas);
   eo_do(texture,
         evas_3d_texture_file_set(image_path, NULL),
         evas_3d_texture_filter_set(EVAS_3D_TEXTURE_FILTER_NEAREST,
                                    EVAS_3D_TEXTURE_FILTER_NEAREST),
         evas_3d_texture_wrap_set(EVAS_3D_WRAP_MODE_REPEAT,
                                  EVAS_3D_WRAP_MODE_REPEAT));
   eo_do(material,
         evas_3d_material_texture_set(EVAS_3D_MATERIAL_DIFFUSE, texture),
         evas_3d_material_enable_set(EVAS_3D_MATERIAL_AMBIENT, EINA_TRUE),
         evas_3d_material_enable_set(EVAS_3D_MATERIAL_DIFFUSE, EINA_TRUE),
         evas_3d_material_enable_set(EVAS_3D_MATERIAL_SPECULAR, EINA_TRUE),
         evas_3d_material_enable_set(EVAS_3D_MATERIAL_NORMAL, EINA_TRUE),
         evas_3d_material_color_set(EVAS_3D_MATERIAL_AMBIENT,
                                    0.01, 0.01, 0.01, 1.0),
         evas_3d_material_color_set(EVAS_3D_MATERIAL_DIFFUSE,
                                    1.0, 1.0, 1.0, 1.0),
         evas_3d_material_color_set(EVAS_3D_MATERIAL_SPECULAR,
                                    1.0, 1.0, 1.0, 1.0),
         evas_3d_material_shininess_set(50.0));

   if (!ecore_file_mkpath(PACKAGE_EXAMPLES_DIR EVAS_SAVED_FILES))
     fprintf(stderr, "Failed to create folder %s\n\n",
             PACKAGE_EXAMPLES_DIR EVAS_SAVED_FILES);

   /* Add the meshes. */
   for (i = 0; i < NUMBER_OF_MESHES; i++)
     {
        mesh[i] = eo_add(EVAS_3D_MESH_CLASS, evas);

        snprintf(buffer, PATH_MAX, "%s%s", input_template, file_name[i % 8]);
        eo_do(mesh[i],
              efl_file_set(buffer, NULL),
              evas_3d_mesh_frame_material_set(0, material),
              evas_3d_mesh_shade_mode_set(draw_mode[(i % 16) / 8]));

        snprintf(buffer, PATH_MAX, "%s%s", output_template, file_name[i % 8]);
        eo_do(mesh[i], efl_file_save(buffer, NULL, NULL));

        if (i > 15)
          {
             eo_do(mesh[i],
                   efl_file_set(buffer, NULL),
                   evas_3d_mesh_frame_material_set(0, material),
                   evas_3d_mesh_shade_mode_set(draw_mode[(i % 16) / 8]));
          }

        mesh_node[i] = eo_add(EVAS_3D_NODE_CLASS, evas,
                              evas_3d_node_constructor(EVAS_3D_NODE_TYPE_MESH));
        eo_do(root_node, evas_3d_node_member_add(mesh_node[i]));
        eo_do(mesh_node[i],
              evas_3d_node_mesh_add(mesh[i]),
              evas_3d_node_position_set(0, ((i % 4) * 4) + ((i / 16) * 1) - 6.5, (((i % 16) / 4) * 4) - 6));
     }

   /* Set up scene. */
   eo_do(scene,
         evas_3d_scene_root_node_set(root_node),
         evas_3d_scene_camera_node_set(camera_node),
         evas_3d_scene_size_set(WIDTH, HEIGHT));

   /* Add a background rectangle objects. */
   background = eo_add(EVAS_RECTANGLE_CLASS, evas);
   eo_do(background,
         efl_gfx_color_set(100, 100, 100, 255),
         efl_gfx_size_set(WIDTH, HEIGHT),
         efl_gfx_visible_set(EINA_TRUE));

   /* Add an image object for 3D scene rendering. */
   image = evas_object_image_filled_add(evas);
   eo_do(image,
         efl_gfx_size_set(WIDTH, HEIGHT),
         efl_gfx_visible_set(EINA_TRUE));

   /* Set the image object as render target for 3D scene. */
   eo_do(image, evas_obj_image_scene_set(scene));

   ecore_animator_frametime_set(0.03);
   for (i = 0; i < NUMBER_OF_MESHES; i++)
     anim = ecore_animator_add(_animate_scene, mesh_node[i]);

   /* Enter main loop. */
   ecore_main_loop_begin();

   ecore_animator_del(anim);
   ecore_evas_free(ecore_evas);
   ecore_evas_shutdown();

   return 0;
}
