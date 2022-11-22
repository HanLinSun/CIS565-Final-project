#pragma once
#include <camera.h>
#include <vector>
#include <string>
#include <dx12lib/CommandList.h>
#include <DirectXMath.h>
#include <future>
#include <GameFramework/GameFramework.h>
#include <dx12lib/RenderTarget.h>


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

enum PTRootParams : UINT32
{
    PTParams_StandardDescriptors,
    PTParams_SceneDescriptor,
    PTParams_UAVDescriptor,
    PTParams_CBuffer,
    PTParams_LightCBuffer,
    PTParams_AppSettings,

    NumRTRootParams
};

struct PathTraceConstants
{
   DirectX::XMFLOAT4X4 InverseViewProjection;

   //DirectX::XMFLOAT3 SunDirectionWS;
   // float CosSunAngularRadius = 0.0f;
   // DirectX::XMFLOAT3 SunIrradiance;
   // float SinSunAngularRadius = 0.0f;
   // DirectX::XMFLOAT3 SunRenderColor;

   UINT32 Padding = 0;

    DirectX::XMFLOAT3 CameraPosWorldSpace;

    UINT32 CurrSampleIdx = 0;
    UINT32 TotalNumPixels = 0;

    UINT32 VtxBufferIdx = UINT32(-1);
    UINT32 IdxBufferIdx = UINT32(-1);
    UINT32 GeometryInfoBufferIdx = UINT32(-1);
    UINT32 MaterialBufferIdx = UINT32(-1);
    UINT32 SkyTextureIdx = UINT32(-1);
    UINT32 NumLights = 0;
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
    MatricesCB,
    Textures,
    NumRootParameters
};

struct alignas(16) MVP
{
    DirectX::XMMATRIX World;
    DirectX::XMMATRIX View;
    DirectX::XMMATRIX Projection;
};

void pathtraceInit(int width, int height, dx12lib::Scene* scene);
void createPipelineStateObject();
uint32_t Run();
//Pipeline
PathTracePipeline(std::shared_ptr<dx12lib::Device> device);
void Apply(dx12lib::CommandList& commandList) {};

private:


    dx12lib::RenderTarget m_RenderTarget;
    std::atomic_bool  m_IsLoading;
    std::future<bool> m_LoadingTask;
    float             m_LoadingProgress;
    std::string       m_LoadingText;

    std::shared_ptr<Window> m_Window;

    // Ray tracing resources
     std::shared_ptr<dx12lib::Device>              m_Device;
     std::shared_ptr<dx12lib::RootSignature>       m_RootSignature;
     std::shared_ptr<dx12lib::PipelineStateObject> m_PipelineStateObject;
     // An SRV used pad unused texture slots.
     std::shared_ptr<dx12lib::ShaderResourceView> m_DefaultSRV;

     dx12lib::CommandList* m_pPreviousCommandList;


    //Pathtrace Pipeline State Object

};


