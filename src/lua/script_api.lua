
EntityMemberGetters['position'] = function(entt)
  return C('ev_object_getposition', entt)
end

EntityMemberSetters['position'] = function(entt, pos)
  C('ev_object_setposition', entt, pos)
  local rb = ComponentGetters[Rigidbody](entt)
  if rb ~= nil then
    rb:setPosition(pos)
  end
end


EntityMemberGetters['eulerAngles'] = function(entt)
  error('Attempt to read Entity.eulerAngles (Write-Only Parameter)')
  -- return C('ev_object_getrotationeuler', entt)
end

EntityMemberSetters['eulerAngles'] = function(entt, rot)
  C('ev_object_setrotationeuler', entt, rot)
  local rb = ComponentGetters[Rigidbody](entt)
  if rb.handle ~= nil then
    rb:setRotation(rot)
  end
end

function gotoScene(name)
  C('ev_game_setactivescenename', name)
end

function getObject(path)
  return Entities[C('ev_scene_getobject', path)]
end
