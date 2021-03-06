#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include "watt_buffer.h"
#include "watt_input.h"
#include "watt_math.h"

#include <assert.h>
#include <math.h>

#define SOKOL_IMPL
#include "sokol_app.h"
#include "sokol_gfx.h"

#include "demo.glsl.h"

#define SAMPLE_COUNT 4

#define MAX_BUFFER_COUNT 64
#define MAX_SUBMESH_COUNT 32
#define MAX_MESH_COUNT 16
#define MAX_ENTITY_COUNT 16

static int32_t buffer_count = 0;
static sg_buffer buffers[MAX_BUFFER_COUNT];

static int32_t pipeline_count = 0;
static sg_pipeline pipelines[MAX_SUBMESH_COUNT];

struct submesh {
  int32_t buffer_indices[4]; // pos, normal, uv, indices
  int32_t buffer_offsets[4]; // pos, normal, uv, indices
  int32_t element_count;
  int32_t pipeline_idx;
};

static int32_t submesh_count = 0;
static struct submesh submeshes[MAX_SUBMESH_COUNT];

struct mesh {
  int32_t submesh_start_idx;
  int32_t submesh_end_idx;
};

static int32_t mesh_count = 0;
static struct mesh meshes[MAX_MESH_COUNT];

struct entity {
  int32_t mesh_idx;
  struct vec3 position;
  struct vec3 rotation;
  struct vec3 scale;
};

static int32_t entity_count = 0;
static struct entity entities[MAX_ENTITY_COUNT];

static sg_shader shader;
static struct input input_state = {0};

static int32_t gltf_attr_type_to_vs_input_slot(cgltf_attribute_type attr_type)
{
  switch (attr_type) {
  case cgltf_attribute_type_position:
    return ATTR_vs_position;
  case cgltf_attribute_type_normal:
    return ATTR_vs_normal;
  case cgltf_attribute_type_texcoord:
    return ATTR_vs_texcoord;
  default:
    return -1;
  }
}

static sg_vertex_format gltf_to_vertex_format(cgltf_accessor *acc)
{
  switch (acc->component_type) {
  case cgltf_component_type_r_8:
    if (acc->type == cgltf_type_vec4) {
      return acc->normalized ? SG_VERTEXFORMAT_BYTE4N : SG_VERTEXFORMAT_BYTE4;
    }
    break;
  case cgltf_component_type_r_8u:
    if (acc->type == cgltf_type_vec4) {
      return acc->normalized ? SG_VERTEXFORMAT_UBYTE4N : SG_VERTEXFORMAT_UBYTE4;
    }
    break;
  case cgltf_component_type_r_16:
    switch (acc->type) {
    case cgltf_type_vec2:
      return acc->normalized ? SG_VERTEXFORMAT_SHORT2N : SG_VERTEXFORMAT_SHORT2;
    case cgltf_type_vec4:
      return acc->normalized ? SG_VERTEXFORMAT_SHORT4N : SG_VERTEXFORMAT_SHORT4;
    default:
      break;
    }
    break;
  case cgltf_component_type_r_32f:
    switch (acc->type) {
    case cgltf_type_scalar:
      return SG_VERTEXFORMAT_FLOAT;
    case cgltf_type_vec2:
      return SG_VERTEXFORMAT_FLOAT2;
    case cgltf_type_vec3:
      return SG_VERTEXFORMAT_FLOAT3;
    case cgltf_type_vec4:
      return SG_VERTEXFORMAT_FLOAT4;
    default:
      break;
    }
    break;
  default:
    break;
  }
  return SG_VERTEXFORMAT_INVALID;
}

static sg_index_type gltf_to_index_type(const cgltf_primitive *prim)
{
  if (prim->indices) {
    if (prim->indices->component_type == cgltf_component_type_r_16u) {
      return SG_INDEXTYPE_UINT16;
    } else {
      return SG_INDEXTYPE_UINT32;
    }
  } else {
    return SG_INDEXTYPE_NONE;
  }
}

