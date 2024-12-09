#include "pch.h"
#include "Engine.h"
#include "PhysicsObject.h"
#include "PhysicsEngine.h"
#include "MemoryManager.h"
#include "EventManager.h"
#include "Logger.h"

Engine::Engine()
	: m_width(0)
	, m_height(0)
	, m_aspectRatio(0.0f)
	, m_frameIndex(0)
	, m_fenceEvent(nullptr)
{
	for (UINT i = 0; i < FRAME_BUFFER_COUNT; ++i) {
		m_fenceValues[i] = 0;
	}
}

Engine::~Engine()
{
	Cleanup();
}

Engine& Engine::Instance()
{
	static Engine instance;
	return instance;
}

bool Engine::Initialize(HWND hwnd, UINT width, UINT height)
{
	// 로거 초기화
	Logger::Instance().AddOutput(std::make_unique<DebugOutput>());
	Logger::Instance().AddOutput(std::make_unique<FileOutput>("Game.log"));

	Logger::Instance().Info("Engine 초기화 시작");

	m_width = width;
	m_height = height;
	m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);

	// 메모리 관리자 초기화
	if (!Memory::InitializeMemory()) {
		Logger::Instance().Fatal("메모리 관리자 초기화 실패");
		return false;
	}

	// 이벤트 핸들러 등록
	RegisterEventHandlers();

	// 디버그 레이어 활성화
	IFDEBUG(
		ComPtr<ID3D12Debug6> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
			debugController->EnableDebugLayer();
			Logger::Instance().Debug("디버그 레이어 활성화");
		}
	);

	// DirectX 12 객체 생성
	if (!CreateDevice()) {
		Logger::Instance().Fatal("디바이스 생성 실패");
		return false;
	}
	if (!CreateCommandQueue()) {
		Logger::Instance().Fatal("커맨드 큐 생성 실패");
		return false;
	}
	if (!CreateSwapChain(hwnd)) {
		Logger::Instance().Fatal("스왑체인 생성 실패");
		return false;

	}
	if (!CreateRTVDescriptorHeaps()) {
		Logger::Instance().Fatal("RTV 디스크립터 힙 생성 실패");
		return false;

	}
	if (!CreateRenderTargetViews()) {
		Logger::Instance().Fatal("RTV 생성 실패");
		return false;
	}
	if (!CreateConstantBuffer()) {
		Logger::Instance().Fatal("상수 버퍼 생성 실패");
		return false;
	}
	if (!CreateLightConstantBuffer()) {
		Logger::Instance().Fatal("라이트 상수 버퍼 생성 실패");
		return false;
	}
	if (!CreateTexture(L"Texture/checker.dds")) {
		Logger::Instance().Fatal("텍스처 생성 실패");
		return false;
	}
	if (!CreateDescHeap()) {
		Logger::Instance().Fatal("디스크립터 힙 생성 실패");
		return false;
	}
	if (!CreateCommandAllocatorAndList()) {
		Logger::Instance().Fatal("커맨드 할당자 및 커맨드 리스트 생성 실패");
		return false;
	}
	if (!CreateFence()) {
		Logger::Instance().Fatal("펜스 생성 실패");
		return false;
	}
	if (!CreateRootSignature()) {
		Logger::Instance().Fatal("루트 시그니처 생성 실패");
		return false;
	}
	if (!CreatePipelineState()) {
		Logger::Instance().Fatal("파이프라인 상태 생성 실패");
		return false;
	}
	if (!CreateVertexBuffer()) {
		Logger::Instance().Fatal("정점 버퍼 생성 실패");
		return false;
	}
	if (!CreateIndexBuffer()) {
		Logger::Instance().Fatal("인덱스 버퍼 생성 실패");
		return false;
	}

	// 물리 엔진 초기화
	m_physicsEngine = std::make_unique<PhysicsEngine>();
	if (!m_physicsEngine->Initialize()) {
		Logger::Instance().Fatal("물리 엔진 초기화 실패");
		return false;
	}

	// 지면 생성
	m_ground = m_physicsEngine->CreateBox(
		PxVec3(0.0f, 0.0f, 0.0f),
		PxVec3(100.0f, 0.5f, 100.0f),
		PhysicsObjectType::STATIC,
		CollisionGroup::Ground,
		CollisionGroup::Default | CollisionGroup::Player);

	// 물리 박스 생성 (크기는 렌더링되는 큐브와 동일하게)
	m_physicsBox = m_physicsEngine->CreateBox(
		PxVec3(0.0f, 5.0f, 0.0f),  // 시작 위치
		PxVec3(0.5f, 0.5f, 0.5f),  // 크기
		PhysicsObjectType::DYNAMIC, // 동적 객체
		CollisionGroup::Default,    // 기본 그룹
		CollisionGroup::Ground);    // 지면과 충돌

	// 초기 변환 행렬 설정
	m_worldMatrix = XMMatrixIdentity();
	m_viewMatrix = XMMatrixLookAtLH(
		XMVectorSet(0.0f, 5.0f, -5.0f, 1.0f),  // 카메라 위치
		XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),   // 보는 지점
		XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f)    // 업 벡터
	);
	m_projectionMatrix = XMMatrixPerspectiveFovLH(
		XM_PIDIV4,                              // 시야각(45도)
		m_aspectRatio,                          // 화면 비율
		0.1f,                                   // 근평면
		100.0f                                  // 원평면
	);

	// 회전 애니메이션 초기화
	m_rotationAngle = 0.0f;
	m_lastTick = GetTickCount64();

	Logger::Instance().Info("Engine 초기화 완료");
	return true;
}

