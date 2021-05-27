#define EV_MODULE_DEFINE
#include <evol/evolmod.h>

#include "components/Transform.h"
#include "components/Camera.h"

#define IMPORT_MODULE evmod_ecs
#include <evol/meta/module_import.h>
#define IMPORT_MODULE evmod_physics
#include <evol/meta/module_import.h>

typedef struct {
  ECSGameWorldHandle ecs_world;
  PhysicsWorldHandle physics_world;
  GameObject activeCamera;
} GameSceneStruct;

void
gamescenestruct_destr(
    void *data)
{
  GameSceneStruct *scn = (GameSceneStruct *)data;
  GameECS->destroyWorld(scn->ecs_world);
  PhysicsWorld->destroyWorld(scn->physics_world);
}

struct evGameData {
  evolmodule_t ecs_module;
  evolmodule_t physics_module;
  vec(GameSceneStruct) scenes;
  GameScene activeScene;
} GameData;

void
ev_game_setactivescene(
    GameScene scene_handle)
{
  GameData.activeScene = scene_handle;
}

GameScene
ev_game_newscene()
{
  GameSceneStruct newscene = {
    .ecs_world = GameECS->newWorld(),
    .physics_world = PhysicsWorld->newWorld(),
    .activeCamera = 0
  };

  ev_log_trace("New game scene created: { .ecs_world = %llu, .physics_world = %llu, .activeCamera = %llu }",
      newscene.ecs_world,
      newscene.physics_world,
      newscene.activeCamera);

  GameScene newscene_handle = vec_push(&GameData.scenes, &newscene);
  ev_log_trace("New scene given handle { %llu }", newscene_handle);
  if(GameData.activeScene == 0) {
    GameData.activeScene = newscene_handle;
    ev_log_trace("Active scene set to { %llu }", GameData.activeScene);
  }

  return newscene_handle;
}

U32
ev_game_progress(
    F32 deltaTime)
{
  U32 result = 0;

  result |= GameECS->progress(GameData.scenes[GameData.activeScene].ecs_world, deltaTime);
  result |= PhysicsWorld->progress(GameData.scenes[GameData.activeScene].physics_world, deltaTime);

  return result;
}

Matrix4x4 *
_ev_object_getworldtransform(
    GameScene world,
    GameObject entt);

void
init_scripting_api();

void
transform_setdirty(
    ECSGameWorldHandle world,
    GameEntityID entt)
{
  if(GameECS->hasTag(world, entt, DirtyTransformTagID)) {
    return;
  }

  GameECS->addTag(world, entt, DirtyTransformTagID);
  GameECS->forEachChild(world, entt, transform_setdirty);
}

void
_ev_object_setrotation(
    GameScene scene_handle,
    GameObject entt, 
    Vec4 new_rot)
{
  GameSceneStruct scene = GameData.scenes[scene_handle?scene_handle:GameData.activeScene];
  TransformComponent *tr = GameECS->getComponent(scene.ecs_world, entt, TransformComponentID);
  tr->rotation = new_rot;
  transform_setdirty(scene.ecs_world, entt);
}

void
_ev_object_setposition(
    GameScene scene_handle,
    GameObject entt, 
    Vec3 new_pos)
{
  GameSceneStruct scene = GameData.scenes[scene_handle?scene_handle:GameData.activeScene];
  TransformComponent *tr = GameECS->getComponent(scene.ecs_world, entt, TransformComponentID);
  tr->position = new_pos;
  transform_setdirty(scene.ecs_world, entt);
}

void
ev_object_settransform(
    GameScene scene_handle,
    GameObject entt, 
    Vec3 new_pos,
    Vec4 new_rot,
    Vec3 new_scale)
{
  GameSceneStruct scene = GameData.scenes[scene_handle?scene_handle:GameData.activeScene];
  TransformComponent *tr = GameECS->getComponent(scene.ecs_world, entt, TransformComponentID);
  tr->position = new_pos;
  tr->rotation = new_rot;
  tr->scale = new_scale;

  transform_setdirty(scene.ecs_world, entt);
}

Vec3
_ev_object_getrotationeuler(
    GameScene scene_handle,
    GameObject entt)
{
  Matrix4x4 *rotationMatrix = _ev_object_getworldtransform(scene_handle, entt);
  Vec3 res;
  glm_euler_angles(*rotationMatrix, &res);
  return res;
}

