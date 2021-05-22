#define EV_MODULE_DEFINE
#include <evol/evolmod.h>

#include "components/Transform.h"
#include "components/Camera.h"

#define TYPE_MODULE evmod_ecs
#define NAMESPACE_MODULE evmod_ecs
#include <evol/meta/type_import.h>
#include <evol/meta/namespace_import.h>

struct evGameData {
  evolmodule_t ecs_module;
} GameData;

Matrix4x4 *
_ev_object_getworldtransform(
    ObjectID entt);

void
init_scripting_api();

void
transform_setdirty(
    ECSEntityID entt)
{
  if(ECS->hasTag(entt, DirtyTransformTagID)) {
    return;
  }

  ECS->addTag(entt, DirtyTransformTagID);
  ECS->forEachChild(entt, transform_setdirty);
}

void
_ev_object_setrotation(
    ECSEntityID entt, 
    Vec4 new_rot)
{
  TransformComponent *tr = ECS->getComponent(entt, TransformComponentID);
  tr->rotation = new_rot;
  transform_setdirty(entt);
}

void
_ev_object_setposition(
    ECSEntityID entt, 
    Vec3 new_pos)
{
  TransformComponent *tr = ECS->getComponent(entt, TransformComponentID);
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
  TransformComponent *tr = ECS->getComponent(entt, TransformComponentID);

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
  TransformComponent *tr = ECS->getComponent(entt, TransformComponentID);
  tr->scale = new_scale;
  transform_setdirty(entt);
}

Vec4
_ev_object_getrotation(
    ECSEntityID entt)
{
  TransformComponent *comp = ECS->getComponent(entt, TransformComponentID);
  return comp->rotation;
}
Vec3
_ev_object_getposition(
    ECSEntityID entt)
{
  TransformComponent *comp = ECS->getComponent(entt, TransformComponentID);
  return comp->position;
}
Vec3
_ev_object_getscale(
    ECSEntityID entt)
{
  TransformComponent *comp = ECS->getComponent(entt, TransformComponentID);
  return comp->scale;
}

void
worldtransform_update(
    ECSEntityID entt)
{
  if(!ECS->hasTag(entt, DirtyTransformTagID)) {
    return;
  }

  WorldTransformComponent *worldTransform = ECS->getComponent(entt, WorldTransformComponentID);

  ECSEntityID parent = ECS->getParent(entt);
  if(parent != 0) {
    Matrix4x4 *parent_worldtransform = _ev_object_getworldtransform(parent);
    glm_mat4_dup(*parent_worldtransform, *worldTransform);
  } else {
    glm_mat4_identity(*worldTransform);
  }

  TransformComponent *transform = ECS->getComponent(entt, TransformComponentID);
  glm_translate(*worldTransform, (float*)&transform->position);
  glm_quat_rotate(*worldTransform, (float*)&transform->rotation, *worldTransform);
  glm_scale(*worldTransform, (float*)&transform->scale);

  ECS->removeTag(entt, DirtyTransformTagID);
}

Matrix4x4 *
_ev_object_getworldtransform(
    ObjectID entt)
{
  worldtransform_update(entt);
  return ECS->getComponent(entt, WorldTransformComponentID);
}

ObjectID
_ev_camera_getactive()
{
  return ((MainCamera*)ECS->getSingleton(MainCameraID))->entt;
}

void
_ev_camera_setactive(
    ObjectID camera)
{
  ECS->setSingleton(MainCameraID, sizeof(MainCamera), &(MainCamera) {camera});
}

ObjectID
_ev_camera_create(CameraViewType type)
{
  ECSEntityID cameraID = ECS->createEntity();
  ECS->setComponent(cameraID, CameraComponentID, sizeof(CameraComponent), &(CameraComponent) {
      .viewType = type,
  });

  if(_ev_camera_getactive() == 0) {
    _ev_camera_setactive(cameraID);
  }

  return (ObjectID)cameraID;
}

void
_ev_camera_sethfov(
  ObjectID camera,
  U32 hfov)
{
  CameraComponent *comp = ECS->getComponent(camera, CameraComponentID);
  if(comp->hfov != hfov) {
    comp->hfov = hfov;
    ECS->modified(camera,CameraComponentID);
  }
}

void
_ev_camera_setvfov(
  ObjectID camera,
  U32 vfov)
{
  CameraComponent *comp = ECS->getComponent(camera, CameraComponentID);
  if(comp->vfov != vfov) {
    comp->vfov = vfov;
    ECS->modified(camera,CameraComponentID);
  }
}

void
_ev_camera_setaspectratio(
  ObjectID camera,
  F32 aspectRatio)
{
  CameraComponent *comp = ECS->getComponent(camera, CameraComponentID);
  if(comp->aspectRatio != aspectRatio) {
    comp->aspectRatio = aspectRatio;
    ECS->modified(camera,CameraComponentID);
  }
}

void
_ev_camera_setnearplane(
  ObjectID camera,
  F32 nearPlane)
{
  CameraComponent *comp = ECS->getComponent(camera, CameraComponentID);
  if(comp->nearPlane != nearPlane) {
    comp->nearPlane = nearPlane;
    ECS->modified(camera,CameraComponentID);
  }
}

void
_ev_camera_setfarplane(
  ObjectID camera,
  F32 farPlane)
{
  CameraComponent *comp = ECS->getComponent(camera, CameraComponentID);
  if(comp->farPlane != farPlane) {
    comp->farPlane = farPlane;
    ECS->modified(camera,CameraComponentID);
  }
}

void
_ev_camera_getviewmat(
  ObjectID camera,
  Matrix4x4 outViewMat)
{
  Matrix4x4 *transform = _ev_object_getworldtransform(camera);
  glm_mat4_inv(*transform, outViewMat);
}

