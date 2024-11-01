#include "pch.h"
#include "Engine.h"

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

bool Engine::Initialize(HWND hwnd, UINT width, UINT height)
{
	m_width = width;
	m_height = height;
	m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);

	// 디버그 레이어 활성화
	IFDEBUG(
		ComPtr<ID3D12Debug6> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
			debugController->EnableDebugLayer();
		}
	);

	// DirectX 12 객체 생성
	if (!CreateDevice()) {
		return false;
	}
	if (!CreateCommandQueue()) {
		return false;
	}
	if (!CreateSwapChain(hwnd)) {
		return false;
	}
	if (!CreateDescriptorHeaps()) {
		return false;
	}
	if (!CreateRenderTargetViews()) {
		return false;
	}
	if (!CreateCommandAllocatorAndList()) {
		return false;
	}
	if (!CreateFence()) {
		return false;
	}
	if (!CreateRootSignature()) {
		return false;
	}
	if (!CreatePipelineState()) {
		return false;
	}
	if (!CreateVertexBuffer()) {
		return false;
	}

	return true;
}

void Engine::Update()
{
	// TODO: 업데이트 로직 구현
}

void Engine::Render()
{
	ThrowIfFailed(m_commandAllocator->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

	const CD3DX12_VIEWPORT viewport(0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height));
	const CD3DX12_RECT scissorRect(0, 0, m_width, m_height);

	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	m_commandList->RSSetViewports(1, &viewport);
	m_commandList->RSSetScissorRects(1, &scissorRect);

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
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// 삼각형 그리기
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->DrawInstanced(3, 1, 0, 0);

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
    CloseHandle(m_fenceEvent);
}

bool Engine::CreateDevice()
{
	ComPtr<IDXGIFactory7> factory;
	UINT dxgiFactoryFlag = 0;

	// 디버그 모드면 FACTORY_FLAG에 디버그 플래그 추가
	IFDEBUG(dxgiFactoryFlag |= DXGI_CREATE_FACTORY_DEBUG;);

	// 팩토리 생성
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlag, IID_PPV_ARGS(&factory)));

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

	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

	return true;
}

bool Engine::CreateSwapChain(HWND hwnd)
{
	ComPtr<IDXGIFactory7> factory;
	UINT dxgiFactoryFlag = 0;
	IFDEBUG(dxgiFactoryFlag |= DXGI_CREATE_FACTORY_DEBUG;);
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlag, IID_PPV_ARGS(&factory)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FRAME_BUFFER_COUNT;
	swapChainDesc.Width = m_width;
	swapChainDesc.Height = m_height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(
		m_commandQueue.Get(), 
		hwnd, 
		&swapChainDesc, 
		nullptr, 
		nullptr, 
		&swapChain));

	ThrowIfFailed(swapChain.As(&m_swapChain));
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	return true;
}

bool Engine::CreateDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = FRAME_BUFFER_COUNT;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc,
		IID_PPV_ARGS(&m_rtvHeap)));

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
		ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));

		// 2-2. m_renderTargets[n]에 대한 RTV 생성
		m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);

		// 2-3. 다음 디스크립터 위치로 이동
		rtvHandle.Offset(1, GetRtvDescriptorSize());
	}

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
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(0, nullptr, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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
		  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// 파이프라인 상태 생성
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(m_vertexShader.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(m_pixelShader.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
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
		LogInitializationError("Pipeline State Creation", "Failed to create graphics pipeline state");
		return false;
	}

	return true;
}

bool Engine::CreateVertexBuffer()
{
	// 삼각형 정점 데이터
	Vertex triangleVertices[] =
	{
		{ { 0.0f, 0.25f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { 0.25f, -0.25f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { -0.25f, -0.25f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
	};

	const UINT vertexBufferSize = sizeof(triangleVertices);

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
	memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
	m_vertexBuffer->Unmap(0, nullptr);

	// 버퍼 뷰 생성
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = vertexBufferSize;

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

void Engine::LogInitializationError(const std::string& step, const std::string& error)
{
	// 로깅 시스템을 사용하여 에러를 기록
	OutputDebugStringA(("Error during " + step + ": " + error + "\n").c_str());
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