void Engine::Update()
{
	// 프레임 메모리 초기화
	Memory::BeginFrameMemory();

	// 모든 큐에 있는 이벤트 처리
	EventManager::Instance().Update();
	
	// 델타 시간 계산
	ULONGLONG currentTick = GetTickCount64();
	float deltaTime = (currentTick - m_lastTick) / 1000.0f;
	m_lastTick = currentTick;

	// deltaTime이 0이하인 경우 최소값으로 설정
	if (deltaTime <= 0.0f) {
		deltaTime = 1.0f / 600.0f;  // 기본 프레임 레이트
		//Logger::Instance().Warning("0 또는 음수 DT 감지, 기본 값 사용: {}", deltaTime);
	}

	// 물리 엔진 업데이트
	m_physicsEngine->Update(deltaTime);

	// 월드 행렬 업데이트
	UpdateWorldMatrix();

	//// 회전 각도 업데이트
	m_rotationAngle += deltaTime;

	// 라이트 방향 업데이트 (원을 그리며 회전)
	float lightAngle = m_rotationAngle * 0.5f;  // 큐브보다 천천히 회전
	m_lightConstants.lightDirection.x = sinf(lightAngle);
	m_lightConstants.lightDirection.z = cosf(lightAngle);
	m_lightConstants.lightDirection.y = -0.5f;  // 약간 위에서 비추도록

	// 정규화
	XMVECTOR lightDir = XMLoadFloat4(&m_lightConstants.lightDirection);
	lightDir = XMVector3Normalize(lightDir);
	XMStoreFloat4(&m_lightConstants.lightDirection, lightDir);

	// 라이트 상수 버퍼 업데이트
	memcpy(m_lightConstantBufferMappedData, &m_lightConstants, sizeof(m_lightConstants));
}