void
_ev_object_setrotationeuler(
    GameScene scene_handle,
    GameObject entt,
    Vec3 new_angles)
{
  GameSceneStruct scene = GameData.scenes[scene_handle?scene_handle:GameData.activeScene];
  TransformComponent *tr = GameECS->getComponent(scene.ecs_world, entt, TransformComponentID);

  Matrix4x4 rotationMatrix;
  glm_euler(&new_angles, rotationMatrix);
  glm_mat4_quat(rotationMatrix, &(tr->rotation));

  transform_setdirty(scene.ecs_world, entt);
}

void
_ev_object_setscale(
    GameScene scene_handle,
    GameObject entt,
    Vec3 new_scale)
{
  GameSceneStruct scene = GameData.scenes[scene_handle?scene_handle:GameData.activeScene];
  TransformComponent *tr = GameECS->getComponent(scene.ecs_world, entt, TransformComponentID);
  tr->scale = new_scale;
  transform_setdirty(scene.ecs_world, entt);
}

Vec4
_ev_object_getrotation(
    GameScene scene_handle,
    GameObject entt)
{
  GameSceneStruct scene = GameData.scenes[scene_handle?scene_handle:GameData.activeScene];
  TransformComponent *comp = GameECS->getComponent(scene.ecs_world, entt, TransformComponentID);
  return comp->rotation;
}

Vec3
_ev_object_getposition(
    GameScene scene_handle,
    GameObject entt)
{
  GameSceneStruct scene = GameData.scenes[scene_handle?scene_handle:GameData.activeScene];
  TransformComponent *comp = GameECS->getComponent(scene.ecs_world, entt, TransformComponentID);
  return comp->position;
}

Vec3
_ev_object_getscale(
    GameScene scene_handle,
    GameObject entt)
{
  GameSceneStruct scene = GameData.scenes[scene_handle?scene_handle:GameData.activeScene];
  TransformComponent *comp = GameECS->getComponent(scene.ecs_world, entt, TransformComponentID);
  return comp->scale;
}

void
worldtransform_update(
    GameScene scene_handle,
    GameObject entt)
{
  GameSceneStruct scene = GameData.scenes[scene_handle?scene_handle:GameData.activeScene];
  if(!GameECS->hasTag(scene.ecs_world, entt, DirtyTransformTagID)) {
    return;
  }

  WorldTransformComponent *worldTransform = GameECS->getComponent(scene.ecs_world, entt, WorldTransformComponentID);

  GameObject parent = GameECS->getParent(scene.ecs_world, entt);
  if(parent != 0) {
    Matrix4x4 *parent_worldtransform = _ev_object_getworldtransform(scene.ecs_world, parent);
    glm_mat4_dup(*parent_worldtransform, *worldTransform);
  } else {
    glm_mat4_identity(*worldTransform);
  }

  TransformComponent *transform = GameECS->getComponent(scene.ecs_world, entt, TransformComponentID);
  glm_translate(*worldTransform, (float*)&transform->position);
  glm_quat_rotate(*worldTransform, (float*)&transform->rotation, *worldTransform);
  glm_scale(*worldTransform, (float*)&transform->scale);

  GameECS->removeTag(scene.ecs_world, entt, DirtyTransformTagID);
}

Matrix4x4 *
_ev_object_getworldtransform(
    GameScene scene_handle,
    GameObject entt)
{
  GameSceneStruct scene = GameData.scenes[scene_handle?scene_handle:GameData.activeScene];
  worldtransform_update(scene_handle, entt);
  return GameECS->getComponent(scene.ecs_world, entt, WorldTransformComponentID);
}

ECSGameWorldHandle
ev_scene_getecsworld(
    GameScene scene_handle)
{
  GameSceneStruct scene = GameData.scenes[scene_handle?scene_handle:GameData.activeScene];
  return scene.ecs_world;
}

PhysicsWorldHandle
ev_scene_getphysicsworld(
    GameScene scene_handle)
{
  GameSceneStruct scene = GameData.scenes[scene_handle?scene_handle:GameData.activeScene];
  return scene.physics_world;
}

