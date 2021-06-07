#define EV_MODULE_DEFINE
#include <evol/evolmod.h>

#include "components/Transform.h"
#include "components/Camera.h"

#define IMPORT_MODULE evmod_ecs
#include <evol/meta/module_import.h>
#define IMPORT_MODULE evmod_physics
#include <evol/meta/module_import.h>
#define IMPORT_MODULE evmod_script
#include <evol/meta/module_import.h>
#define IMPORT_MODULE evmod_assets
#include IMPORT_MODULE_H

#include <evjson.h>

HashmapDefine(evstring, GameScene, evstring_free, NULL);

typedef struct {
  ECSGameWorldHandle ecs_world;
  PhysicsWorldHandle physics_world;
  ScriptContextHandle script_context;

  GameObject activeCamera;
} GameSceneStruct;

void
gamescenestruct_destr(
    void *data)
{
  GameSceneStruct *scn = (GameSceneStruct *)data;
  GameECS->destroyWorld(scn->ecs_world);
  PhysicsWorld->destroyWorld(scn->physics_world);
  ScriptContext->destroyContext(scn->script_context);
}

struct evGameData {
  evolmodule_t ecs_module;
  evolmodule_t physics_module;
  evolmodule_t script_module;
  evolmodule_t asset_module;
  vec(GameSceneStruct) scenes;
  Map(evstring, GameScene) scene_map;
  GameScene activeScene;
} GameData;

GameObject
ev_scene_getobject(
    GameScene scene_handle,
    CONST_STR name);
GameObject
ev_scene_getactivecamera(
    GameScene scene_handle);
GameObject
ev_scene_createchildobject(
    GameScene scene_handle,
    GameObject parent);

PhysicsWorldHandle
ev_scene_getphysicsworld(
    GameScene scene_handle);
ECSGameWorldHandle
ev_scene_getecsworld(
    GameScene scene_handle);

void
ev_game_setactivescene(
    GameScene scene_handle)
{
  GameData.activeScene = scene_handle;
}

GameScene
ev_scene_create()
{
  GameSceneStruct newscene = {
    .ecs_world = GameECS->newWorld(),
    .physics_world = PhysicsWorld->newWorld(),
    .script_context = ScriptContext->newContext(),

    .activeCamera = 0
  };

  ev_log_trace("New game scene created: { .ecs_world = %llu, .physics_world = %llu, .activeCamera = %llu, .script_context = %llu }",
      newscene.ecs_world,
      newscene.physics_world,
      newscene.activeCamera,
      newscene.script_context);

  GameScene newscene_handle = vec_push(&GameData.scenes, &newscene);
  ev_log_trace("New scene given handle { %llu }", newscene_handle);
  if(GameData.activeScene == 0) {
    GameData.activeScene = newscene_handle;
    ev_log_trace("Active scene set to { %llu }", GameData.activeScene);
  }

  return newscene_handle;
}

void
ev_sceneloader_loadtransformcomponent(
    GameScene scene,
    GameObject obj,
    evjson_t *json,
    evstring *comp_id)
{
  Vec3 position = {0};
  Vec3 rotation = {0};
  Vec3 scale = {0};

  evstring pos_id = evstring_newfmt("%s.position[x]", *comp_id);
  size_t pos_id_len = evstring_len(pos_id);
  for(char i = 0; i < 3; i++) {
    pos_id[pos_id_len-2] = '0' + i;
    ((float*)&position)[i] = (float)evjs_get(json, pos_id)->as_num;
  }

  evstring rot_id = evstring_newfmt("%s.rotation[x]", *comp_id);
  size_t rot_id_len = evstring_len(rot_id);
  for(char i = 0; i < 3; i++) {
    rot_id[rot_id_len-2] = '0'+i;
    ((float*)&rotation)[i] = (float)evjs_get(json,rot_id)->as_num;
  }

  evstring scale_id = evstring_newfmt("%s.scale[x]", *comp_id);
  size_t scale_id_len = evstring_len(scale_id);
  for(char i = 0; i < 3; i++) {
    scale_id[scale_id_len-2] = '0'+i;
    ((float*)&scale)[i] = (float)evjs_get(json,scale_id)->as_num;
  }

  evstring_free(scale_id);
  evstring_free(rot_id);
  evstring_free(pos_id);

  // Build rotation quaternion from loaded euler angles
  Vec4 rot_quat;
  Matrix4x4 rotationMatrix;
  glm_euler(&rotation, rotationMatrix);
  glm_mat4_quat(rotationMatrix, &rot_quat);

  ev_object_settransform(scene, obj, position, rot_quat, scale);
}