static void load_gltf_meshes(cgltf_data *gltf, int32_t buffer_base_idx)
{
  assert(gltf->meshes);

  printf("load_gltf_meshes\n");

  for (int32_t i = 0, ilen = gltf->meshes_count; i < ilen; ++i) {
    struct mesh *mesh = &meshes[mesh_count++];
    assert(mesh_count < MAX_MESH_COUNT);

    mesh->submesh_start_idx = submesh_count;
    mesh->submesh_end_idx = submesh_count + gltf->meshes[i].primitives_count;

    printf("-- meshes[%d] <= gltf mesh %d (submeshes %d - %d (#%d))\n", mesh_count - 1, i, mesh->submesh_start_idx, mesh->submesh_end_idx, (int32_t)gltf->meshes[i].primitives_count);

    for (int32_t j = 0, jlen = gltf->meshes[i].primitives_count; j < jlen; ++j) {
      cgltf_primitive *prim = &gltf->meshes[i].primitives[j];

      struct submesh *submesh = &submeshes[submesh_count++];
      assert(submesh_count < MAX_SUBMESH_COUNT);

      cgltf_attribute *attrs = prim->attributes;
      for (int32_t k = 0, klen = prim->attributes_count; k < klen; ++k) {
        cgltf_accessor *acc = prim->attributes[k].data;
        int32_t acc_offset = (int32_t)acc->offset;
        int32_t buffer_view_idx = (int32_t)((cgltf_buffer_view *)acc->buffer_view - (cgltf_buffer_view *)gltf->buffer_views);
        submesh->buffer_indices[k] = buffer_base_idx + buffer_view_idx;
        submesh->buffer_offsets[k] = acc_offset;
      }
      cgltf_accessor *indices = prim->indices;
      int32_t ibuffer_view_idx = (int32_t)((cgltf_buffer_view *)indices->buffer_view - (cgltf_buffer_view *)gltf->buffer_views);
      submesh->buffer_indices[3] = buffer_base_idx + ibuffer_view_idx;
      submesh->buffer_offsets[3] = (int32_t)indices->offset;

      submesh->element_count = prim->indices->count;

      submesh->pipeline_idx = pipeline_count;
      pipelines[pipeline_count++] = sg_make_pipeline(&(sg_pipeline_desc){
        .layout = {
          .buffers = {
            [0].stride = 12,
            [1].stride = 12,
            [2].stride = 8},
          .attrs = {
            [ATTR_vs_position] = {
              .format = SG_VERTEXFORMAT_FLOAT3,
              .offset = 0,
              .buffer_index = 0,
            },
            [ATTR_vs_normal] = {
              .format = SG_VERTEXFORMAT_FLOAT3,
              .offset = 0,
              .buffer_index = 1,
            },
            [ATTR_vs_texcoord] = {
              .format = SG_VERTEXFORMAT_FLOAT2,
              .offset = 0,
              .buffer_index = 2,
            },
          },
        },
        .shader = shader,
        .index_type = SG_INDEXTYPE_UINT16,
        .depth_stencil = {
          .depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL,
          .depth_write_enabled = true,
        },
        .rasterizer.sample_count = SAMPLE_COUNT,
      });
      assert(pipeline_count < MAX_SUBMESH_COUNT);
      printf("-- -- submesh[%d] <= gltf primitive %d (pipeline_idx = %d)\n", submesh_count - 1, j, pipeline_count - 1);
    }
  }
}

static void load_gltf_buffers(cgltf_data *gltf)
{
  assert(gltf->buffers && gltf->buffer_views && gltf->accessors);
  printf("load_gltf_buffers\n");

  for (int32_t i = 0, len = gltf->buffer_views_count; i < len; ++i) {
    cgltf_buffer_view *buf_view = &gltf->buffer_views[i];

    int32_t buffer_idx = (int32_t)((cgltf_buffer *)buf_view->buffer - (cgltf_buffer *)gltf->buffers);

    printf("-- buffers[%d] <= gltf buffer_view %d (buffer %d)\n", buffer_count, i, buffer_idx);
    buffers[buffer_count++] = sg_make_buffer(&(sg_buffer_desc){
      .type = (buf_view->type == cgltf_buffer_view_type_indices) ? SG_BUFFERTYPE_INDEXBUFFER : SG_BUFFERTYPE_VERTEXBUFFER,
      .size = buf_view->size,
      .content = (uint8_t *)(gltf->buffers[buffer_idx].data) + buf_view->offset});
    assert(buffer_count < MAX_BUFFER_COUNT);
  }
  return;
}