void
_ev_camera_sethfov(
  GameScene scene_handle,
  GameObject camera,
  U32 hfov)
{
  GameSceneStruct scene = GameData.scenes[scene_handle?scene_handle:GameData.activeScene];
  camera = camera?camera:scene.activeCamera;
  CameraComponent *comp = GameECS->getComponent(scene.ecs_world, camera, CameraComponentID);
  if(comp->hfov != hfov) {
    comp->hfov = hfov;
    GameECS->modified(scene.ecs_world, camera, CameraComponentID);
  }
}

void
_ev_camera_setvfov(
  GameScene scene_handle,
  GameObject camera,
  U32 vfov)
{
  GameSceneStruct scene = GameData.scenes[scene_handle?scene_handle:GameData.activeScene];
  camera = camera?camera:scene.activeCamera;
  CameraComponent *comp = GameECS->getComponent(scene.ecs_world, camera, CameraComponentID);
  if(comp->vfov != vfov) {
    comp->vfov = vfov;
    GameECS->modified(scene.ecs_world, camera, CameraComponentID);
  }
}

void
_ev_camera_setaspectratio(
  GameScene scene_handle,
  GameObject camera,
  F32 aspectRatio)
{
  GameSceneStruct scene = GameData.scenes[scene_handle?scene_handle:GameData.activeScene];
  camera = camera?camera:scene.activeCamera;
  CameraComponent *comp = GameECS->getComponent(scene.ecs_world, camera, CameraComponentID);
  if(comp->aspectRatio != aspectRatio) {
    comp->aspectRatio = aspectRatio;
    GameECS->modified(scene.ecs_world, camera, CameraComponentID);
  }
}

void
_ev_camera_setnearplane(
  GameScene scene_handle,
  GameObject camera,
  F32 nearPlane)
{
  GameSceneStruct scene = GameData.scenes[scene_handle?scene_handle:GameData.activeScene];
  camera = camera?camera:scene.activeCamera;
  CameraComponent *comp = GameECS->getComponent(scene.ecs_world, camera, CameraComponentID);
  if(comp->nearPlane != nearPlane) {
    comp->nearPlane = nearPlane;
    GameECS->modified(scene.ecs_world, camera, CameraComponentID);
  }
}

void
_ev_camera_setfarplane(
  GameScene scene_handle,
  GameObject camera,
  F32 farPlane)
{
  GameSceneStruct scene = GameData.scenes[scene_handle?scene_handle:GameData.activeScene];
  camera = camera?camera:scene.activeCamera;
  CameraComponent *comp = GameECS->getComponent(scene.ecs_world, camera, CameraComponentID);
  if(comp->farPlane != farPlane) {
    comp->farPlane = farPlane;
    GameECS->modified(scene.ecs_world, camera, CameraComponentID);
  }
}

void
_ev_camera_getviewmat(
  GameScene scene_handle,
  GameObject camera,
  Matrix4x4 outViewMat)
{
  camera = camera?camera:GameData.scenes[scene_handle?scene_handle:GameData.activeScene].activeCamera;
  Matrix4x4 *transform = _ev_object_getworldtransform(scene_handle, camera);
  glm_mat4_inv(*transform, outViewMat);
}

void
_ev_camera_getprojectionmat(
  GameScene scene_handle,
  GameObject camera,
  Matrix4x4 outProjMat)
{
  GameSceneStruct scene = GameData.scenes[scene_handle?scene_handle:GameData.activeScene];
  camera = camera?camera:scene.activeCamera;
  CameraComponent *comp = GameECS->getComponent(scene.ecs_world, camera, CameraComponentID);
  glm_mat4_dup(comp->projectionMatrix, outProjMat);
}

void
_ev_object_setworldtransform(
    GameScene scene_handle,
    GameObject entt,
    Matrix4x4 new_transform)
{
  GameSceneStruct scene = GameData.scenes[scene_handle?scene_handle:GameData.activeScene];
  WorldTransformComponent *world_transform = GameECS->getComponent(scene.ecs_world, entt, WorldTransformComponentID);
  glm_mat4_copy(new_transform, *world_transform);

  GameECS->forEachChild(scene.ecs_world, entt, transform_setdirty);
  ev_log_trace("World transform set for Entity { %llu } in GameScene { %llu }", entt, scene_handle);
}