void
ev_sceneloader_loadscriptcomponent(
    GameScene scene,
    GameObject obj,
    evjson_t *json,
    evstring *comp_id)
{
  evstring scriptname_id = evstring_newfmt("%s.script_name", *comp_id);
  evstring scriptpath_id = evstring_newfmt("%s.script_path", *comp_id);
  evstring scriptname = evstring_refclone(evjs_get(json,scriptname_id)->as_str);
  evstring scriptpath = evstring_refclone(evjs_get(json,scriptpath_id)->as_str);

  AssetHandle script_asset = Asset->load(scriptpath);
  TextAsset script_text = TextLoader->loadAsset(script_asset);
  ScriptHandle script_handle = Script->new(scriptname, script_text.text);
  Asset->free(script_asset);

  Script->addToEntity(scene, obj, script_handle);

  evstring_free(scriptpath);
  evstring_free(scriptname);
  evstring_free(scriptname_id);
  evstring_free(scriptpath_id);
}

void
ev_sceneloader_loadcameracomponent(
    GameScene scene,
    GameObject obj,
    evjson_t *json,
    evstring *comp_id)
{
  evstring cameraview_id = evstring_newfmt("%s.view", *comp_id);
  evstr_ref cameraview = evjs_get(json,cameraview_id)->as_str;

  ECSGameWorldHandle ecs_world = ev_scene_getecsworld(scene);
  CameraComponent comp;

  switch(*(cameraview.data + cameraview.offset)) {
    case 'P':
      comp.viewType = EV_CAMERA_VIEWTYPE_PERSPECTIVE;

      evstring fov_id = evstring_newfmt("%s.fov", *comp_id);
      evstring near_id = evstring_newfmt("%s.near", *comp_id);
      evstring far_id = evstring_newfmt("%s.far", *comp_id);
      evstring aspectratio_id = evstring_newfmt("%s.aspectRatio", *comp_id);

      comp.hfov = (U32)evjs_get(json, fov_id)->as_num;
      comp.nearPlane = (F32)evjs_get(json, near_id)->as_num;
      comp.farPlane = (F32)evjs_get(json, far_id)->as_num;
      comp.aspectRatio = (F32)evjs_get(json, aspectratio_id)->as_num;

      evstring_free(fov_id);
      evstring_free(near_id);
      evstring_free(far_id);
      evstring_free(aspectratio_id);
      break;
    case 'O':
      comp.viewType = EV_CAMERA_VIEWTYPE_ORTHOGRAPHIC;
      // TODO Implement
      UNIMPLEMENTED();
      break;
    default:
      ev_log_error("Camera View type unknown `%.*s`", cameraview.len, cameraview.data + cameraview.offset);
      break;
  }

  GameECS->setComponent(ecs_world, obj, CameraComponentID, &comp);

  evstring_free(cameraview_id);
}

