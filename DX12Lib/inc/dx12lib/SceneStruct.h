#pragma once
//========== Added by Hanlin ===============
#pragma once
#pragma once
#include <DirectXMath.h>
#include <string>

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
    DirectX::XMFLOAT3 point_0;
    DirectX::XMFLOAT3 point_1;
    DirectX::XMFLOAT3 point_2;

    DirectX::XMFLOAT3 normal_0;
    DirectX::XMFLOAT3 normal_1;
    DirectX::XMFLOAT3 normal_2;

    DirectX::XMFLOAT3 centroid;

};