PTR
ev_object_getcomponent(
    GameScene scene_handle,
    GameObject entt,
    GameComponentID comp_id)
{
  ECSGameWorldHandle ecs_world = ev_scene_getecsworld(scene_handle);
  return GameECS->getComponent(ecs_world, entt, comp_id);
}

bool
ev_object_hascomponent(
    GameScene scene_handle,
    GameObject entt,
    GameComponentID comp_id)
{
  ECSGameWorldHandle ecs_world = ev_scene_getecsworld(scene_handle);
  return GameECS->hasComponent(ecs_world, entt, comp_id);
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

GameObject
ev_scene_createobject(
    GameScene scene_handle)
{
  GameSceneStruct scene = GameData.scenes[scene_handle?scene_handle:GameData.activeScene];
  GameObject object = GameECS->createEntity(scene.ecs_world);

  ev_object_settransform(scene_handle, object, 
    Vec3new(0, 0, 0),     // Position
    Vec4new(0, 0, 0, 1),  // Rotation
    Vec3new(1, 1, 1)      // Scale
  );

  return object;
}

GameObject
ev_scene_createchildobject(
    GameScene scene_handle,
    GameObject parent)
{
  GameSceneStruct scene = GameData.scenes[scene_handle?scene_handle:GameData.activeScene];
  GameObject object = GameECS->createChildEntity(scene.ecs_world, parent);

  ev_object_settransform(scene_handle, object, 
    Vec3new(0, 0, 0),     // Position
    Vec4new(0, 0, 0, 1),  // Rotation
    Vec3new(1, 1, 1)      // Scale
  );

  return object;
}

void
ev_scene_addchildtoobject(
    GameScene scene_handle,
    GameObject parent,
    GameObject child)
{
  GameSceneStruct scene = GameData.scenes[scene_handle?scene_handle:GameData.activeScene];
  GameECS->addChildToEntity(scene.ecs_world, parent, child);
}

void
ev_scene_destroyobject(
    GameScene scene_handle,
    GameObject object)
{
  GameSceneStruct scene = GameData.scenes[scene_handle?scene_handle:GameData.activeScene];
  GameECS->destroyEntity(scene.ecs_world, object);
}

GameObject
ev_scene_getactivecamera(
    GameScene scene_handle)
{
  GameSceneStruct scene = GameData.scenes[scene_handle?scene_handle:GameData.activeScene];
  return scene.activeCamera;
}

void
ev_scene_setactivecamera(
    GameScene scene_handle,
    GameObject camera)
{
  GameData.scenes[scene_handle?scene_handle:GameData.activeScene].activeCamera = camera;
}

GameObject
ev_scene_createcamera(
    GameScene scene_handle,
    CameraViewType type)
{
  GameSceneStruct scene = GameData.scenes[scene_handle?scene_handle:GameData.activeScene];
  GameObject camera = ev_scene_createobject(scene_handle);
  GameECS->setComponent(scene.ecs_world, camera, CameraComponentID, &(CameraComponent) {
      .viewType = type,
  });

  if(scene.activeCamera == 0) {
    GameData.scenes[scene_handle?scene_handle:GameData.activeScene].activeCamera = camera;
  }

  return camera;
}


EV_CONSTRUCTOR 
{
  static_assert(sizeof(GameObject) == sizeof(ECSEntityID), "ObjectID not the same size as ECSEntityID");

  GameData.scenes = vec_init(GameSceneStruct, NULL, gamescenestruct_destr);
  vec_setlen((vec_t*)&GameData.scenes, 1);
  GameData.scenes[0] = (GameSceneStruct){NULL};
  GameData.activeScene = 0;

  GameData.ecs_module = evol_loadmodule("ecs");
  if(GameData.ecs_module) {
    imports(GameData.ecs_module, (ECS, GameECS));

    if(GameECS) {
      TransformComponentID = GameECS->registerComponent(TransformComponentName, sizeof(TransformComponent), EV_ALIGNOF(TransformComponent));
      WorldTransformComponentID = GameECS->registerComponent(WorldTransformComponentName, sizeof(WorldTransformComponent), EV_ALIGNOF(WorldTransformComponent));
      DirtyTransformTagID = GameECS->registerTag(DirtyTransformTagName);

      CameraComponentID = GameECS->registerComponent(CameraComponentName, sizeof(CameraComponent), EV_ALIGNOF(CameraComponent));

      GameECS->setOnAddTrigger("CameraComponentOnAddTrigger", CameraComponentName, CameraComponentOnAddTrigger);
      GameECS->setOnSetTrigger("CameraComponentOnSetTrigger", CameraComponentName, CameraComponentOnSetTrigger);
    }
  }

  GameData.physics_module = evol_loadmodule("physics");
  if(GameData.physics_module) {
    imports(GameData.physics_module, (PhysicsWorld));
  }

  init_scripting_api();
}

EV_DESTRUCTOR 
{
  vec_fini(GameData.scenes);

  if(GameData.ecs_module) {
    evol_unloadmodule(GameData.ecs_module);
  }

  if(GameData.physics_module) {
    evol_unloadmodule(GameData.physics_module);
  }
}

EV_BINDINGS
{
  EV_NS_BIND_FN(Game, newScene, ev_game_newscene);
  EV_NS_BIND_FN(Game, setActiveScene, ev_game_setactivescene);
  EV_NS_BIND_FN(Game, progress, ev_game_progress);

  EV_NS_BIND_FN(Scene, createObject, ev_scene_createobject);
  EV_NS_BIND_FN(Scene, createChildObject, ev_scene_createchildobject);
  EV_NS_BIND_FN(Scene, addChildToObject, ev_scene_addchildtoobject);
  EV_NS_BIND_FN(Scene, destroyObject, ev_scene_destroyobject);
  EV_NS_BIND_FN(Scene, createCamera, ev_scene_createcamera);
  EV_NS_BIND_FN(Scene, getECSWorld, ev_scene_getecsworld);
  EV_NS_BIND_FN(Scene, getPhysicsWorld, ev_scene_getphysicsworld);
  EV_NS_BIND_FN(Scene, getActiveCamera, ev_scene_getactivecamera);
  EV_NS_BIND_FN(Scene, setActiveCamera, ev_scene_setactivecamera);

  EV_NS_BIND_FN(Object, getWorldTransform, _ev_object_getworldtransform);
  EV_NS_BIND_FN(Object, setWorldTransform, _ev_object_setworldtransform);
  EV_NS_BIND_FN(Object, setPosition, _ev_object_setposition);
  EV_NS_BIND_FN(Object, setRotation, _ev_object_setrotation);
  EV_NS_BIND_FN(Object, setScale,    _ev_object_setscale);
  EV_NS_BIND_FN(Object, getPosition, _ev_object_getposition);
  EV_NS_BIND_FN(Object, getRotation, _ev_object_getrotation);
  EV_NS_BIND_FN(Object, getScale,    _ev_object_getscale);

  // ECS shortcuts
  EV_NS_BIND_FN(Object, getComponent, ev_object_getcomponent);
  EV_NS_BIND_FN(Object, hasComponent, ev_object_hascomponent);

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
  Vec3 res = _ev_object_getposition(NULL, *entt);
  out->x = res.x;
  out->y = res.y;
  out->z = res.z;
}

void
_ev_object_setposition_wrapper(
    EV_UNALIGNED ECSEntityID *entt,
    EV_UNALIGNED Vec3 *new_pos)
{
  _ev_object_setposition(NULL, *entt, Vec3new(
        new_pos->x, 
        new_pos->y, 
        new_pos->z));
}

void
_ev_object_setrotationeuler_wrapper(
    EV_UNALIGNED ECSEntityID *entt,
    EV_UNALIGNED Vec3 *new_rot)
{
  _ev_object_setrotationeuler(NULL, *entt, Vec3new(
        new_rot->x,
        new_rot->y,
        new_rot->z));
}

void
_ev_object_getrotationeuler_wrapper(
    EV_UNALIGNED Vec3 *out,
    EV_UNALIGNED ECSEntityID *entt)
{
  Vec3 res = _ev_object_getrotationeuler(NULL, *entt);
  out->x = res.x;
  out->y = res.y;
  out->z = res.z;
}

#define IMPORT_MODULE evmod_script
#include <evol/meta/module_import.h>

void 
init_scripting_api()
{
  evolmodule_t scripting_module = evol_loadmodule("script");
  if(!scripting_module) return;
  imports(scripting_module, (ScriptInterface));

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
