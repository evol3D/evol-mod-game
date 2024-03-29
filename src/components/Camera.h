#pragma once

#include <evol/common/ev_types.h>

// =====================
// Component Definitions
// =====================

/* =================Camera Component================ */
typedef struct {
  CameraViewType viewType;
  U32 hfov;
  U32 vfov;

  F32 aspectRatio;

  F32 nearPlane;
  F32 farPlane;

  Matrix4x4 projectionMatrix;
} CameraComponent;
static CONST_STR CameraComponentName = "CameraComponent";
static U64 CameraComponentID;


// ===============
// Tag Definitions
// ===============
