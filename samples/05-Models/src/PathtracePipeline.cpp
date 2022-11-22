#include <pathtrace.h>
#include <GameFramework/Window.h>
#include <dx12lib/CommandList.h>
#include <dx12lib/Device.h>
#include <dx12lib/Helpers.h>
#include <d3d12.h>
#include <dx12lib/Material.h>
#include <dx12lib/PipelineStateObject.h>
#include <dx12lib/RootSignature.h>
#include <dx12lib/VertexTypes.h>
#include <DirectXMath.h>
#include <Windows.h>
#include <memory>
#include <d3dcompiler.h>

using namespace Microsoft::WRL;

#define ArraySize_(x) ((sizeof(x) / sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))


PathTracePipeline::PathTracePipeline(std::shared_ptr<dx12lib::Device> device)
	:m_Device(device), m_pPreviousCommandList(nullptr)
{
	// Create an SRV that can be used to pad unused texture slots.
	D3D12_SHADER_RESOURCE_VIEW_DESC defaultSRV;
	defaultSRV.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	defaultSRV.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	defaultSRV.Texture2D.MostDetailedMip = 0;
	defaultSRV.Texture2D.MipLevels = 1;
	defaultSRV.Texture2D.PlaneSlice = 0;
	defaultSRV.Texture2D.ResourceMinLODClamp = 0;
	defaultSRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	m_DefaultSRV = m_Device->CreateShaderResourceView(nullptr, &defaultSRV);
}


void PathTracePipeline::pathtraceInit(int width, int height, dx12lib::Scene* scene)
{
	// Load the vertex shader.
	ComPtr<ID3DBlob> vertexShaderBlob;
	ComPtr<ID3DBlob> pixelShaderBlob;

#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	//Read shader
	ThrowIfFailed(D3DCompileFromFile(std::wstring(L"pathtrace.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShaderBlob, nullptr));
	ThrowIfFailed(D3DCompileFromFile(std::wstring(L"pathtrace.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShaderBlob, nullptr));


	//======== Raytrace root signature =============

	D3D12_DESCRIPTOR_RANGE1 uavRanges[1] = {};
	uavRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	uavRanges[0].NumDescriptors = 1;
	uavRanges[0].BaseShaderRegister = 0;
	uavRanges[0].RegisterSpace = 0;
	uavRanges[0].OffsetInDescriptorsFromTableStart = 0;

	const UINT32 NumUserDescriptorRanges = 16;
	const UINT32 NumGlobalSRVDescriptorRanges = 7 + NumUserDescriptorRanges;
	D3D12_DESCRIPTOR_RANGE1 GlobalSRVDescriptorRangeDescs[NumGlobalSRVDescriptorRanges] = { };

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;


	D3D12_ROOT_PARAMETER1 rootParameters[NumRTRootParams] = {};

	// Standard SRV descriptors
	rootParameters[PTParams_StandardDescriptors].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[PTParams_StandardDescriptors].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[PTParams_StandardDescriptors].DescriptorTable.pDescriptorRanges = GlobalSRVDescriptorRangeDescs;
	rootParameters[PTParams_StandardDescriptors].DescriptorTable.NumDescriptorRanges = NumGlobalSRVDescriptorRanges;

	// Acceleration structure SRV descriptor
	rootParameters[PTParams_SceneDescriptor].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParameters[PTParams_SceneDescriptor].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[PTParams_SceneDescriptor].Descriptor.ShaderRegister = 0;
	rootParameters[PTParams_SceneDescriptor].Descriptor.RegisterSpace = 200;

	// UAV descriptor
	rootParameters[PTParams_UAVDescriptor].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[PTParams_UAVDescriptor].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[PTParams_UAVDescriptor].DescriptorTable.pDescriptorRanges = uavRanges;
	rootParameters[PTParams_UAVDescriptor].DescriptorTable.NumDescriptorRanges = ArraySize_(uavRanges);

	// CBuffer
	rootParameters[PTParams_CBuffer].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[PTParams_CBuffer].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[PTParams_CBuffer].Descriptor.RegisterSpace = 0;
	rootParameters[PTParams_CBuffer].Descriptor.ShaderRegister = 0;
	rootParameters[PTParams_CBuffer].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;

	// LightCBuffer
	rootParameters[PTParams_LightCBuffer].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[PTParams_LightCBuffer].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[PTParams_LightCBuffer].Descriptor.RegisterSpace = 0;
	rootParameters[PTParams_LightCBuffer].Descriptor.ShaderRegister = 1;
	rootParameters[PTParams_LightCBuffer].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;

	// AppSettings
	rootParameters[PTParams_AppSettings].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[PTParams_AppSettings].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[PTParams_AppSettings].Descriptor.RegisterSpace = 0;
	//CBuffer Register is 12
	rootParameters[PTParams_AppSettings].Descriptor.ShaderRegister = 12;
	rootParameters[PTParams_AppSettings].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;

	D3D12_STATIC_SAMPLER_DESC staticSamplers[2] = {};
	//May have problem here(Maybe should use D3D12)
	CD3DX12_STATIC_SAMPLER_DESC anisotropicSampler(0, D3D12_FILTER_ANISOTROPIC);
	CD3DX12_STATIC_SAMPLER_DESC linearClamp(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

	staticSamplers[0] = anisotropicSampler;
	staticSamplers[1] = linearClamp;


	D3D12_ROOT_SIGNATURE_DESC1 rootSignatureDesc = {};
	rootSignatureDesc.NumParameters = ArraySize_(rootParameters);
	rootSignatureDesc.pParameters = rootParameters;
	rootSignatureDesc.NumStaticSamplers = ArraySize_(staticSamplers);
	rootSignatureDesc.pStaticSamplers = staticSamplers;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	//rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;
	m_RootSignature = m_Device->CreateRootSignature(rootSignatureDesc);

	//========== Raytrace pipeline state ====================
	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE        pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_VS                    VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS                    PS;
		CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER            RasterizerState;
		//CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT          InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY    PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT  DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
		CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC           SampleDesc;
	} pipelineStateStream;

	// Create a color buffer with sRGB for gamma correction.
	DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

	// Check the best multisample quality level that can be used for the given back buffer format.
	DXGI_SAMPLE_DESC sampleDesc = m_Device->GetMultisampleQualityLevels(backBufferFormat);

	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 1;
	rtvFormats.RTFormats[0] = backBufferFormat;

	CD3DX12_RASTERIZER_DESC rasterizerState(D3D12_DEFAULT);

	//if (m_EnableDecal) {
	//	// Disable backface culling on decal geometry.
	//	rasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	//}
	pipelineStateStream.pRootSignature = m_RootSignature->GetD3D12RootSignature().Get();
	pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
	pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
	pipelineStateStream.RasterizerState = rasterizerState;
	//	pipelineStateStream.InputLayout = VertexPositionNormalTangentBitangentTexture::InputLayout;
	pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.DSVFormat = depthBufferFormat;
	pipelineStateStream.RTVFormats = rtvFormats;
	pipelineStateStream.SampleDesc = sampleDesc;

	m_PipelineStateObject = m_Device->CreatePipelineStateObject(pipelineStateStream);

	//Remember to set ray trace camera as scene camera
}