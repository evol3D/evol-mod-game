
EntityMemberGetters['position'] = function(entt)
  return C('ev_object_getposition', entt)
end

EntityMemberSetters['position'] = function(entt, pos)
  C('ev_object_setposition', entt, pos)
  local rb = ComponentGetters[Rigidbody](entt)
  if rb.handle ~= nil then
    rb:setPosition(pos)
  end
end