void Engine::Render()
{
	ThrowIfFailed(m_commandAllocator->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

	// 상수 버퍼 업데이트
	UpdateConstantBuffer();

	// 뷰포트와 시저렉트 설정
	const CD3DX12_VIEWPORT viewport(0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height));
	const CD3DX12_RECT scissorRect(0, 0, m_width, m_height);

	m_commandList->RSSetViewports(1, &viewport);
	m_commandList->RSSetScissorRects(1, &scissorRect);

	// 루트 시그니처와 디스크립터 힙 설정
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	// CBV/SRV 힙 설정
	ID3D12DescriptorHeap* ppHeaps[] = { m_descHeap.Get() };
	m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// Transform CBV 설정 (첫 번째 위치)
	m_commandList->SetGraphicsRootDescriptorTable(0, m_descHeap->GetGPUDescriptorHandleForHeapStart());

	// Light CBV 설정 (두 번째 위치)
	CD3DX12_GPU_DESCRIPTOR_HANDLE lightCbvHandle(m_descHeap->GetGPUDescriptorHandleForHeapStart());
	lightCbvHandle.Offset(m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	m_commandList->SetGraphicsRootDescriptorTable(1, lightCbvHandle);

	// Texture SRV 설정 (세 번째 위치)
	CD3DX12_GPU_DESCRIPTOR_HANDLE textureSrvHandle(m_descHeap->GetGPUDescriptorHandleForHeapStart());
	textureSrvHandle.Offset(2, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	m_commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandle);

	// 리소스 배리어
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_renderTargets[m_frameIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_commandList->ResourceBarrier(1, &barrier);

	// 렌더 타겟 설정
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
		m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		m_frameIndex, GetRtvDescriptorSize());

	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// 화면 클리어
	const float clearColor[] = { 0.0f, 0.0f, 0.2f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	////////// RENDER ///////////
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->IASetIndexBuffer(&m_indexBufferView);
	m_commandList->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);
	/////////////////////////////

	// 리소스 배리어
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_renderTargets[m_frameIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_commandList->ResourceBarrier(1, &barrier);

	ThrowIfFailed(m_commandList->Close());

	// 커맨드 리스트 실행
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// 화면 표시
	ThrowIfFailed(m_swapChain->Present(1, 0));

	WaitForGpu();
	MoveToNextFrame();
}

void Engine::Cleanup()
{
	WaitForGpu();

	UnregisterEventHandlers();

	m_physicsEngine.reset();

    CloseHandle(m_fenceEvent);
}

void Engine::UpdateWorldMatrix()
{
	if (m_physicsBox) {
		// 물리 객체의 변환 행렬을 가져와서 렌더링에 사용할 월드 행렬 업데이트
		m_worldMatrix = m_physicsBox->GetTransformMatrix();
	}
}

bool Engine::CreateDevice()
{
	ComPtr<IDXGIFactory7> factory;
	UINT dxgiFactoryFlag = 0;

	// 디버그 모드면 FACTORY_FLAG에 디버그 플래그 추가
	IFDEBUG(dxgiFactoryFlag |= DXGI_CREATE_FACTORY_DEBUG;);

	// 팩토리 생성
	if (FAILED(CreateDXGIFactory2(dxgiFactoryFlag, IID_PPV_ARGS(&factory)))) {
		return false;
	}

	ComPtr<IDXGIAdapter1> hardwareAdapter;
	for (UINT adapterIndex = 0;; ++adapterIndex) {
		hardwareAdapter.Reset();

		if (FAILED(factory->EnumAdapters1(adapterIndex, &hardwareAdapter))) {
			break;
		}

		DXGI_ADAPTER_DESC1 desc;
		hardwareAdapter->GetDesc1(&desc);

		// 소프트웨어 어댑터는 무시
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
			continue;
		}

		// 디바이스 생성
		if (SUCCEEDED(D3D12CreateDevice(
			hardwareAdapter.Get(), 
			D3D_FEATURE_LEVEL_11_0, 
			IID_PPV_ARGS(&m_device)))) {
			Logger::Instance().Info("디바이스 생성 성공");
			break;
		}
	}

	return nullptr != m_device;
}

bool Engine::CreateCommandQueue()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	if (FAILED(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)))) {
		return false;
	}

	Logger::Instance().Info("커맨드 큐 생성 성공");
	return true;
}

bool Engine::CreateSwapChain(HWND hwnd)
{
	ComPtr<IDXGIFactory7> factory;
	UINT dxgiFactoryFlag = 0;
	IFDEBUG(dxgiFactoryFlag |= DXGI_CREATE_FACTORY_DEBUG;);
	if (FAILED(CreateDXGIFactory2(dxgiFactoryFlag, IID_PPV_ARGS(&factory)))) {
		return false;
	}

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FRAME_BUFFER_COUNT;
	swapChainDesc.Width = m_width;
	swapChainDesc.Height = m_height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	if (FAILED(factory->CreateSwapChainForHwnd(m_commandQueue.Get(), hwnd, 
		&swapChainDesc, nullptr, nullptr, &swapChain))) {
		return false;
	}

	if (FAILED(swapChain.As(&m_swapChain))) {
		return false;
	}
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	Logger::Instance().Info("스왑체인 생성 성공");
	return true;
}