static void load_gltf(const char *filename)
{
  struct buffer gltf_buffer = buffer_create_from_file(filename);
  printf("load gltf file %s %d\n", filename, gltf_buffer.size);

  cgltf_options options = {0};
  cgltf_data *gltf = NULL;
  const cgltf_result parse_result = cgltf_parse(&options, gltf_buffer.data, gltf_buffer.size, &gltf);
  assert(parse_result == cgltf_result_success);

  const cgltf_result load_buf_result = cgltf_load_buffers(&options, gltf, NULL);
  assert(load_buf_result == cgltf_result_success);

  int32_t buffer_base_idx = buffer_count;
  load_gltf_buffers(gltf);
  load_gltf_meshes(gltf, buffer_base_idx);

  cgltf_free(gltf);
  buffer_destroy(&gltf_buffer);

  return;
}

static void init(void)
{
  sg_setup(&(sg_desc){
    .gl_force_gles2 = sapp_gles2(), .mtl_device = sapp_metal_get_device(), .mtl_renderpass_descriptor_cb = sapp_metal_get_renderpass_descriptor, .mtl_drawable_cb = sapp_metal_get_drawable});

  shader = sg_make_shader(demo_shader_desc());

  /* load gltf files */
  load_gltf("assets/toob.gltf");
  load_gltf("assets/plus.gltf");
  load_gltf("assets/toob.gltf");
  load_gltf("assets/reggie.gltf");

  for (int32_t i = 0, ilen = mesh_count; i < ilen; ++i) {
    float scale_factor = (float)(i + 1.0f) * 0.5f;
    entities[entity_count++] = (struct entity){
      .mesh_idx = i,
      .position = {
        .x = -((float)mesh_count * 10.0f / 2.0f) + ((float)i * 10.f) + 5.0f,
        .y = 0.0f,
        .z = 0.0f,
      },
      .rotation = {
        .x = -90.0f,
        .y = 0.0f,
        .z = 0.0f,
      },
      .scale = {
        .x = scale_factor,
        .y = scale_factor,
        .z = scale_factor,
      },
    };
    assert(entity_count < MAX_ENTITY_COUNT);
  }
}

void cleanup(void)
{
  sg_shutdown();
}

static void event(const sapp_event *e)
{
  int32_t event_type = e->type;
  assert((event_type >= 0) && (event_type < _SAPP_EVENTTYPE_NUM));
  if (event_type == SAPP_EVENTTYPE_KEY_UP || event_type == SAPP_EVENTTYPE_KEY_DOWN) {
    int32_t is_down = event_type == SAPP_EVENTTYPE_KEY_DOWN;
    switch (e->key_code) {
    case SAPP_KEYCODE_UP:
      input_button_process(&input_state.up, is_down);
      break;
    case SAPP_KEYCODE_DOWN:
      input_button_process(&input_state.down, is_down);
      break;
    case SAPP_KEYCODE_LEFT:
      input_button_process(&input_state.left, is_down);
      break;
    case SAPP_KEYCODE_RIGHT:
      input_button_process(&input_state.right, is_down);
      break;
    case SAPP_KEYCODE_Q:
      input_button_process(&input_state.quit, is_down);
      break;
    default:
      break;
    }
  }
}

static void process_input(struct input *input_state)
{
  struct entity *entity = &entities[1];

  if (input_state->quit.is_down) {
    cleanup();
    exit(0);
  }

  if (input_state->left.is_down || input_state->right.is_down) {
    entity->rotation.z += 5.0f * (input_state->right.is_down ? -1.0f : 1.0f);
  }

  if (input_state->up.is_down || input_state->down.is_down) {
    int32_t is_up = input_state->up.is_down;
    float z_inc = cosf(WATT_RAD_FROM_DEG(entity->rotation.z));
    float x_inc = sinf(WATT_RAD_FROM_DEG(entity->rotation.z));
    float direction = (input_state->up.is_down ? 1.0f : -1.0f) * 0.5f;
    entity->position.x += x_inc * direction;
    entity->position.z += z_inc * direction;
  }
}

