#include <pathtrace.h>
#include <GameFramework/Window.h>
#include <dx12lib/CommandList.h>
#include <dx12lib/Helpers.h>
#include <AppSettings.h>
#include <d3d12.h>
#include <dx12lib/Material.h>
#include <Assert.h>
#include <Graphics/DX12_Helpers.h>
#include <Graphics/DXRHelper.h>
#include <dx12lib/PipelineStateObject.h>
#include <dx12lib/RootSignature.h>
#include <dx12lib/VertexTypes.h>
#include <dx12lib/GUI.h>
#include <DirectXMath.h>
#include <Windows.h>
#include <memory>
#include <d3dcompiler.h>
#include <ShObjIdl.h>  // For IFileOpenDialog
#include <shlwapi.h>

#include "../../../DX12Lib/src/GUI.cpp"

using namespace Microsoft::WRL;

#define ArraySize_(x) ((sizeof(x) / sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))



void PathTracePipeline::OnKeyPressed(KeyEventArgs& e)
{
	if (!ImGui::GetIO().WantCaptureKeyboard)
	{
		switch (e.Key)
		{
		case KeyCode::Escape:
			GameFramework::Get().Stop();
			break;
		case KeyCode::V:
			m_SwapChain->ToggleVSync();
			break;
		case KeyCode::R:
			// Reset camera transform
			m_CameraController.ResetView();
			break;
		case KeyCode::O:
			if (e.Control)
			{
				OpenFile();
			}
			break;
		}
	}
}

void PathTracePipeline::OnResize(ResizeEventArgs& e)
{
	m_Logger->info("Resize: {}, {}", e.Width, e.Height);

	m_Width = std::max(1, e.Width);
	m_Height = std::max(1, e.Height);

	m_Camera.set_Projection(45.0f, m_Width / (float)m_Height, 0.1f, 100.0f);
	m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height));

	m_RenderTarget.Resize(m_Width, m_Height);

	m_SwapChain->Resize(m_Width, m_Height);
}

PathTracePipeline::PathTracePipeline(std::shared_ptr<dx12lib::Device> device, int width, int height)
	:m_Device(device), m_pPreviousCommandList(nullptr), m_Width(width)
	, m_Height(height), m_CameraController(m_Camera), m_CancelLoading(false)
{
	//Initiate Window
	m_Window->Update += UpdateEvent::slot(&PathTracePipeline::OnUpdate, this);
	m_Window->KeyPressed += KeyboardEvent::slot(&PathTracePipeline::OnKeyPressed, this);
}

struct HitGroupRecord
{
	ShaderIdentifier ID;
};