bool Engine::CreateRTVDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = FRAME_BUFFER_COUNT;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	if (FAILED(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)))) {
		return false;
	}

	Logger::Instance().Info("RTV 디스크립터 힙 생성 성공");
	return true;
}

bool Engine::CreateRenderTargetViews()
{
	// 1. RTV 힙의 시작 핸들 얻기
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

	//[RTV0] [RTV1]
	//	^     ^
	//	|     |
	//	|     +-- 두 번째 위치
	//	+ --------첫 번째 위치
	// 2. 각 백버퍼에 대한 RTV 생성
	for (UINT n = 0; n < FRAME_BUFFER_COUNT; ++n) {
		// 2-1. 스왑체인으로부터 버퍼 얻기
		if (FAILED(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])))) {
			return false;
		}

		// 2-2. m_renderTargets[n]에 대한 RTV 생성
		m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);

		// 2-3. 다음 디스크립터 위치로 이동
		rtvHandle.Offset(1, GetRtvDescriptorSize());
	}

	Logger::Instance().Info("RTV 생성 성공");
	return true;
}

bool Engine::CreateCommandAllocatorAndList()
{
	ThrowIfFailed(m_device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&m_commandAllocator)));

	ThrowIfFailed(m_device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_commandAllocator.Get(),
		nullptr,
		IID_PPV_ARGS(&m_commandList)));

	ThrowIfFailed(m_commandList->Close());
	return true;
}

