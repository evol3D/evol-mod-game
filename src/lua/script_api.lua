
EntityMemberGetters['name'] = function(entt)
  res = C('ev_object_getname', entt)
  Entities[entt].name = res
  return res
end

Entity.getChild = function(self,name)
  return Entities[C('ev_object_getchild', self.entityID, name)]
end

EntityMemberGetters['position'] = function(entt)
  res = C('ev_object_getposition', entt)
  return Vec3:new(res.x, res.y, res.z)
end

EntityMemberSetters['position'] = function(entt, pos)
  C('ev_object_setposition', entt, pos)
  local rb = ComponentGetters[Rigidbody](entt)
  if rb ~= nil then
    rb:setPosition(pos)
  end
end

EntityMemberGetters['worldPosition'] = function(entt)
  res = C('ev_object_getworldposition', entt)
  return Vec3:new(res.x, res.y, res.z)
end

EntityMemberGetters['eulerAngles'] = function(entt)
  error('Attempt to read Entity.eulerAngles (Write-Only Parameter)')
  -- return C('ev_object_getrotationeuler', entt)
end

EntityMemberSetters['eulerAngles'] = function(entt, rot)
  C('ev_object_setrotationeuler', entt, rot)
  local rb = ComponentGetters[Rigidbody](entt)
  if rb ~= nil and rb.handle ~= nil then
    rb:setRotation(rot)
  end
end

EntityMemberGetters['forward'] = function(entt)
  res = C('ev_object_getforwardvec', entt)
  return Vec3:new(res.x, res.y, res.z)
end

EntityMemberGetters['right'] = function(entt)
  res = C('ev_object_getrightvec', entt)
  return Vec3:new(res.x, res.y, res.z)
end

EntityMemberGetters['up'] = function(entt)
  res = C('ev_object_getupvec', entt)
  return Vec3:new(res.x, res.y, res.z)
end

function gotoScene(name)
  C('ev_game_setactivescenename', name)
end

function getObject(path)
  return Entities[C('ev_scene_getobject', path)]
end

function loadPrefab(path)
  -- Storing this as this operation will change what this points to
  old_this = this
  prefab = Entities[C('ev_sceneloader_loadprefab', path)]
  this = old_this
  return prefab
end
