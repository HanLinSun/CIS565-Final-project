#pragma once
#include <camera.h>
#include <vector>
#include <string>
#include <DirectXMath.h>

#include "dx12lib/Scene.h"

namespace dx12lib
{
    class CommandList;
    class Device;
    class Material;
    class RootSignature;
    class PipelineStateObject;
    class ShaderResourceView;
    class Texture;
}  // namespace dx12lib

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

class PathTracePipeline
{
public:    
struct LightProperties
{
    uint32_t NumAreaLights;
    uint32_t NumPointLights;
};

struct alignas(16) Matrices
{
    DirectX::XMMATRIX ModelMatrix;
    DirectX::XMMATRIX ModelViewMatrix;
    DirectX::XMMATRIX InverseTransposeModelViewMatrix;
    DirectX::XMMATRIX ModelViewProjectionMatrix;
};
enum RootParameters
{
    //constant buffer
    PathSegmentCB,
    Textures
};

struct alignas(16) MVP
{
    DirectX::XMMATRIX World;
    DirectX::XMMATRIX View;
    DirectX::XMMATRIX Projection;
};


//Pipeline
PathTracePipeline(std::shared_ptr<dx12lib::Device> device);

private:
     std::shared_ptr<dx12lib::Device>              m_Device;
     std::shared_ptr<dx12lib::RootSignature>       m_RootSignature;
     std::shared_ptr<dx12lib::PipelineStateObject> m_PipelineStateObject;

     dx12lib::CommandList* m_pPreviousCommandList;

    void pathtraceInit(int width, int height, dx12lib::Scene* scene);
};