void PathTracePipeline::LoadContent()
{
	
	m_Device = dx12lib::Device::Create();
	m_Logger->info(L"Device created: {}", m_Device->GetDescription());

	m_SwapChain = m_Device->CreateSwapChain(m_Window->GetWindowHandle(), DXGI_FORMAT_R8G8B8A8_UNORM);
	m_GUI = m_Device->CreateGUI(m_Window->GetWindowHandle(), m_SwapChain->GetRenderTarget());

	// This magic here allows ImGui to process window messages.

	GameFramework::Get().WndProcHandler += WndProcEvent::slot(&GUI::WndProcHandler, m_GUI);

	// Start the loading task to perform async loading of the scene file.
	m_LoadingTask = std::async(std::launch::async, std::bind(&PathTracePipeline::InitScene, this,
		L"Assets/Models/crytek-sponza/sponza_nobanner.obj"));



	// Load a few (procedural) models to represent the light sources in the scene.
	auto& commandQueue = m_Device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto  commandList = commandQueue.GetCommandList();

	//m_Sphere = commandList->CreateSphere(0.1f);
	//m_Cone = commandList->CreateCone(0.1f, 0.2f);
	//m_Axis = commandList->LoadSceneFromFile(L"Assets/Models/axis_of_evil.nff");

	auto fence = commandQueue.ExecuteCommandList(commandList);

	// Create a color buffer with sRGB for gamma correction.
	DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

	// Check the best multisample quality level that can be used for the given back buffer format.
	DXGI_SAMPLE_DESC sampleDesc = m_Device->GetMultisampleQualityLevels(backBufferFormat);

	// Create an off-screen render target with a single color buffer and a depth buffer.
	auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(backBufferFormat, m_Width, m_Height, 1, 1, sampleDesc.Count,
		sampleDesc.Quality, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	D3D12_CLEAR_VALUE colorClearValue;
	colorClearValue.Format = colorDesc.Format;
	colorClearValue.Color[0] = 0.4f;
	colorClearValue.Color[1] = 0.6f;
	colorClearValue.Color[2] = 0.9f;
	colorClearValue.Color[3] = 1.0f;

	auto colorTexture = m_Device->CreateTexture(colorDesc, &colorClearValue);
	colorTexture->SetName(L"Color Render Target");

	// Create a depth buffer.
	auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthBufferFormat, m_Width, m_Height, 1, 1, sampleDesc.Count,
		sampleDesc.Quality, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	D3D12_CLEAR_VALUE depthClearValue;
	depthClearValue.Format = depthDesc.Format;
	depthClearValue.DepthStencil = { 1.0f, 0 };

	auto depthTexture = m_Device->CreateTexture(depthDesc, &depthClearValue);
	depthTexture->SetName(L"Depth Render Target");

	// Attach the textures to the render target.
	m_RenderTarget.AttachTexture(AttachmentPoint::Color0, colorTexture);
	m_RenderTarget.AttachTexture(AttachmentPoint::DepthStencil, depthTexture);

	// Make sure the copy command queue is finished before leaving this function.
	commandQueue.WaitForFenceValue(fence);
}

bool PathTracePipeline::LoadingProgress(float loadingProgress)
{
	m_LoadingProgress = loadingProgress;
	// This function should return false to cancel the loading process.
	return !m_CancelLoading;
}


bool PathTracePipeline::InitScene(const std::wstring& sceneFile)
{
	//Input Scene file
	auto& commandQueue = m_Device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto  commandList = commandQueue.GetCommandList();

	auto scene = commandList->LoadSceneFromFile(sceneFile, std::bind(&PathTracePipeline::LoadingProgress, this, _1));

	const uint64 currSceneIdx = uint64(AppSettings::CurrentScene);
	AppSettings::EnableWhiteFurnaceMode.SetValue(currSceneIdx == uint64(Scenes::TestBuilding));

	// Load the scene (if necessary)
	if (sceneModels[currSceneIdx].NumMeshes() == 0)
	{
		if (currSceneIdx == uint64(Scenes::BoxTest) || sceneFile.c_str() == nullptr)
		{
			sceneModels[currSceneIdx].GenerateBoxTestScene();
		}
		else
		{
			ModelLoadSettings settings;
			settings.FilePath = sceneFile.c_str();
			settings.TextureDir = SceneTextureDirs[currSceneIdx];
			settings.ForceSRGB = true;
			settings.SceneScale = SceneScales[currSceneIdx];
			settings.MergeMeshes = false;
			sceneModels[currSceneIdx].CreateWithAssimp(settings);
		}
	}

	currentModel = &sceneModels[currSceneIdx];
	DX12::FlushGPU();
	meshRenderer.Initialize(currentModel);

	auto cameraRotation = m_Camera.get_Rotation();
	auto cameraFoV = m_Camera.get_FoV();
	auto distanceToObject = s.Radius / std::tanf(XMConvertToRadians(cameraFoV) / 2.0f);

	auto cameraPosition = XMVectorSet(0, 0, -distanceToObject, 1);
	//        cameraPosition      = XMVector3Rotate( cameraPosition, cameraRotation );
	auto focusPoint = XMVectorSet(s.Center.x * scale, s.Center.y * scale, s.Center.z * scale, 1.0f);
	cameraPosition = cameraPosition + focusPoint;

	m_Camera.set_Translation(cameraPosition);
	m_Camera.set_FocalPoint(focusPoint);

	//AppSettings::SunDirection.SetValue(SceneSunDirections[currSceneIdx]);

	//{
	//	// Initialize the spotlight data used for rendering
	//	const uint64 numSpotLights = Min(currentModel->SpotLights().Size(), AppSettings::MaxSpotLights);
	//	spotLights.Init(numSpotLights);

	//	for (uint64 i = 0; i < numSpotLights; ++i)
	//	{
	//		const ModelSpotLight& srcLight = currentModel->SpotLights()[i];

	//		SpotLight& spotLight = spotLights[i];
	//		spotLight.Position = srcLight.Position;
	//		spotLight.Direction = -srcLight.Direction;
	//		spotLight.Intensity = srcLight.Intensity * 2500.0f;
	//		spotLight.AngularAttenuationX = std::cos(srcLight.AngularAttenuation.x * 0.5f);
	//		spotLight.AngularAttenuationY = std::cos(srcLight.AngularAttenuation.y * 0.5f);
	//		spotLight.Range = AppSettings::SpotLightRange;
	//	}
	//}

	//buildAccelStructure = true;
}

void PathTracePipeline::PathtraceInit()
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
	pathTraceShader= CompileFromFile(L"pathtrace.hlsl", nullptr, ShaderType::Library);
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
	CreatePathTracePipelineStateObject();
	//Remember to set ray trace camera as scene camera
}