void
ev_sceneloader_loadrigidbodycomponent(
    GameScene scene,
    GameObject obj,
    evjson_t *json,
    evstring *comp_id)
{
  evstring SphereCollisionShapeSTR = evstring_new("Sphere");
  evstring BoxCollisionShapeSTR = evstring_new("Box");
  evstring CapsuleCollisionShapeSTR = evstring_new("Capsule");

  RigidbodyInfo info;

  evstring rigidbodytype_id = evstring_newfmt("%s.rigidbodyType", *comp_id);
  evstr_ref rigidbodyType = evjs_get(json,rigidbodytype_id)->as_str;
  switch(*(rigidbodyType.data + rigidbodyType.offset)) {
    case 'D':
      info.type = EV_RIGIDBODY_DYNAMIC;
      break;
    case 'S':
      info.type = EV_RIGIDBODY_STATIC;
      break;
    case 'K':
      info.type = EV_RIGIDBODY_KINEMATIC;
      break;
    default:
      ev_log_warn("Unidentified RigidbodyType `%.*s`. Falling back to `Kinematic`", rigidbodyType.len, (rigidbodyType.data + rigidbodyType.offset));
      info.type = EV_RIGIDBODY_KINEMATIC;
      break;
  }
  evstring_free(rigidbodytype_id);

  evstring mass_id = evstring_newfmt("%s.mass", *comp_id);
  info.mass = (F32)evjs_get(json, mass_id)->as_num;
  evstring_free(mass_id);

  evstring restitution_id = evstring_newfmt("%s.restitution", *comp_id);
  info.restitution = (F32)evjs_get(json, restitution_id)->as_num;
  evstring_free(restitution_id);


  evstring collisionshapetype_id = evstring_newfmt("%s.collisionShape.type", *comp_id);
  evstring collisionshapetype = evstring_refclone(evjs_get(json, collisionshapetype_id)->as_str);

  PhysicsWorldHandle physWorld = ev_scene_getphysicsworld(scene);
  CollisionShapeHandle collShapeHandle;
  if(!evstring_cmp(collisionshapetype, SphereCollisionShapeSTR)) {
    evstring radius_id = evstring_newfmt("%s.collisionShape.radius", *comp_id);
    F32 radius = (F32)evjs_get(json, radius_id)->as_num;
    collShapeHandle = CollisionShape->newSphere(physWorld, radius);
    evstring_free(radius_id);
  } else if(!evstring_cmp(collisionshapetype, BoxCollisionShapeSTR)) {
    Vec3 halfExtents;
    evstring halfextents_id = evstring_newfmt("%s.collisionShape.halfExtents[x]", *comp_id);
    size_t halfextents_id_len = evstring_len(halfextents_id);
    for(char i = 0; i < 3; i++) {
      halfextents_id[halfextents_id_len-2] = '0' + i;
      ((float*)&halfExtents)[i] = (float)evjs_get(json, halfextents_id)->as_num;
    }
    evstring_free(halfextents_id);

    collShapeHandle = CollisionShape->newBox(physWorld, halfExtents);

  } else if(!evstring_cmp(collisionshapetype, CapsuleCollisionShapeSTR)) {
    evstring radius_id = evstring_newfmt("%s.collisionShape.radius", *comp_id);
    evstring height_id = evstring_newfmt("%s.collisionShape.height", *comp_id);
    F32 radius = (F32)evjs_get(json, radius_id)->as_num;
    F32 height = (F32)evjs_get(json, height_id)->as_num;

    collShapeHandle = CollisionShape->newCapsule(physWorld, radius, height);

    evstring_free(radius_id);
    evstring_free(height_id);
  }

  info.collisionShape = collShapeHandle;

  Rigidbody->addToEntity(scene, obj, info);

  evstring_free(collisionshapetype);
  evstring_free(collisionshapetype_id);

  evstring_free(SphereCollisionShapeSTR);
  evstring_free(BoxCollisionShapeSTR);
  evstring_free(CapsuleCollisionShapeSTR);
}

GameObject
ev_scene_createobject(
    GameScene scene_handle);

