#pragma once
#pragma once
#include <DirectXMath.h>
#include <string>
#include <camera.h>
#include <vector>

#define BACKGROUND_COLOR         ( glm::vec3( 0.0f ) )
#define USE_BVH_FOR_INTERSECTION 1

//CUDA PT framework
enum GeomType
{
    SPHERE,
    CUBE,
    TRIANGLE,
    MESH,
};

struct Ray
{
    DirectX::XMVECTOR origin;
    DirectX::XMVECTOR direction;
};

struct Geom
{
    enum GeomType type;
    int           materialid;
    DirectX::XMVECTOR translation;
    DirectX::XMVECTOR rotation;
    DirectX::XMVECTOR scale;
    DirectX::XMMATRIX transform;
    DirectX::XMMATRIX inverseTransform;
    DirectX::XMMATRIX invTranspose;
    int           faceStartIdx;  // use with array of Triangle
    int           faceNum;
};

struct Triangle
{
    DirectX::XMVECTOR point_0;
    DirectX::XMVECTOR point_1;
    DirectX::XMVECTOR point_2;
    
    DirectX::XMVECTOR normal_0;
    DirectX::XMVECTOR normal_1;
    DirectX::XMVECTOR normal_2;
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

struct RenderState
{
    Camera                 camera;
    unsigned int           iterations;
    int                    traceDepth;
    std::vector<DirectX::XMVECTOR> image;
    std::string            imageName;
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
