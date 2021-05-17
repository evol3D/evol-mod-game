#pragma once

#include <evol/common/ev_types.h>


// =====================
// Component Definitions
// =====================

/* ===============Transform Component=============== */
typedef struct {
  Vec3 position;
  Vec4 rotation;
  Vec3 scale;
} TransformComponent;
static CONST_STR TransformComponentName = "TransformComponent";
static U64 TransformComponentID;



/* ============World Transform Component============ */
typedef Matrix4x4 
WorldTransformComponent;
static CONST_STR WorldTransformComponentName = "WorldTransformComponent";
static U64 WorldTransformComponentID;


// ===============
// Tag Definitions
// ===============

/* ===============Dirty Transform Tag=============== */
static CONST_STR DirtyTransformTagName = "DirtyTransform";
static U64 DirtyTransformTagID;
