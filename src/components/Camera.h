#pragma once

#include <evol/common/ev_types.h>

typedef enum {
  EV_CAMERA_PERSPECTIVE_VIEW,
  EV_CAMERA_ORTHOGRAPHIC_VIEW,
} CameraViewType;

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

/* ==============Main Camera Component============== */
typedef struct {
  U64 entt;
} MainCamera;
static CONST_STR MainCameraName = "MainCamera";
static U64 MainCameraID;


// ===============
// Tag Definitions
// ===============
