#define EV_MODULE_DEFINE
#include <evol/evolmod.h>

#define TYPE_MODULE evmod_ecs
#define NAMESPACE_MODULE evmod_ecs
#include <evol/meta/type_import.h>
#include <evol/meta/namespace_import.h>

struct evGameData {
  evolmodule_t ecs_module;
  ECSComponentID transformComponentID;
  ECSComponentID worldTransformComponentID;
  ECSTagID dirtyTransformTagID;
} GameData;

typedef struct {
  Vec3 position;
  Vec4 rotation;
  Vec3 scale;
} TransformComponent;

Matrix4x4 *
_ev_object_getworldtransform(
    ObjectID entt);

void
init_scripting_api();

void
transform_setdirty(
    ECSEntityID entt)
{
  if(ECS->hasTag(entt, GameData.dirtyTransformTagID)) {
    return;
  }

  /* printf("Set Entity #%llu as dirty\n", entt); */

  ECS->addTag(entt, GameData.dirtyTransformTagID);
  ECS->forEachChild(entt, transform_setdirty);
}

void
_ev_object_setrotation(
    ECSEntityID entt, 
    Vec4 new_rot)
{
  TransformComponent *tr = ECS->getComponent(entt, GameData.transformComponentID);
  tr->rotation = new_rot;
  transform_setdirty(entt);
}

void
_ev_object_setposition(
    ECSEntityID entt, 
    Vec3 new_pos)
{
  TransformComponent *tr = ECS->getComponent(entt, GameData.transformComponentID);
  tr->position = new_pos;
  transform_setdirty(entt);
}

Vec3
_ev_object_getrotationeuler(
    ECSEntityID entt)
{
  Matrix4x4 *rotationMatrix = _ev_object_getworldtransform(entt);
  Vec3 res;
  glm_euler_angles(*rotationMatrix, &res);
  return res;
}

void
_ev_object_setrotationeuler(
    ECSEntityID entt,
    Vec3 new_angles)
{
  TransformComponent *tr = ECS->getComponent(entt, GameData.transformComponentID);

  Matrix4x4 rotationMatrix;
  glm_euler(&new_angles, rotationMatrix);
  glm_mat4_quat(rotationMatrix, &(tr->rotation));

  transform_setdirty(entt);
}

void
_ev_object_setscale(
    ECSEntityID entt,
    Vec3 new_scale)
{
  TransformComponent *tr = ECS->getComponent(entt, GameData.transformComponentID);
  tr->scale = new_scale;
  transform_setdirty(entt);
}

Vec4
_ev_object_getrotation(
    ECSEntityID entt)
{
  TransformComponent *comp = ECS->getComponent(entt, GameData.transformComponentID);
  return comp->rotation;
}
Vec3
_ev_object_getposition(
    ECSEntityID entt)
{
  TransformComponent *comp = ECS->getComponent(entt, GameData.transformComponentID);
  return comp->position;
}
Vec3
_ev_object_getscale(
    ECSEntityID entt)
{
  TransformComponent *comp = ECS->getComponent(entt, GameData.transformComponentID);
  return comp->scale;
}

void
worldtransform_update(
    ECSEntityID entt)
{
  if(!ECS->hasTag(entt, GameData.dirtyTransformTagID)) {
    return;
  }

  Matrix4x4 *worldTransform = ECS->getComponent(entt, GameData.worldTransformComponentID);

  ECSEntityID parent = ECS->getParent(entt);
  if(parent != 0) {
    Matrix4x4 *parent_worldtransform = _ev_object_getworldtransform(parent);
    glm_mat4_dup(*parent_worldtransform, *worldTransform);
  } else {
    glm_mat4_identity(*worldTransform);
  }

  /* printf("Matrix4x4 for Entity %llu 's parent = \n", entt); */
  /* glm_mat4_print(*worldTransform, stdout); */

  TransformComponent *transform = ECS->getComponent(entt, GameData.transformComponentID);
  glm_translate(*worldTransform, (float*)&transform->position);
  glm_quat_rotate(*worldTransform, (float*)&transform->rotation, *worldTransform);
  glm_scale(*worldTransform, (float*)&transform->scale);

  ECS->removeTag(entt, GameData.dirtyTransformTagID);
}

Matrix4x4 *
_ev_object_getworldtransform(
    ObjectID entt)
{
  worldtransform_update(entt);
  return ECS->getComponent(entt, GameData.worldTransformComponentID);
}

void
_ev_object_setworldtransform(
    ObjectID obj_id,
    Matrix4x4 new_transform)
{
  ECSEntityID entt = (ECSEntityID)obj_id;
  Matrix4x4 *world_transform = ECS->getComponent(entt, GameData.worldTransformComponentID);
  glm_mat4_copy(new_transform, *world_transform);

  ECS->forEachChild(entt, transform_setdirty);
}

