#pragma once
#include <DirectXMath.h>
enum GeomType {
    SPHERE,
    CUBE,
    TRIANGLE,
    MESH,
};

struct Ray {
    DirectX::XMFLOAT3 origin;
    DirectX::XMFLOAT3 direction;
};


struct Triangle {
    DirectX::XMFLOAT3 point_0;
    DirectX::XMFLOAT3 point_1;
    DirectX::XMFLOAT3 point_2;
    DirectX::XMFLOAT3 normal_0;
    DirectX::XMFLOAT3 normal_1;
    DirectX::XMFLOAT3 normal_2;
   //For BVH
    int geomId;
    DirectX::XMFLOAT3  minCorner;
    DirectX::XMFLOAT3  maxCorner;
    DirectX::XMFLOAT3  centroid;
  
};

struct PBRMaterial {
    DirectX::XMVECTOR color;
    struct {
        float exponent;
        DirectX::XMVECTOR color;
    } specular;
    float hasReflective;
    float hasRefractive;
    float indexOfRefraction;
    float emittance;
};