bool Engine::CreateFence()
{
	ThrowIfFailed(m_device->CreateFence(m_fenceValues[m_frameIndex],
		D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	m_fenceValues[m_frameIndex]++;

	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_fenceEvent == nullptr) {
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		return false;
	}

	return true;
}

bool Engine::CreateRootSignature()
{
	// 정적 샘플러 생성
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.ShaderRegister = 0; // s0 레지스터
	samplerDesc.RegisterSpace = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// 디스크립터 테이블 설정
	D3D12_DESCRIPTOR_RANGE ranges[3] = {};

	// 변환 행렬용 range
	ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	ranges[0].NumDescriptors = 1;
	ranges[0].BaseShaderRegister = 0;	// b0 레지스터
	ranges[0].RegisterSpace = 0;
	ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// 라이팅용 range
	ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	ranges[1].NumDescriptors = 1;
	ranges[1].BaseShaderRegister = 1;	// b1 레지스터
	ranges[1].RegisterSpace = 0;
	ranges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// 텍스처용 range
	ranges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ranges[2].NumDescriptors = 1;
	ranges[2].BaseShaderRegister = 0;	// t0 레지스터
	ranges[2].RegisterSpace = 0;
	ranges[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// 루트 파라미터 설정
	D3D12_ROOT_PARAMETER rootParameters[3] = {};

	// 변환 행렬용 파라미터
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[0].DescriptorTable.pDescriptorRanges = &ranges[0];
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// 라이팅용 파라미터
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[1].DescriptorTable.pDescriptorRanges = &ranges[1];
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// 텍스처용 파라미터
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[2].DescriptorTable.pDescriptorRanges = &ranges[2];
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// 루트 시그니처 생성
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.NumParameters = _countof(rootParameters);
	rootSignatureDesc.pParameters = rootParameters;
	rootSignatureDesc.NumStaticSamplers = 1;
	rootSignatureDesc.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(),
		signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

	return true;
}

bool Engine::CreatePipelineState()
{
	// 셰이더 컴파일 및 로드
	CompileShaders();

	// 정점 입력 레이아웃 정의
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
		  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
		  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28,
		  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40,
		  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// 파이프라인 상태 생성
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(m_vertexShader.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(m_pixelShader.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;

	HRESULT hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));

	if (FAILED(hr)) {
		return false;
	}

	return true;
}

bool Engine::CreateVertexBuffer()
{
	// 큐브의 정점 데이터
	Vertex cubeVertices[] = {
		// 앞면 (z = 0.5f)
		{ XMFLOAT3(-0.5f, -0.5f, 0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(-0.5f,  0.5f, 0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(0.5f,  0.5f, 0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(0.5f, -0.5f, 0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },

		// 뒷면 (z = -0.5f)
		{ XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(0.5f,  0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },

		// 윗면 (y = 0.5f)
		{ XMFLOAT3(-0.5f, 0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(0.5f, 0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(0.5f, 0.5f,  0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-0.5f, 0.5f,  0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },

		// 아랫면 (y = -0.5f)
		{ XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(0.5f, -0.5f,  0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },

		// 오른쪽면 (x = 0.5f)
		{ XMFLOAT3(0.5f, -0.5f,  0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(0.5f,  0.5f,  0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(0.5f,  0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },

		// 왼쪽면 (x = -0.5f)
		{ XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }
	};


	const UINT vertexBufferSize = sizeof(cubeVertices);

	// 버퍼 생성
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

	ThrowIfFailed(m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_vertexBuffer)));

	// 데이터 복사
	UINT8* pVertexDataBegin;
	CD3DX12_RANGE readRange(0, 0);
	ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, cubeVertices, sizeof(cubeVertices));
	m_vertexBuffer->Unmap(0, nullptr);

	// 버퍼 뷰 생성
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = vertexBufferSize;

	return true;
}

bool Engine::CreateIndexBuffer()
{
	// 큐브의 인덱스 데이터
	UINT indices[] = {
		// 앞면 (0-3)
		0, 1, 2,    // 첫 번째 삼각형
		0, 2, 3,    // 두 번째 삼각형

		// 뒷면 (4-7)
		4, 5, 6,
		4, 6, 7,

		// 윗면 (8-11)
		8, 9, 10,
		8, 10, 11,

		// 아랫면 (12-15)
		12, 13, 14,
		12, 14, 15,

		// 오른쪽면 (16-19)
		16, 17, 18,
		16, 18, 19,

		// 왼쪽면 (20-23)
		20, 21, 22,
		20, 22, 23
	};

	m_indexCount = ARRAYSIZE(indices);
	const UINT indexBufferSize = sizeof(indices);

	// 인덱스 버퍼 생성
	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

	ThrowIfFailed(m_device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_indexBuffer)));

	// 데이터 복사
	UINT8* pIndexDataBegin;
	CD3DX12_RANGE readRange(0, 0);
	ThrowIfFailed(m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
	memcpy(pIndexDataBegin, indices, indexBufferSize);
	m_indexBuffer->Unmap(0, nullptr);

	// 인덱스 버퍼 뷰 생성
	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_indexBufferView.SizeInBytes = indexBufferSize;

	return true;
}

bool Engine::CompileShaders()
{
	UINT compileFlags = 0;
	IFDEBUG(compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;);
	ComPtr<ID3DBlob> errorBlob = nullptr;

	// 버텍스 셰이더 컴파일
	ThrowIfFailed(D3DCompileFromFile(
		L"shaders.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"VSMain",
		"vs_5_0",
		compileFlags,
		0,
		&m_vertexShader,
		&errorBlob));

	// 픽셀 셰이더 컴파일
	ThrowIfFailed(D3DCompileFromFile(
		L"shaders.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"PSMain",
		"ps_5_0",
		compileFlags,
		0,
		&m_pixelShader,
		&errorBlob));

	return true;
}

bool Engine::CreateConstantBuffer()
{
	// 상수 버퍼는 256 바이트 정렬이 필요
	const UINT constantBufferSize = (sizeof(ObjectConstants) + 255) & ~255;

	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);

	// 상수 버퍼 생성
	if(FAILED(m_device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_constantBuffer)))) {
		return false;
	}

	// 상수 버퍼를 CPU 메모리에 매핑
	CD3DX12_RANGE readRange(0, 0);
	if (FAILED(m_constantBuffer->Map(0, &readRange,
		reinterpret_cast<void**>(&m_constantBufferMappedData)))) {
		return false;
	}

	Logger::Instance().Info("상수 버퍼 생성 성공");
	return true;
}

bool Engine::CreateLightConstantBuffer()
{
	// 상수 버퍼는 256바이트 정렬이 필요
	const UINT constantBufferSize = (sizeof(LightConstants) + 255) & ~255;

	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);

	if (FAILED(m_device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_lightConstantBuffer)))) {
		return false;
	}

	// 상수 버퍼를 CPU 메모리에 매핑
	CD3DX12_RANGE readRange(0, 0);
	if (FAILED(m_lightConstantBuffer->Map(0, &readRange,
		reinterpret_cast<void**>(&m_lightConstantBufferMappedData)))) {
		return false;
	}

	// 초기 라이팅 값 설정
	m_lightConstants.lightDirection = XMFLOAT4(-0.577f, -0.577f, -0.577f, 0.0f);  // 대각선 방향
	m_lightConstants.lightColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);              // 흰색 조명
	m_lightConstants.ambientColor = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);            // 회색 주변광
	m_lightConstants.eyePosition = XMFLOAT4(0.0f, 0.0f, -5.0f, 0.0f);            // 카메라 위치

	// 초기값 복사
	memcpy(m_lightConstantBufferMappedData, &m_lightConstants, sizeof(m_lightConstants));

	Logger::Instance().Info("라이트 상수 버퍼 생성 성공");
	return true;
}

bool Engine::CreateTexture(const wchar_t* filename)
{
	// 리소스 업로드 배치 생성
	DirectX::ResourceUploadBatch resourceUpload(m_device.Get());
	resourceUpload.Begin();

	// DDS 텍스처 로드
	if (FAILED(DirectX::CreateDDSTextureFromFile(
		m_device.Get(),
		resourceUpload,
		filename,
		m_texture.ReleaseAndGetAddressOf()))) {
		return false;
	}

	// 리소스 업로드 실행
	auto uploadResourcesFinished = resourceUpload.End(m_commandQueue.Get());
	uploadResourcesFinished.wait();

	return true;
}

bool Engine::CreateDescHeap()
{
	// CBV 2개와 SRV 1개를 위한 디스크립터 힙 생성
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 3;  // CBV 2개 + SRV 1개
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	ThrowIfFailed(m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_descHeap)));

	// 변환 행렬용 CBV 생성
	D3D12_CONSTANT_BUFFER_VIEW_DESC transformCbvDesc = {};
	transformCbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
	transformCbvDesc.SizeInBytes = (sizeof(ObjectConstants) + 255) & ~255;

	// 라이팅용 CBV 생성
	D3D12_CONSTANT_BUFFER_VIEW_DESC lightCbvDesc = {};
	lightCbvDesc.BufferLocation = m_lightConstantBuffer->GetGPUVirtualAddress();
	lightCbvDesc.SizeInBytes = (sizeof(LightConstants) + 255) & ~255;

	// 텍스처용 SRV 생성
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_texture->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = m_texture->GetDesc().MipLevels;

	// 디스크립터 핸들 계산
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_descHeap->GetCPUDescriptorHandleForHeapStart());
	UINT handleIncrement = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// CBV들과 SRV 생성
	m_device->CreateConstantBufferView(&transformCbvDesc, handle);
	handle.Offset(handleIncrement);
	m_device->CreateConstantBufferView(&lightCbvDesc, handle);
	handle.Offset(handleIncrement);
	m_device->CreateShaderResourceView(m_texture.Get(), &srvDesc, handle);

	return true;
}

