#pragma once
#include <camera.h>
#include <CameraController.h>
#include <vector>
#include <string>
#include <dx12lib/CommandList.h>
#include <Graphics/Model.h>
#include <Graphics/GraphicsTypes.h>
#include <DirectXMath.h>
#include <future>
#include <GameFramework/GameFramework.h>
#include <GameFramework/Window.h>
#include <dx12lib/RenderTarget.h>
#include <dx12lib/SwapChain.h>
#include <dx12lib/Device.h>
#include "dx12lib/Scene.h"
#include <Graphics/GraphicsTypes.h>
#include <Graphics/ShaderCompilation.h>
#include <memory>

using namespace SampleFramework12;

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

// Shader pass
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

struct RayTraceConstants
{
    Float4x4 InvViewProjection;
    Float3 SunDirectionWS;
    float CosSunAngularRadius = 0.0f;
    Float3 SunIrradiance;
    float SinSunAngularRadius = 0.0f;
    Float3 SunRenderColor;
    uint32 Padding = 0;
    Float3 CameraPosWS;
    uint32 CurrSampleIdx = 0;
    uint32 TotalNumPixels = 0;

    uint32 VtxBufferIdx = uint32(-1);
    uint32 IdxBufferIdx = uint32(-1);
    uint32 GeometryInfoBufferIdx = uint32(-1);
    uint32 MaterialBufferIdx = uint32(-1);
    uint32 SkyTextureIdx = uint32(-1);
    uint32 NumLights = 0;
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

void OpenFile();
bool InitScene(const std::wstring& sceneFile);
bool LoadingProgress(float loadingProgress);
void LoadContent();
void PathtraceInit();
void CreatePathTracePipelineStateObject();
//Windows callback function
void Render();
void CreateRenderTargets();
void OnResize(ResizeEventArgs& e);
void OnUpdate(UpdateEventArgs& e);


void OnKeyPressed(KeyEventArgs& e);
uint32_t Run();
//Pipeline
PathTracePipeline(std::shared_ptr<dx12lib::Device> device, int width, int height);
void Apply(dx12lib::CommandList& commandList) {};

private:

    std::shared_ptr<Window> m_Window;

    Camera           m_Camera;
    CameraController m_CameraController;
    Logger           m_Logger;

    ID3D12StateObject* rtPSO = nullptr;
    SampleFramework12::StructuredBuffer rtRayGenTable;
    SampleFramework12::StructuredBuffer rtHitTable;
    SampleFramework12::StructuredBuffer rtMissTable;
    SampleFramework12::StructuredBuffer rtGeoInfoBuffer;

    int  m_Width;
    int  m_Height;
    bool m_VSync;

    dx12lib::RenderTarget m_RenderTarget;

    std::atomic_bool  m_IsLoading;
    std::future<bool> m_LoadingTask;
    float             m_LoadingProgress;
    bool              m_CancelLoading;
    std::string       m_LoadingText;

    Model sceneModels;
    //MeshRenderer meshRenderer;
    DepthBuffer depthBuffer;

    Array<SpotLight> spotLights;
    SampleFramework12:: ConstantBuffer spotLightBuffer;
    SampleFramework12::StructuredBuffer spotLightBoundsBuffer;
    SampleFramework12::StructuredBuffer spotLightInstanceBuffer;
    RawBuffer spotLightClusterBuffer;
    uint64 numIntersectingSpotLights = 0;

    RenderTexture rtTarget;

    const Model* currentModel = nullptr;

    std::shared_ptr<dx12lib::Device>              m_Device;
    std::shared_ptr<dx12lib::GUI>       m_GUI;
    std::shared_ptr<dx12lib::SwapChain> m_SwapChain;
    // Global root signature with all of our normal bindings
    std::shared_ptr<dx12lib::RootSignature>       m_RootSignature;

    // Ray tracing resources
    CompiledShaderPtr pathTraceShader;
    uint32 rtCurrSampleIdx = 0;
    RawBuffer rtBottomLevelAccelStructure;
    RawBuffer rtTopLevelAccelStructure;

     //Pathtrace Pipeline State Object
     std::shared_ptr<dx12lib::PipelineStateObject> m_PipelineStateObject;
     D3D12_VIEWPORT m_Viewport;
     
     // An SRV used pad unused texture slots.
     std::shared_ptr<dx12lib::Scene> m_Scene;
     std::shared_ptr<dx12lib::ShaderResourceView> m_DefaultSRV;

     dx12lib::CommandList* m_pPreviousCommandList;
};