void
ev_sceneloader_loadnode(
    GameScene scene,
    evjson_t *json,
    evstring *id,
    GameObject parent)
{
  evstring TransformComponentSTR = evstring_new("TransformComponent");
  evstring RigidbodyComponentSTR = evstring_new("RigidbodyComponent");
  evstring ScriptComponentSTR = evstring_new("ScriptComponent");
  evstring CameraComponentSTR = evstring_new("CameraComponent");
  ECSGameWorldHandle ecs_world = ev_scene_getecsworld(scene);

  GameObject obj;
  if(parent == 0) {
    obj = ev_scene_createobject(scene);
  } else {
    obj = ev_scene_createchildobject(scene, parent);
  }
  evstring nodename_id = evstring_newfmt("%s.id", *id);
  evstring nodename = evstring_refclone(evjs_get(json, nodename_id)->as_str);
  GameECS->setEntityName(ecs_world, obj, nodename);
  evstring_free(nodename);
  evstring_free(nodename_id);

  EV_DEFER(
      evstring components_count_id = evstring_newfmt("%s.components.len", *id),
      evstring_free(components_count_id))
  {
    int components_count = (int)evjs_get(json, components_count_id)->as_num;
    for(int i = 0; i < components_count; i++) {
      evstring component_id = evstring_newfmt("%s.components[%d]", *id, i);
      evstring component_type_id = evstring_newfmt("%s.type", component_id);
      evstring component_type = evstring_refclone(evjs_get(json, component_type_id)->as_str);

      if(!evstring_cmp(component_type, TransformComponentSTR)) {
        ev_sceneloader_loadtransformcomponent(scene, obj, json, &component_id);
      } else if(!evstring_cmp(component_type, ScriptComponentSTR)) {
        ev_sceneloader_loadscriptcomponent(scene, obj, json, &component_id);
      } else if(!evstring_cmp(component_type, RigidbodyComponentSTR)) {
        ev_sceneloader_loadrigidbodycomponent(scene, obj, json, &component_id);
      } else if(!evstring_cmp(component_type, CameraComponentSTR)) {
        ev_sceneloader_loadcameracomponent(scene, obj, json, &component_id);
      }


      evstring_free(component_type);
      evstring_free(component_type_id);
      evstring_free(component_id);
    }
  }

  EV_DEFER(
      evstring children_count_id = evstring_newfmt("%s.children.len", *id),
      evstring_free(children_count_id))
  {
    evjson_entry *children_count_res = evjs_get(json, children_count_id);
    if(children_count_res) {
      int children_count = (int)children_count_res->as_num;
      for(int i = 0; i < children_count; i++) {
        evstring child_id = evstring_newfmt("%s.children[%d]", *id, i);
        ev_sceneloader_loadnode(scene, json, &child_id, obj);
        evstring_free(child_id);
      }
    }
  }

  evstring_free(TransformComponentSTR);
  evstring_free(CameraComponentSTR);
  evstring_free(RigidbodyComponentSTR);
  evstring_free(ScriptComponentSTR);
}