void
_ev_game_initecs()
{
  GameData.ecs_module = evol_loadmodule("ecs");
  if(GameData.ecs_module == NULL) {
    return;
  }

  IMPORT_NAMESPACE(ECS, GameData.ecs_module);

  GameData.transformComponentID = ECS->registerComponent("TransformComponent", sizeof(TransformComponent), EV_ALIGNOF(TransformComponent));
  GameData.worldTransformComponentID = ECS->registerComponent("WorldTransformComponent", sizeof(Matrix4x4), EV_ALIGNOF(Matrix4x4));
  GameData.dirtyTransformTagID = ECS->registerTag("DirtyTransform");
}

EV_CONSTRUCTOR 
{
  static_assert(sizeof(ObjectID) == sizeof(ECSEntityID));

  GameData.ecs_module = NULL;

  init_scripting_api();
}

EV_DESTRUCTOR 
{
  if(GameData.ecs_module) {
    evol_unloadmodule(GameData.ecs_module);
  }
}

EV_BINDINGS
{
  EV_NS_BIND_FN(Game, initECS, _ev_game_initecs);

  EV_NS_BIND_FN(Object, getWorldTransform, _ev_object_getworldtransform);
  EV_NS_BIND_FN(Object, setWorldTransform, _ev_object_setworldtransform);

  EV_NS_BIND_FN(Object, setPosition, _ev_object_setposition);
  EV_NS_BIND_FN(Object, setRotation, _ev_object_setrotation);
  EV_NS_BIND_FN(Object, setScale,    _ev_object_setscale);

  EV_NS_BIND_FN(Object, getPosition, _ev_object_getposition);
  EV_NS_BIND_FN(Object, getRotation, _ev_object_getrotation);
  EV_NS_BIND_FN(Object, getScale,    _ev_object_getscale);
}

void
_ev_object_getposition_wrapper(
    Vec3 *out,
    ECSEntityID *entt)
{
  *out = _ev_object_getposition(*entt);
}

void
_ev_object_setposition_wrapper(
    ECSEntityID *entt,
    Vec3 *new_pos)
{
  Vec3 pos = *new_pos; // Alignment
  _ev_object_setposition(*entt, pos);
}

void
_ev_object_setrotationeuler_wrapper(
    ECSEntityID *entt,
    Vec3 *new_rot)
{
  Vec3 angles = Vec3new(new_rot->x, new_rot->y, new_rot->z); // Alignment
  _ev_object_setrotationeuler(*entt, angles);
}

void
_ev_object_getrotationeuler_wrapper(
    Vec3 *out,
    ECSEntityID *entt)
{
  *out = _ev_object_getrotationeuler(*entt);
}

#define TYPE_MODULE evmod_script
#define NAMESPACE_MODULE evmod_script
#include <evol/meta/type_import.h>
#include <evol/meta/namespace_import.h>

void 
init_scripting_api()
{
  evolmodule_t scripting_module = evol_loadmodule("script");
  if(!scripting_module) return;
  IMPORT_NAMESPACE(ScriptInterface, scripting_module);

  ScriptType voidSType = ScriptInterface->getType("void");
  ScriptType floatSType = ScriptInterface->getType("float");
  ScriptType ullSType = ScriptInterface->getType("unsigned long long");

  ScriptType vec3SType = ScriptInterface->addStruct("Vec3", sizeof(Vec3), 3, (ScriptStructMember[]) {
      {"x", floatSType, offsetof(Vec3, x)},
      {"y", floatSType, offsetof(Vec3, y)},
      {"z", floatSType, offsetof(Vec3, z)}
  });

  ScriptInterface->addFunction(_ev_object_getposition_wrapper, "ev_object_getposition", vec3SType, 1, (ScriptType[]){ullSType});
  ScriptInterface->addFunction(_ev_object_setposition_wrapper, "ev_object_setposition", voidSType, 2, (ScriptType[]){ullSType, vec3SType});

  ScriptInterface->addFunction(_ev_object_getrotationeuler_wrapper, "ev_object_getrotationeuler", vec3SType, 1, (ScriptType[]){ullSType});
  ScriptInterface->addFunction(_ev_object_setrotationeuler_wrapper, "ev_object_setrotationeuler", voidSType, 2, (ScriptType[]){ullSType, vec3SType});

  /* ScriptInterface->addFunction(_ev_object_getscale_wrapper, "ev_object_getscale", vec3SType, 1, (ScriptType[]){ullSType}); */
  /* ScriptInterface->addFunction(_ev_object_setscale_wrapper, "ev_object_setscale", voidSType, 2, (ScriptType[]){ullSType, vec3SType}); */

  ScriptInterface->loadAPI("subprojects/evmod_game/script_api.lua");

  evol_unloadmodule(scripting_module);
  // Invalidating namespace reference as the module is unloaded
  ScriptInterface = NULL;
}
