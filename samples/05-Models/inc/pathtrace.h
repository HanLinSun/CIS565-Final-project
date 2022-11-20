#pragma once
#include <camera.h>
#include <vector>
#include <string>
#include <DirectXMath.h>
#include "dx12lib/Scene.h"

struct RenderState
{
    Camera                 camera;
    unsigned int           iterations;
    int                    traceDepth;
    std::vector<DirectX::XMVECTOR> image;
    std::string            imageName;
};

struct Ray
{
    DirectX::XMVECTOR origin;
    DirectX::XMVECTOR direction;
};

struct PBRMaterial
{
    DirectX::XMVECTOR color;
    struct
    {
        float     exponent;
        DirectX::XMVECTOR color;
    } specular;
    float hasReflective;
    float hasRefractive;
    float indexOfRefraction;
    float emittance;
};

struct PathSegment
{
    Ray       ray;
    DirectX::XMVECTOR color;
    int       pixelIndex;
    int       remainingBounces;
};

// Use with a corresponding PathSegment to do:
// 1) color contribution computation
// 2) BSDF evaluation: generate a new ray
struct ShadeableIntersection
{
    float     t;
    DirectX::XMVECTOR surfaceNormal;
    int       materialId;
};


void pathtraceInit(int width, int height, dx12lib::Scene* scene);