GameScene
ev_scene_loadfromfile(
    CONST_STR path)
{
  GameScene newscene = ev_scene_create();
  AssetHandle scenefile_handle = Asset->load(path);
  JSONAsset scene_jsonasset = JSONLoader->loadAsset(scenefile_handle);
  evjson_t *scene_desc = (evjson_t*)scene_jsonasset.json_data;

  int nodes_count = (int)evjs_get(scene_desc, "nodes.len")->as_num;
  for(int i = 0; i < nodes_count; i++) {
    evstring node_id = evstring_newfmt("nodes[%d]", i);
    ev_sceneloader_loadnode(newscene, scene_desc, &node_id, 0);
    evstring_free(node_id);
  }

  evstring activeCamera = evstring_refclone(evjs_get(scene_desc, "activeCamera")->as_str);
  ev_scene_setactivecamera(newscene, ev_scene_getobject(newscene, activeCamera));
  evstring_free(activeCamera);

  Asset->free(scenefile_handle);
  return newscene;
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
ev_gamemod_scriptapi_loader(
    ScriptContextHandle ctx_h);

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

GameObject
ev_scene_getobject(
    GameScene scene_handle,
    CONST_STR name)
{
  GameSceneStruct scene = GameData.scenes[scene_handle?scene_handle:GameData.activeScene];
  return GameECS->getEntityFromName(scene.ecs_world, name);
}

GameObject
ev_object_getchild(
    GameScene scene_handle,
    GameObject parent,
    CONST_STR name)
{
  GameSceneStruct scene = GameData.scenes[scene_handle?scene_handle:GameData.activeScene];
  return GameECS->getChildFromName(scene.ecs_world, parent, name);
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

ScriptContextHandle
ev_scene_getscriptcontext(
    GameScene scene_handle)
{
  GameSceneStruct scene = GameData.scenes[scene_handle?scene_handle:GameData.activeScene];
  return scene.script_context;
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
ev_object_addcomponent(
    GameScene scene_handle,
    GameObject entt,
    GameComponentID comp_id)
{
  ECSGameWorldHandle ecs_world = ev_scene_getecsworld(scene_handle);
  GameECS->addComponent(ecs_world, entt, comp_id);
}

void
ev_object_addtag(
    GameScene scene_handle,
    GameObject entt,
    GameTagID tag_id)
{
  ECSGameWorldHandle ecs_world = ev_scene_getecsworld(scene_handle);
  GameECS->addTag(ecs_world, entt, tag_id);
}

void
ev_object_setcomponent(
    GameScene scene_handle,
    GameObject entt,
    GameComponentID comp_id,
    PTR data)
{
  ECSGameWorldHandle ecs_world = ev_scene_getecsworld(scene_handle);
  GameECS->setComponent(ecs_world, entt, comp_id, data);
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

  GameData.script_module = evol_loadmodule("script");
  if(GameData.script_module) {
    imports(GameData.script_module, (Script, ScriptContext, ScriptInterface));
    ScriptInterface->registerAPILoadFn(ev_gamemod_scriptapi_loader);
  }

  GameData.asset_module = evol_loadmodule("assetmanager");
  if(GameData.asset_module) {
    imports(GameData.asset_module, (Asset, TextLoader, JSONLoader));
  }

  GameData.physics_module = evol_loadmodule("physics");
  if(GameData.physics_module) {
    imports(GameData.physics_module, (PhysicsWorld, CollisionShape, Rigidbody));
  }

  GameData.scenes = vec_init(GameSceneStruct, NULL, gamescenestruct_destr);
  GameData.scene_map = Hashmap(evstring, GameScene).new();
  vec_setlen((vec_t*)&GameData.scenes, 1);
  GameData.scenes[0] = (GameSceneStruct){
    .physics_world = PhysicsWorld->invalidHandle(),
    .script_context = ScriptContext->invalidHandle()
  };
  GameData.activeScene = 0;
}

EV_DESTRUCTOR 
{
  vec_fini(GameData.scenes);
  Hashmap(evstring,GameScene).free(GameData.scene_map);

  if(GameData.physics_module) {
    evol_unloadmodule(GameData.physics_module);
  }

  if(GameData.asset_module) {
    evol_unloadmodule(GameData.asset_module);
  }

  if(GameData.ecs_module) {
    evol_unloadmodule(GameData.ecs_module);
  }

  if(GameData.script_module) {
    evol_unloadmodule(GameData.script_module);
  }
}

void
ev_scene_setname(
    GameScene scene_handle,
    CONST_STR name)
{
  Hashmap(evstring,GameScene).push(GameData.scene_map, evstring_new(name), scene_handle);
}

GameScene
ev_scene_getfromname(
    CONST_STR name)
{
  return *(GameScene*)Hashmap(evstring, GameScene).get(GameData.scene_map, name);
}

EV_BINDINGS
{
  EV_NS_BIND_FN(Game, setActiveScene, ev_game_setactivescene);
  EV_NS_BIND_FN(Game, progress, ev_game_progress);

  EV_NS_BIND_FN(Scene, create, ev_scene_create);
  EV_NS_BIND_FN(Scene, loadFromFile, ev_scene_loadfromfile);
  EV_NS_BIND_FN(Scene, setName, ev_scene_setname);
  EV_NS_BIND_FN(Scene, getFromName, ev_scene_getfromname);
  EV_NS_BIND_FN(Scene, createObject, ev_scene_createobject);
  EV_NS_BIND_FN(Scene, createChildObject, ev_scene_createchildobject);
  EV_NS_BIND_FN(Scene, addChildToObject, ev_scene_addchildtoobject);
  EV_NS_BIND_FN(Scene, destroyObject, ev_scene_destroyobject);
  EV_NS_BIND_FN(Scene, createCamera, ev_scene_createcamera);
  EV_NS_BIND_FN(Scene, getECSWorld, ev_scene_getecsworld);
  EV_NS_BIND_FN(Scene, getPhysicsWorld, ev_scene_getphysicsworld);
  EV_NS_BIND_FN(Scene, getScriptContext, ev_scene_getscriptcontext);
  EV_NS_BIND_FN(Scene, getActiveCamera, ev_scene_getactivecamera);
  EV_NS_BIND_FN(Scene, setActiveCamera, ev_scene_setactivecamera);
  EV_NS_BIND_FN(Scene, getObject, ev_scene_getobject);

  EV_NS_BIND_FN(Object, getWorldTransform, _ev_object_getworldtransform);
  EV_NS_BIND_FN(Object, setWorldTransform, _ev_object_setworldtransform);
  EV_NS_BIND_FN(Object, setPosition, _ev_object_setposition);
  EV_NS_BIND_FN(Object, setRotation, _ev_object_setrotation);
  EV_NS_BIND_FN(Object, setScale,    _ev_object_setscale);
  EV_NS_BIND_FN(Object, getPosition, _ev_object_getposition);
  EV_NS_BIND_FN(Object, getRotation, _ev_object_getrotation);
  EV_NS_BIND_FN(Object, getScale,    _ev_object_getscale);
  EV_NS_BIND_FN(Object, getChild,     ev_object_getchild);

  // ECS shortcuts
  EV_NS_BIND_FN(Object, getComponent, ev_object_getcomponent);
  EV_NS_BIND_FN(Object, hasComponent, ev_object_hascomponent);
  EV_NS_BIND_FN(Object, setComponent, ev_object_setcomponent);
  EV_NS_BIND_FN(Object, addComponent, ev_object_addcomponent);
  EV_NS_BIND_FN(Object, addTag      , ev_object_addtag);

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

void
ev_game_setactivescenename_wrapper(
    CONST_STR *name)
{
  ev_game_setactivescene(ev_scene_getfromname(*name));
}

void
ev_scene_getobject_wrapper(
    GameObject *out,
    CONST_STR *path)
{
  *out = ev_scene_getobject(NULL, *path);
}

void 
ev_gamemod_scriptapi_loader(
    ScriptContextHandle ctx_h)
{
  ScriptType voidSType = ScriptInterface->getType(ctx_h, "void");
  ScriptType floatSType = ScriptInterface->getType(ctx_h, "float");
  ScriptType ullSType = ScriptInterface->getType(ctx_h, "unsigned long long");
  ScriptType constCharType = ScriptInterface->getType(ctx_h, "const char*");

  ScriptType vec3SType = ScriptInterface->addStruct(ctx_h, "Vec3", sizeof(Vec3), 3, (ScriptStructMember[]) {
      {"x", floatSType, offsetof(Vec3, x)},
      {"y", floatSType, offsetof(Vec3, y)},
      {"z", floatSType, offsetof(Vec3, z)}
  });

  ScriptInterface->addFunction(ctx_h, _ev_object_getposition_wrapper, "ev_object_getposition", vec3SType, 1, (ScriptType[]){ullSType});
  ScriptInterface->addFunction(ctx_h, _ev_object_setposition_wrapper, "ev_object_setposition", voidSType, 2, (ScriptType[]){ullSType, vec3SType});

  ScriptInterface->addFunction(ctx_h, _ev_object_getrotationeuler_wrapper, "ev_object_getrotationeuler", vec3SType, 1, (ScriptType[]){ullSType});
  ScriptInterface->addFunction(ctx_h, _ev_object_setrotationeuler_wrapper, "ev_object_setrotationeuler", voidSType, 2, (ScriptType[]){ullSType, vec3SType});

  /* ScriptInterface->addFunction(_ev_object_getscale_wrapper, "ev_object_getscale", vec3SType, 1, (ScriptType[]){ullSType}); */
  /* ScriptInterface->addFunction(_ev_object_setscale_wrapper, "ev_object_setscale", voidSType, 2, (ScriptType[]){ullSType, vec3SType}); */

  ScriptInterface->addFunction(ctx_h, ev_game_setactivescenename_wrapper, "ev_game_setactivescenename", voidSType, 1, (ScriptType[]){constCharType});

  ScriptInterface->addFunction(ctx_h, ev_scene_getobject_wrapper, "ev_scene_getobject", ullSType, 1, (ScriptType[]){constCharType});

  ScriptInterface->loadAPI(ctx_h, "subprojects/evmod_game/script_api.lua");
}
