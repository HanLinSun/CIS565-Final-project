#pragma once
#include "Utils/HostDeviceShared.slangh"

BEGIN_NAMESPACE_FALCOR

/** Describes a mesh area light.

    Note that due to mesh merging etc. it's not guaranteed that a mesh light
    represents a single localized light source in the scene.
    We should not make any assumptions of the extents or geometry of a mesh light.
*/
struct MeshInstanceData
{
    uint        instanceID = kInvalidIndex;    ///< Geometry instance ID in the scene.
    uint        triangleOffset = kInvalidIndex;    ///< Offset into Scene's global list of triangles.
    uint        triangleCount = 0;                ///< Number of triangles in mesh lnstance
    uint        materialID = kInvalidIndex;    ///< Material ID (index into scene.materials array).

    static const uint kInvalidIndex = 0xffffffff;
};

END_NAMESPACE_FALCOR
