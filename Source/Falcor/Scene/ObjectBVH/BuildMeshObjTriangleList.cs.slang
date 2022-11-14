#pragma once
#include "Utils/HostDeviceShared.slangh"

#ifdef HOST_CODE
#include "Utils/Math/PackedFormats.h"
#else
import Utils.Math.PackedFormats;
#endif

struct EmissiveTriangle
{
    float3  posW[3];            ///< Per-vertex world-space positions.
    float2  texCoords[3];       ///< Per-vertex texture coordinates.
    float3  normal;             ///< Face normal in world space.
    float   area;               ///< Triangle area in world space.
    uint    materialID;         ///< Material ID.
    uint    lightIdx;           ///< Index into global mesh lights array.

    /** Returns the interpolated position.
        \param[in] barycentrics Barycentric coordinates.
        \return Interpolated position in world space.
    */
    float3 getPosition(float3 barycentrics) CONST_FUNCTION
    {
        return posW[0] * barycentrics[0] +
            posW[1] * barycentrics[1] +
            posW[2] * barycentrics[2];
    }

    /** Returns the interpolated texture coordinate.
        \param[in] barycentrics Barycentric coordinates.
        \return Interpolated texture coordinate.
    */
    float2 getTexCoord(float3 barycentrics) CONST_FUNCTION
    {
        return texCoords[0] * barycentrics[0] +
            texCoords[1] * barycentrics[1] +
            texCoords[2] * barycentrics[2];
    }
};

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
}