static void frame(void)
{
  static int32_t frame_count = 0;
  ++frame_count;

  process_input(&input_state);

  /* NOTE: the vs_params_t struct has been code-generated by the shader-code-gen */
  vs_params_t vs_params;
  const float w = (float)sapp_width();
  const float h = (float)sapp_height();

  struct vec3 camera_position = {
    .x = 0.0f,
    .y = 50.0f,
    .z = 50.0f,
  };

  struct vec3 camera_direction = vec3_add(v3(0.0f, 0.0f, 0.0f), vec3_scale(camera_position, -1.0f));

  struct mat4 proj = mat4_perspective(60.0f / 180.0f * WATT_PI32, w / h, 0.01f, 1000.0f);
  struct mat4 view = mat4_look_at(camera_position, camera_direction, v3(0.0f, 1.0f, 0.0f));
  struct mat4 view_proj = mat4_multiply(proj, view);

  sg_pass_action pass_action = {
    .colors[0] = {
      .action = SG_ACTION_CLEAR,
      .val = {0.25f, 0.5f, 0.75f, 1.0f},
    },
  };

  sg_begin_default_pass(&pass_action, (int32_t)w, (int32_t)h);

  if (frame_count == 1) printf("render\n");
  for (int32_t i = 0, ilen = entity_count; i < ilen; ++i) {
    struct entity entity = entities[i];
    struct mesh mesh = meshes[entity.mesh_idx];
    if (frame_count == 1) printf("-- mesh %d\n", entity.mesh_idx);

    // calc mvp
    struct mat4 translated = mat4_translate(mat4_identity(), entity.position);
    struct mat4 rotated_and_translated = mat4_rotate_z(
      mat4_rotate_y(
        mat4_rotate_x(
          translated,
          WATT_RAD_FROM_DEG(entity.rotation.x)),
        WATT_RAD_FROM_DEG(entity.rotation.y)),
      WATT_RAD_FROM_DEG(entity.rotation.z));
    struct mat4 model = mat4_scale(rotated_and_translated, v3(entity.scale.x, entity.scale.y, entity.scale.z));

    vs_params.mvp = mat4_multiply(view_proj, model);

    for (int32_t j = mesh.submesh_start_idx, jlen = mesh.submesh_end_idx; j < jlen; ++j) {
      struct submesh submesh = submeshes[j];
      if (frame_count == 1) printf("-- -- render submesh %d (pipeline_idx = %d)\n", j, submesh.pipeline_idx);
      if (frame_count == 1) printf("-- -- -- buffers %d %d %d %d\n", submesh.buffer_indices[0], submesh.buffer_indices[1], submesh.buffer_indices[2], submesh.buffer_indices[3]);

      sg_apply_pipeline(pipelines[submesh.pipeline_idx]);
      sg_bindings bindings = (sg_bindings){
        .vertex_buffers = {
          [0] = buffers[submesh.buffer_indices[0]],
          [1] = buffers[submesh.buffer_indices[1]],
          [2] = buffers[submesh.buffer_indices[2]],
        },
        .vertex_buffer_offsets = {
          [0] = submesh.buffer_offsets[0],
          [1] = submesh.buffer_offsets[1],
          [2] = submesh.buffer_offsets[2],
        },
        .index_buffer = buffers[submesh.buffer_indices[3]],
        .index_buffer_offset = submesh.buffer_offsets[3],
      };
      sg_apply_bindings(&bindings);

      sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));
      sg_draw(0, submesh.element_count, 1);
    }
  }
  sg_end_pass();
  sg_commit();
}

sapp_desc sokol_main(int argc, char *argv[])
{
  return (sapp_desc){
    .init_cb = init,
    .event_cb = event,
    .frame_cb = frame,
    .cleanup_cb = cleanup,
    .width = 800,
    .height = 600,
    .sample_count = SAMPLE_COUNT,
    .gl_force_gles2 = true,
    .window_title = "builder.town",
  };
}