void Engine::RegisterEventHandlers()
{
	auto collisionHandler = m_collisionHandlerIds.emplace_back(
		EventManager::Instance().Subscribe<Event::CollisionEvent>(
			Event::EventCallback<Event::CollisionEvent>(
			[this](const Event::CollisionEvent& event) {
				// 충돌 이벤트 처리
				Logger::Instance().Debug("'{}'와 '{}'가 충돌 발생. 위치: ({}, {}, {}), 노말: ({}, {}, {}), 충격량: ({})",
					event.actor1->getName(),
					event.actor2->getName(),
					event.position.x,
					event.position.y,
					event.position.z,
					event.normal.x,
					event.normal.y,
					event.normal.z,
					event.impulse);
			})
		)
	);

	auto resourceHandler = m_resourceHandlerIds.emplace_back(
		EventManager::Instance().Subscribe<Event::ResourceEvent>(
			Event::EventCallback <Event::ResourceEvent>(
			[this](const Event::ResourceEvent& event) {
				// 리소스 이벤트 처리
				switch (event.type) {
				case Event::ResourceEvent::Type::Started:
					Logger::Instance().Debug("리소스 로딩 시작: {}", event.path);
					break;
				case Event::ResourceEvent::Type::Completed:
					Logger::Instance().Debug("리소스 로딩 완료: {}", event.path);
					break;
				case Event::ResourceEvent::Type::Failed:
					Logger::Instance().Error("리소스 로딩 실패: {}, 오류: {}",
						event.path, event.error);
					break;
				}
			})
		)
	);

	auto inputHandler = m_inputHandlerIds.emplace_back(
		EventManager::Instance().Subscribe<Event::InputEvent>(
			Event::EventCallback<Event::InputEvent>(
			[this](const Event::InputEvent& event) {
				// 입력 이벤트 처리
				switch (event.type) {
				case Event::InputEvent::Type::KeyDown:
					Logger::Instance().Debug("키 눌림: {}", event.code);
					break;
				case Event::InputEvent::Type::KeyUp:
					Logger::Instance().Debug("키 뗌: {}", event.code);
					break;
				case Event::InputEvent::Type::MouseMove:
					Logger::Instance().Debug("마우스 이동: ({}, {})", event.x, event.y);
					break;
				case Event::InputEvent::Type::MouseButtonDown:
					Logger::Instance().Debug("마우스 버튼 눌림: {}", event.code);
					break;
				case Event::InputEvent::Type::MouseButtonUp:
					Logger::Instance().Debug("마우스 버튼 뗌: {}", event.code);
					break;
				}
			})
		)
	);
}

