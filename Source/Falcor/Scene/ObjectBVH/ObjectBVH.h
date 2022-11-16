#pragma once
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include "Utils/Math/Vector.h"
#include "Utils/Math/Matrix.h"
//============= Hanlin Added Here ==================
namespace Falcor
{
    struct BVHNode
    {
        BVHNode* left = NULL; //left tree pointer
        BVHNode* right = NULL; //right tree pointer
        int n;
        int index;
        float3 AA; //Bounding Box minpoint
        float3 BB; //Bounding Box maxpoint;
    };

    struct MeshObjectVertex
    {
        float3 pos;     ///< World-space position.
        float2 uv;      ///< Texture coordinates in emissive texture (if textured).
    };

    struct MeshObjectTriangle
    {
        // TODO: Perf of indexed vs non-indexed on GPU. We avoid level of indirection, but have more bandwidth non-indexed.
        MeshObjectVertex vtx[3];                             ///< Vertices. These are non-indexed for now.

        // Pre-computed quantities.
        float3 normal = float3(0);                 ///< Triangle's face normal in world space.
        float   area = 0.f;                         ///< Triangle area in world space units.
        /** Returns the center of the triangle in world space.
        */
        float3 getCenter() const
        {
            return (vtx[0].pos + vtx[1].pos + vtx[2].pos) / 3.0f;
        }
    };
}
//============ End ==================