void
_ev_camera_getprojectionmat(
  ObjectID camera,
  Matrix4x4 outProjMat)
{
  CameraComponent *comp = ECS->getComponent(camera, CameraComponentID);
  glm_mat4_dup(comp->projectionMatrix, outProjMat);
}

void
_ev_object_setworldtransform(
    ObjectID obj_id,
    Matrix4x4 new_transform)
{
  ECSEntityID entt = (ECSEntityID)obj_id;
  WorldTransformComponent *world_transform = ECS->getComponent(entt, WorldTransformComponentID);
  glm_mat4_copy(new_transform, *world_transform);

  ECS->forEachChild(entt, transform_setdirty);
}

void 
CameraComponentOnAddTrigger(ECSQuery query)
{
  CameraComponent *cameraComp = ECS->getQueryColumn(query, sizeof(CameraComponent), 1);
  glm_mat4_identity(cameraComp->projectionMatrix);
}

void
CameraComponentOnSetTrigger(ECSQuery query)
{
  CameraComponent *cameraComp = ECS->getQueryColumn(query, sizeof(CameraComponent), 1);
  U32 count = ECS->getQueryMatchCount(query);

  for(int i = 0; i < count; i++) {
    if(cameraComp[i].viewType == EV_CAMERA_VIEWTYPE_PERSPECTIVE) {
      glm_perspective(glm_rad(cameraComp[i].hfov), cameraComp[i].aspectRatio, cameraComp[i].nearPlane, cameraComp[i].farPlane, cameraComp[i].projectionMatrix);
    } else if (cameraComp[i].viewType == EV_CAMERA_VIEWTYPE_ORTHOGRAPHIC){
      assert(!"Unimplemented: Orthographic Camera");
    } else {
      ev_log_error("Invalid camera view type. Choose one from: (EV_CAMERA_VIEWTYPE_PERSPECTIVE, EV_CAMERA_VIEWTYPE_ORTHOGRAPHIC)");
      assert(false);
    }
  }
}

void
_ev_game_initecs()
{
  if(ECS) {
    TransformComponentID = ECS->registerComponent(TransformComponentName, sizeof(TransformComponent), EV_ALIGNOF(TransformComponent));
    WorldTransformComponentID = ECS->registerComponent(WorldTransformComponentName, sizeof(WorldTransformComponent), EV_ALIGNOF(WorldTransformComponent));
    DirtyTransformTagID = ECS->registerTag(DirtyTransformTagName);

    CameraComponentID = ECS->registerComponent(CameraComponentName, sizeof(CameraComponent), EV_ALIGNOF(CameraComponent));
    MainCameraID = ECS->registerComponent(MainCameraName, sizeof(MainCamera), EV_ALIGNOF(MainCamera));

    _ev_camera_setactive(0);

    ECS->setOnAddTrigger("CameraComponentOnAddTrigger", CameraComponentName, CameraComponentOnAddTrigger);
    ECS->setOnSetTrigger("CameraComponentOnSetTrigger", CameraComponentName, CameraComponentOnSetTrigger);
  }
}

EV_CONSTRUCTOR 
{
  static_assert(sizeof(ObjectID) == sizeof(ECSEntityID), "ObjectID not the same size as ECSEntityID");

  GameData.ecs_module = evol_loadmodule("ecs");

  if(GameData.ecs_module) {
    IMPORT_NAMESPACE(ECS, GameData.ecs_module);
  }

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

  EV_NS_BIND_FN(Camera, create, _ev_camera_create);
  EV_NS_BIND_FN(Camera, getActive, _ev_camera_getactive);
  EV_NS_BIND_FN(Camera, setActive, _ev_camera_setactive);
  EV_NS_BIND_FN(Camera, setAspectRatio, _ev_camera_setaspectratio);
  EV_NS_BIND_FN(Camera, setHFOV, _ev_camera_sethfov);
  EV_NS_BIND_FN(Camera, setVFOV, _ev_camera_setvfov);
  EV_NS_BIND_FN(Camera, setNearPlane, _ev_camera_setnearplane);
  EV_NS_BIND_FN(Camera, setFarPlane, _ev_camera_setfarplane);
  EV_NS_BIND_FN(Camera, getViewMat, _ev_camera_getviewmat);
  EV_NS_BIND_FN(Camera, getProjectionMat, _ev_camera_getprojectionmat);
}

void
_ev_object_getposition_wrapper(
    EV_UNALIGNED Vec3 *out,
    EV_UNALIGNED ECSEntityID *entt)
{
  Vec3 res = _ev_object_getposition(*entt);
  out->x = res.x;
  out->y = res.y;
  out->z = res.z;
}

void
_ev_object_setposition_wrapper(
    EV_UNALIGNED ECSEntityID *entt,
    EV_UNALIGNED Vec3 *new_pos)
{
  _ev_object_setposition(*entt, Vec3new(
        new_pos->x, 
        new_pos->y, 
        new_pos->z));
}

void
_ev_object_setrotationeuler_wrapper(
    EV_UNALIGNED ECSEntityID *entt,
    EV_UNALIGNED Vec3 *new_rot)
{
  _ev_object_setrotationeuler(*entt, Vec3new(
        new_rot->x,
        new_rot->y,
        new_rot->z));
}

void
_ev_object_getrotationeuler_wrapper(
    EV_UNALIGNED Vec3 *out,
    EV_UNALIGNED ECSEntityID *entt)
{
  Vec3 res = _ev_object_getrotationeuler(*entt);
  out->x = res.x;
  out->y = res.y;
  out->z = res.z;
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