void Engine::UnregisterEventHandlers()
{
	for (auto id : m_collisionHandlerIds) {
		EventManager::Instance().Unsubscribe<Event::CollisionEvent>(id);
	}
	for (auto id : m_resourceHandlerIds) {
		EventManager::Instance().Unsubscribe<Event::ResourceEvent>(id);
	}
	for (auto id : m_inputHandlerIds) {
		EventManager::Instance().Unsubscribe<Event::InputEvent>(id);
	}
}

void Engine::UpdateConstantBuffer()
{
	ObjectConstants constants;
	constants.worldMatrix = XMMatrixTranspose(m_worldMatrix);
	constants.viewMatrix = XMMatrixTranspose(m_viewMatrix);
	constants.projectionMatrix = XMMatrixTranspose(m_projectionMatrix);

	memcpy(m_constantBufferMappedData, &constants, sizeof(constants));
}

void Engine::WaitForGpu()
{
	// 1. 현재 프레임의 Fence 값으로 Signal
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValues[m_frameIndex]));

	// 2. Fence 값이 현재 프레임의 값에 도달할 때까지 대기하라는 이벤트를 설정
	ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));

	// 3. 설정한 이벤트가 발생할 때까지 대기 (Blocking)
	WaitForSingleObject(m_fenceEvent, INFINITE);
}

void Engine::MoveToNextFrame()
{
	// 1. 현재 프레임의 Fence값 저장
	const UINT64 currentFenceValue = m_fenceValues[m_frameIndex];

	// 2. CommandQueue에 Signal
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), currentFenceValue));

	// 3. 다음 프레임으로 이동
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// 4. 다음 프레임의 Fence값이 현재 프레임의 값에 도달할 때까지 대기
	if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex]) {
		// 4-1. Fence값이 도달할 때까지 대기 (GPU가 현재 프레임 작업을 완료할 때까지 대기)
		ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	// 5. 다음 프레임의 Fence값 설정
	m_fenceValues[m_frameIndex] = currentFenceValue + 1;
}