uint32_t PathTracePipeline::Run()
{
	LoadContent();
	// Only show the window after content has been loaded.
	m_Window->Show();
	auto retCode = GameFramework::Get().Run();
	// Make sure the loading task is finished
	m_LoadingTask.get();
	return retCode;
}

void PathTracePipeline::CreatePathTracePipelineStateObject()
{
	StateObjectBuilder builder;
	builder.Init(12);

	{
		// DXIL library sub-object containing all of our code
		D3D12_DXIL_LIBRARY_DESC dxilDesc = { };
		dxilDesc.DXILLibrary = pathTraceShader.ByteCode();
		builder.AddSubObject(dxilDesc);
	}

	{
		// Primary hit group
		D3D12_HIT_GROUP_DESC hitDesc = { };
		hitDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
		hitDesc.ClosestHitShaderImport = L"ClosestHitShader";
		hitDesc.HitGroupExport = L"HitGroup";
		builder.AddSubObject(hitDesc);
	}

	{
		// Primary alpha-test hit group
		D3D12_HIT_GROUP_DESC hitDesc = { };
		hitDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
		hitDesc.ClosestHitShaderImport = L"ClosestHitShader";
		hitDesc.AnyHitShaderImport = L"AnyHitShader";
		hitDesc.HitGroupExport = L"AlphaTestHitGroup";
		builder.AddSubObject(hitDesc);
	}

	{
		// Shadow hit group
		D3D12_HIT_GROUP_DESC hitDesc = { };
		hitDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
		hitDesc.ClosestHitShaderImport = L"ShadowHitShader";
		hitDesc.HitGroupExport = L"ShadowHitGroup";
		builder.AddSubObject(hitDesc);
	}

	{
		// Shadow alpha-test hit group
		D3D12_HIT_GROUP_DESC hitDesc = { };
		hitDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
		hitDesc.ClosestHitShaderImport = L"ShadowHitShader";
		hitDesc.AnyHitShaderImport = L"ShadowAnyHitShader";
		hitDesc.HitGroupExport = L"ShadowAlphaTestHitGroup";
		builder.AddSubObject(hitDesc);
	}

	{
		D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = { };
		shaderConfig.MaxAttributeSizeInBytes = 2 * sizeof(float);                      // float2 barycentrics;
		shaderConfig.MaxPayloadSizeInBytes = 4 * sizeof(float) + 4 * sizeof(uint32);   // float3 radiance + float roughness + uint pathLength + uint pixelIdx + uint setIdx + bool IsDiffuse
		builder.AddSubObject(shaderConfig);
	}

	{
		// Global root signature with all of our normal bindings
		D3D12_GLOBAL_ROOT_SIGNATURE globalRSDesc = { };
		//original version is rtRootSignature
		globalRSDesc.pGlobalRootSignature = m_RootSignature->GetD3D12RootSignature().Get();
		builder.AddSubObject(globalRSDesc);
	}

	{
		// The path tracer is recursive, so set the max recursion depth to the max path length
		D3D12_RAYTRACING_PIPELINE_CONFIG configDesc = { };
		configDesc.MaxTraceRecursionDepth = 8;
		builder.AddSubObject(configDesc);
	}

	rtPSO = builder.CreateStateObject(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

	// Get shader identifiers (for making shader records)
	ID3D12StateObjectProperties* psoProps = nullptr;
	rtPSO->QueryInterface(IID_PPV_ARGS(&psoProps));

	const void* rayGenID = psoProps->GetShaderIdentifier(L"RaygenShader");
	const void* hitGroupID = psoProps->GetShaderIdentifier(L"HitGroup");
	const void* alphaTestHitGroupID = psoProps->GetShaderIdentifier(L"AlphaTestHitGroup");
	const void* shadowHitGroupID = psoProps->GetShaderIdentifier(L"ShadowHitGroup");
	const void* shadowAlphaTestHitGroupID = psoProps->GetShaderIdentifier(L"ShadowAlphaTestHitGroup");
	const void* missID = psoProps->GetShaderIdentifier(L"MissShader");
	const void* shadowMissID = psoProps->GetShaderIdentifier(L"ShadowMissShader");

	// Make our shader tables
	{
		ShaderIdentifier rayGenRecords[1] = { ShaderIdentifier(rayGenID) };
		StructuredBufferInit sbInit;
		sbInit.Stride = sizeof(ShaderIdentifier);
		sbInit.NumElements = ArraySize_(rayGenRecords);
		sbInit.InitData = rayGenRecords;
		sbInit.ShaderTable = true;
		sbInit.Name = L"Ray Gen Shader Table";
		rtRayGenTable.Initialize(sbInit);
	}

	{
		ShaderIdentifier missRecords[2] = { ShaderIdentifier(missID), ShaderIdentifier(shadowMissID) };

		StructuredBufferInit sbInit;
		sbInit.Stride = sizeof(ShaderIdentifier);
		sbInit.NumElements = ArraySize_(missRecords);
		sbInit.InitData = missRecords;
		sbInit.ShaderTable = true;
		sbInit.Name = L"Miss Shader Table";
		rtMissTable.Initialize(sbInit);
	}

	{
		const uint32 numMeshes = uint32(currentModel->NumMeshes());

		Array<HitGroupRecord> hitGroupRecords(numMeshes * 2);

		for (uint64 i = 0; i < numMeshes; ++i)
		{
			// Use the alpha test hit group (with an any hit shader) if the material has an opacity map
			const SampleFramework12::Mesh& mesh = currentModel->Meshes()[i];
			Assert_(mesh.NumMeshParts() == 1);
			const uint32 materialIdx = mesh.MeshParts()[0].MaterialIdx;
			const MeshMaterial& material = currentModel->Materials()[materialIdx];
			const bool alphaTest = material.Textures[uint32(MaterialTextures::Opacity)] != nullptr;

			hitGroupRecords[i * 2 + 0].ID = alphaTest ? ShaderIdentifier(alphaTestHitGroupID) : ShaderIdentifier(hitGroupID);
			hitGroupRecords[i * 2 + 1].ID = alphaTest ? ShaderIdentifier(shadowAlphaTestHitGroupID) : ShaderIdentifier(shadowHitGroupID);
		}

		StructuredBufferInit sbInit;
		sbInit.Stride = sizeof(HitGroupRecord);
		sbInit.NumElements = hitGroupRecords.Size();
		sbInit.InitData = hitGroupRecords.Data();
		sbInit.ShaderTable = true;
		sbInit.Name = L"Hit Shader Table";
		rtHitTable.Initialize(sbInit);
	}

	DX12::Release(psoProps);
}

void PathTracePipeline::OpenFile()
{
	// clang-format off
	static const COMDLG_FILTERSPEC g_FileFilters[] = {
		{ L"Autodesk", L"*.fbx" },
		{ L"Collada", L"*.dae" },
		{ L"glTF", L"*.gltf;*.glb" },
		{ L"Blender 3D", L"*.blend" },
		{ L"3ds Max 3DS", L"*.3ds" },
		{ L"3ds Max ASE", L"*.ase" },
		{ L"Wavefront Object", L"*.obj" },
		{ L"Industry Foundation Classes (IFC/Step)", L"*.ifc" },
		{ L"XGL", L"*.xgl;*.zgl" },
		{ L"Stanford Polygon Library", L"*.ply" },
		{ L"AutoCAD DXF", L"*.dxf" },
		{ L"LightWave", L"*.lws" },
		{ L"LightWave Scene", L"*.lws" },
		{ L"Modo", L"*.lxo" },
		{ L"Stereolithography", L"*.stl" },
		{ L"DirectX X", L"*.x" },
		{ L"AC3D", L"*.ac" },
		{ L"Milkshape 3D", L"*.ms3d" },
		{ L"TrueSpace", L"*.cob;*.scn" },
		{ L"Ogre XML", L"*.xml" },
		{ L"Irrlicht Mesh", L"*.irrmesh" },
		{ L"Irrlicht Scene", L"*.irr" },
		{ L"Quake I", L"*.mdl" },
		{ L"Quake II", L"*.md2" },
		{ L"Quake III", L"*.md3" },
		{ L"Quake III Map/BSP", L"*.pk3" },
		{ L"Return to Castle Wolfenstein", L"*.mdc" },
		{ L"Doom 3", L"*.md5*" },
		{ L"Valve Model", L"*.smd;*.vta" },
		{ L"Open Game Engine Exchange", L"*.ogx" },
		{ L"Unreal", L"*.3d" },
		{ L"BlitzBasic 3D", L"*.b3d" },
		{ L"Quick3D", L"*.q3d;*.q3s" },
		{ L"Neutral File Format", L"*.nff" },
		{ L"Sense8 WorldToolKit", L"*.nff" },
		{ L"Object File Format", L"*.off" },
		{ L"PovRAY Raw", L"*.raw" },
		{ L"Terragen Terrain", L"*.ter" },
		{ L"Izware Nendo", L"*.ndo" },
		{ L"All Files", L"*.*" }
	};
	// clang-format on

	ComPtr<IFileOpenDialog> pFileOpen;
	HRESULT                 hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_PPV_ARGS(&pFileOpen));

	if (SUCCEEDED(hr))
	{
		// Create an event handling object, and hook it up to the dialog.
		// ComPtr<IFileDialogEvents> pDialogEvents;
		// hr = DialogEventHandler_CreateInstance( IID_PPV_ARGS( &pDialogEvents ) );

		if (SUCCEEDED(hr))
		{
			// Setup filters.
			hr = pFileOpen->SetFileTypes(_countof(g_FileFilters), g_FileFilters);
			pFileOpen->SetFileTypeIndex(40);  // All Files (*.*)

			// Show the open dialog box.
			if (SUCCEEDED(pFileOpen->Show(m_Window->GetWindowHandle())))
			{
				ComPtr<IShellItem> pItem;
				if (SUCCEEDED(pFileOpen->GetResult(&pItem)))
				{
					PWSTR pszFilePath;
					if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
					{
						// try to load the scene file (asynchronously).
						m_LoadingTask =
							std::async(std::launch::async, std::bind(&PathTracePipeline::InitScene, this, pszFilePath));

						CoTaskMemFree(pszFilePath);
					}
				}
			}
		}
	}
}