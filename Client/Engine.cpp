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
	// �ΰ� �ʱ�ȭ
	Logger::Instance().AddOutput(std::make_unique<DebugOutput>());
	Logger::Instance().AddOutput(std::make_unique<FileOutput>("Game.log"));

	Logger::Instance().Info("Engine �ʱ�ȭ ����");

	m_width = width;
	m_height = height;
	m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);

	// �޸� ������ �ʱ�ȭ
	if (!Memory::InitializeMemory()) {
		Logger::Instance().Fatal("�޸� ������ �ʱ�ȭ ����");
		return false;
	}

	// �̺�Ʈ �ڵ鷯 ���
	RegisterEventHandlers();

	// ����� ���̾� Ȱ��ȭ
	IFDEBUG(
		ComPtr<ID3D12Debug6> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
			debugController->EnableDebugLayer();
			Logger::Instance().Debug("����� ���̾� Ȱ��ȭ");
		}
	);

	// DirectX 12 ��ü ����
	if (!CreateDevice()) {
		Logger::Instance().Fatal("����̽� ���� ����");
		return false;
	}
	if (!CreateCommandQueue()) {
		Logger::Instance().Fatal("Ŀ�ǵ� ť ���� ����");
		return false;
	}
	if (!CreateSwapChain(hwnd)) {
		Logger::Instance().Fatal("����ü�� ���� ����");
		return false;

	}
	if (!CreateRTVDescriptorHeaps()) {
		Logger::Instance().Fatal("RTV ��ũ���� �� ���� ����");
		return false;

	}
	if (!CreateRenderTargetViews()) {
		Logger::Instance().Fatal("RTV ���� ����");
		return false;
	}
	if (!CreateConstantBuffer()) {
		Logger::Instance().Fatal("��� ���� ���� ����");
		return false;
	}
	if (!CreateLightConstantBuffer()) {
		Logger::Instance().Fatal("����Ʈ ��� ���� ���� ����");
		return false;
	}
	if (!CreateTexture(L"Texture/checker.dds")) {
		Logger::Instance().Fatal("�ؽ�ó ���� ����");
		return false;
	}
	if (!CreateDescHeap()) {
		Logger::Instance().Fatal("��ũ���� �� ���� ����");
		return false;
	}
	if (!CreateCommandAllocatorAndList()) {
		Logger::Instance().Fatal("Ŀ�ǵ� �Ҵ��� �� Ŀ�ǵ� ����Ʈ ���� ����");
		return false;
	}
	if (!CreateFence()) {
		Logger::Instance().Fatal("�潺 ���� ����");
		return false;
	}
	if (!CreateRootSignature()) {
		Logger::Instance().Fatal("��Ʈ �ñ״�ó ���� ����");
		return false;
	}
	if (!CreatePipelineState()) {
		Logger::Instance().Fatal("���������� ���� ���� ����");
		return false;
	}
	if (!CreateVertexBuffer()) {
		Logger::Instance().Fatal("���� ���� ���� ����");
		return false;
	}
	if (!CreateIndexBuffer()) {
		Logger::Instance().Fatal("�ε��� ���� ���� ����");
		return false;
	}

	// ���� ���� �ʱ�ȭ
	m_physicsEngine = std::make_unique<PhysicsEngine>();
	if (!m_physicsEngine->Initialize()) {
		Logger::Instance().Fatal("���� ���� �ʱ�ȭ ����");
		return false;
	}

	// ���� ����
	m_ground = m_physicsEngine->CreateBox(
		PxVec3(0.0f, 0.0f, 0.0f),
		PxVec3(100.0f, 0.5f, 100.0f),
		PhysicsObjectType::STATIC,
		CollisionGroup::Ground,
		CollisionGroup::Default | CollisionGroup::Player);

	// ���� �ڽ� ���� (ũ��� �������Ǵ� ť��� �����ϰ�)
	m_physicsBox = m_physicsEngine->CreateBox(
		PxVec3(0.0f, 5.0f, 0.0f),  // ���� ��ġ
		PxVec3(0.5f, 0.5f, 0.5f),  // ũ��
		PhysicsObjectType::DYNAMIC, // ���� ��ü
		CollisionGroup::Default,    // �⺻ �׷�
		CollisionGroup::Ground);    // ����� �浹

	// �ʱ� ��ȯ ��� ����
	m_worldMatrix = XMMatrixIdentity();
	m_viewMatrix = XMMatrixLookAtLH(
		XMVectorSet(0.0f, 5.0f, -5.0f, 1.0f),  // ī�޶� ��ġ
		XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),   // ���� ����
		XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f)    // �� ����
	);
	m_projectionMatrix = XMMatrixPerspectiveFovLH(
		XM_PIDIV4,                              // �þ߰�(45��)
		m_aspectRatio,                          // ȭ�� ����
		0.1f,                                   // �����
		100.0f                                  // �����
	);

	// ȸ�� �ִϸ��̼� �ʱ�ȭ
	m_rotationAngle = 0.0f;
	m_lastTick = GetTickCount64();

	Logger::Instance().Info("Engine �ʱ�ȭ �Ϸ�");
	return true;
}

void Engine::Update()
{
	// ������ �޸� �ʱ�ȭ
	Memory::BeginFrameMemory();

	// ��� ť�� �ִ� �̺�Ʈ ó��
	EventManager::Instance().Update();
	
	// ��Ÿ �ð� ���
	ULONGLONG currentTick = GetTickCount64();
	float deltaTime = (currentTick - m_lastTick) / 1000.0f;
	m_lastTick = currentTick;

	// deltaTime�� 0������ ��� �ּҰ����� ����
	if (deltaTime <= 0.0f) {
		deltaTime = 1.0f / 600.0f;  // �⺻ ������ ����Ʈ
		//Logger::Instance().Warning("0 �Ǵ� ���� DT ����, �⺻ �� ���: {}", deltaTime);
	}

	// ���� ���� ������Ʈ
	m_physicsEngine->Update(deltaTime);

	// ���� ��� ������Ʈ
	UpdateWorldMatrix();

	//// ȸ�� ���� ������Ʈ
	m_rotationAngle += deltaTime;

	// ����Ʈ ���� ������Ʈ (���� �׸��� ȸ��)
	float lightAngle = m_rotationAngle * 0.5f;  // ť�꺸�� õõ�� ȸ��
	m_lightConstants.lightDirection.x = sinf(lightAngle);
	m_lightConstants.lightDirection.z = cosf(lightAngle);
	m_lightConstants.lightDirection.y = -0.5f;  // �ణ ������ ���ߵ���

	// ����ȭ
	XMVECTOR lightDir = XMLoadFloat4(&m_lightConstants.lightDirection);
	lightDir = XMVector3Normalize(lightDir);
	XMStoreFloat4(&m_lightConstants.lightDirection, lightDir);

	// ����Ʈ ��� ���� ������Ʈ
	memcpy(m_lightConstantBufferMappedData, &m_lightConstants, sizeof(m_lightConstants));
}

void Engine::Render()
{
	ThrowIfFailed(m_commandAllocator->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

	// ��� ���� ������Ʈ
	UpdateConstantBuffer();

	// ����Ʈ�� ������Ʈ ����
	const CD3DX12_VIEWPORT viewport(0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height));
	const CD3DX12_RECT scissorRect(0, 0, m_width, m_height);

	m_commandList->RSSetViewports(1, &viewport);
	m_commandList->RSSetScissorRects(1, &scissorRect);

	// ��Ʈ �ñ״�ó�� ��ũ���� �� ����
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	// CBV/SRV �� ����
	ID3D12DescriptorHeap* ppHeaps[] = { m_descHeap.Get() };
	m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// Transform CBV ���� (ù ��° ��ġ)
	m_commandList->SetGraphicsRootDescriptorTable(0, m_descHeap->GetGPUDescriptorHandleForHeapStart());

	// Light CBV ���� (�� ��° ��ġ)
	CD3DX12_GPU_DESCRIPTOR_HANDLE lightCbvHandle(m_descHeap->GetGPUDescriptorHandleForHeapStart());
	lightCbvHandle.Offset(m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	m_commandList->SetGraphicsRootDescriptorTable(1, lightCbvHandle);

	// Texture SRV ���� (�� ��° ��ġ)
	CD3DX12_GPU_DESCRIPTOR_HANDLE textureSrvHandle(m_descHeap->GetGPUDescriptorHandleForHeapStart());
	textureSrvHandle.Offset(2, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	m_commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandle);

	// ���ҽ� �踮��
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_renderTargets[m_frameIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_commandList->ResourceBarrier(1, &barrier);

	// ���� Ÿ�� ����
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
		m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		m_frameIndex, GetRtvDescriptorSize());

	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// ȭ�� Ŭ����
	const float clearColor[] = { 0.0f, 0.0f, 0.2f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	////////// RENDER ///////////
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->IASetIndexBuffer(&m_indexBufferView);
	m_commandList->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);
	/////////////////////////////

	// ���ҽ� �踮��
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_renderTargets[m_frameIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_commandList->ResourceBarrier(1, &barrier);

	ThrowIfFailed(m_commandList->Close());

	// Ŀ�ǵ� ����Ʈ ����
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// ȭ�� ǥ��
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
		// ���� ��ü�� ��ȯ ����� �����ͼ� �������� ����� ���� ��� ������Ʈ
		m_worldMatrix = m_physicsBox->GetTransformMatrix();
	}
}

bool Engine::CreateDevice()
{
	ComPtr<IDXGIFactory7> factory;
	UINT dxgiFactoryFlag = 0;

	// ����� ���� FACTORY_FLAG�� ����� �÷��� �߰�
	IFDEBUG(dxgiFactoryFlag |= DXGI_CREATE_FACTORY_DEBUG;);

	// ���丮 ����
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

		// ����Ʈ���� ����ʹ� ����
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
			continue;
		}

		// ����̽� ����
		if (SUCCEEDED(D3D12CreateDevice(
			hardwareAdapter.Get(), 
			D3D_FEATURE_LEVEL_11_0, 
			IID_PPV_ARGS(&m_device)))) {
			Logger::Instance().Info("����̽� ���� ����");
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

	Logger::Instance().Info("Ŀ�ǵ� ť ���� ����");
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

	Logger::Instance().Info("����ü�� ���� ����");
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

	Logger::Instance().Info("RTV ��ũ���� �� ���� ����");
	return true;
}

bool Engine::CreateRenderTargetViews()
{
	// 1. RTV ���� ���� �ڵ� ���
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

	//[RTV0] [RTV1]
	//	^     ^
	//	|     |
	//	|     +-- �� ��° ��ġ
	//	+ --------ù ��° ��ġ
	// 2. �� ����ۿ� ���� RTV ����
	for (UINT n = 0; n < FRAME_BUFFER_COUNT; ++n) {
		// 2-1. ����ü�����κ��� ���� ���
		if (FAILED(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])))) {
			return false;
		}

		// 2-2. m_renderTargets[n]�� ���� RTV ����
		m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);

		// 2-3. ���� ��ũ���� ��ġ�� �̵�
		rtvHandle.Offset(1, GetRtvDescriptorSize());
	}

	Logger::Instance().Info("RTV ���� ����");
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
	// ���� ���÷� ����
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
	samplerDesc.ShaderRegister = 0; // s0 ��������
	samplerDesc.RegisterSpace = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// ��ũ���� ���̺� ����
	D3D12_DESCRIPTOR_RANGE ranges[3] = {};

	// ��ȯ ��Ŀ� range
	ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	ranges[0].NumDescriptors = 1;
	ranges[0].BaseShaderRegister = 0;	// b0 ��������
	ranges[0].RegisterSpace = 0;
	ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// �����ÿ� range
	ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	ranges[1].NumDescriptors = 1;
	ranges[1].BaseShaderRegister = 1;	// b1 ��������
	ranges[1].RegisterSpace = 0;
	ranges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// �ؽ�ó�� range
	ranges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ranges[2].NumDescriptors = 1;
	ranges[2].BaseShaderRegister = 0;	// t0 ��������
	ranges[2].RegisterSpace = 0;
	ranges[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ��Ʈ �Ķ���� ����
	D3D12_ROOT_PARAMETER rootParameters[3] = {};

	// ��ȯ ��Ŀ� �Ķ����
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[0].DescriptorTable.pDescriptorRanges = &ranges[0];
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// �����ÿ� �Ķ����
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[1].DescriptorTable.pDescriptorRanges = &ranges[1];
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// �ؽ�ó�� �Ķ����
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[2].DescriptorTable.pDescriptorRanges = &ranges[2];
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// ��Ʈ �ñ״�ó ����
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
	// ���̴� ������ �� �ε�
	CompileShaders();

	// ���� �Է� ���̾ƿ� ����
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

	// ���������� ���� ����
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
	// ť���� ���� ������
	Vertex cubeVertices[] = {
		// �ո� (z = 0.5f)
		{ XMFLOAT3(-0.5f, -0.5f, 0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(-0.5f,  0.5f, 0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(0.5f,  0.5f, 0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(0.5f, -0.5f, 0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },

		// �޸� (z = -0.5f)
		{ XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(0.5f,  0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },

		// ���� (y = 0.5f)
		{ XMFLOAT3(-0.5f, 0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(0.5f, 0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(0.5f, 0.5f,  0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-0.5f, 0.5f,  0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },

		// �Ʒ��� (y = -0.5f)
		{ XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(0.5f, -0.5f,  0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },

		// �����ʸ� (x = 0.5f)
		{ XMFLOAT3(0.5f, -0.5f,  0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(0.5f,  0.5f,  0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(0.5f,  0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },

		// ���ʸ� (x = -0.5f)
		{ XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }
	};


	const UINT vertexBufferSize = sizeof(cubeVertices);

	// ���� ����
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

	ThrowIfFailed(m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_vertexBuffer)));

	// ������ ����
	UINT8* pVertexDataBegin;
	CD3DX12_RANGE readRange(0, 0);
	ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, cubeVertices, sizeof(cubeVertices));
	m_vertexBuffer->Unmap(0, nullptr);

	// ���� �� ����
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = vertexBufferSize;

	return true;
}

bool Engine::CreateIndexBuffer()
{
	// ť���� �ε��� ������
	UINT indices[] = {
		// �ո� (0-3)
		0, 1, 2,    // ù ��° �ﰢ��
		0, 2, 3,    // �� ��° �ﰢ��

		// �޸� (4-7)
		4, 5, 6,
		4, 6, 7,

		// ���� (8-11)
		8, 9, 10,
		8, 10, 11,

		// �Ʒ��� (12-15)
		12, 13, 14,
		12, 14, 15,

		// �����ʸ� (16-19)
		16, 17, 18,
		16, 18, 19,

		// ���ʸ� (20-23)
		20, 21, 22,
		20, 22, 23
	};

	m_indexCount = ARRAYSIZE(indices);
	const UINT indexBufferSize = sizeof(indices);

	// �ε��� ���� ����
	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

	ThrowIfFailed(m_device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_indexBuffer)));

	// ������ ����
	UINT8* pIndexDataBegin;
	CD3DX12_RANGE readRange(0, 0);
	ThrowIfFailed(m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
	memcpy(pIndexDataBegin, indices, indexBufferSize);
	m_indexBuffer->Unmap(0, nullptr);

	// �ε��� ���� �� ����
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

	// ���ؽ� ���̴� ������
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

	// �ȼ� ���̴� ������
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
	// ��� ���۴� 256 ����Ʈ ������ �ʿ�
	const UINT constantBufferSize = (sizeof(ObjectConstants) + 255) & ~255;

	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);

	// ��� ���� ����
	if(FAILED(m_device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_constantBuffer)))) {
		return false;
	}

	// ��� ���۸� CPU �޸𸮿� ����
	CD3DX12_RANGE readRange(0, 0);
	if (FAILED(m_constantBuffer->Map(0, &readRange,
		reinterpret_cast<void**>(&m_constantBufferMappedData)))) {
		return false;
	}

	Logger::Instance().Info("��� ���� ���� ����");
	return true;
}

bool Engine::CreateLightConstantBuffer()
{
	// ��� ���۴� 256����Ʈ ������ �ʿ�
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

	// ��� ���۸� CPU �޸𸮿� ����
	CD3DX12_RANGE readRange(0, 0);
	if (FAILED(m_lightConstantBuffer->Map(0, &readRange,
		reinterpret_cast<void**>(&m_lightConstantBufferMappedData)))) {
		return false;
	}

	// �ʱ� ������ �� ����
	m_lightConstants.lightDirection = XMFLOAT4(-0.577f, -0.577f, -0.577f, 0.0f);  // �밢�� ����
	m_lightConstants.lightColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);              // ��� ����
	m_lightConstants.ambientColor = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);            // ȸ�� �ֺ���
	m_lightConstants.eyePosition = XMFLOAT4(0.0f, 0.0f, -5.0f, 0.0f);            // ī�޶� ��ġ

	// �ʱⰪ ����
	memcpy(m_lightConstantBufferMappedData, &m_lightConstants, sizeof(m_lightConstants));

	Logger::Instance().Info("����Ʈ ��� ���� ���� ����");
	return true;
}

bool Engine::CreateTexture(const wchar_t* filename)
{
	// ���ҽ� ���ε� ��ġ ����
	DirectX::ResourceUploadBatch resourceUpload(m_device.Get());
	resourceUpload.Begin();

	// DDS �ؽ�ó �ε�
	if (FAILED(DirectX::CreateDDSTextureFromFile(
		m_device.Get(),
		resourceUpload,
		filename,
		m_texture.ReleaseAndGetAddressOf()))) {
		return false;
	}

	// ���ҽ� ���ε� ����
	auto uploadResourcesFinished = resourceUpload.End(m_commandQueue.Get());
	uploadResourcesFinished.wait();

	return true;
}

bool Engine::CreateDescHeap()
{
	// CBV 2���� SRV 1���� ���� ��ũ���� �� ����
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 3;  // CBV 2�� + SRV 1��
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	ThrowIfFailed(m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_descHeap)));

	// ��ȯ ��Ŀ� CBV ����
	D3D12_CONSTANT_BUFFER_VIEW_DESC transformCbvDesc = {};
	transformCbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
	transformCbvDesc.SizeInBytes = (sizeof(ObjectConstants) + 255) & ~255;

	// �����ÿ� CBV ����
	D3D12_CONSTANT_BUFFER_VIEW_DESC lightCbvDesc = {};
	lightCbvDesc.BufferLocation = m_lightConstantBuffer->GetGPUVirtualAddress();
	lightCbvDesc.SizeInBytes = (sizeof(LightConstants) + 255) & ~255;

	// �ؽ�ó�� SRV ����
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_texture->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = m_texture->GetDesc().MipLevels;

	// ��ũ���� �ڵ� ���
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_descHeap->GetCPUDescriptorHandleForHeapStart());
	UINT handleIncrement = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// CBV��� SRV ����
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
				// �浹 �̺�Ʈ ó��
				Logger::Instance().Debug("'{}'�� '{}'�� �浹 �߻�. ��ġ: ({}, {}, {}), �븻: ({}, {}, {}), ��ݷ�: ({})",
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
				// ���ҽ� �̺�Ʈ ó��
				switch (event.type) {
				case Event::ResourceEvent::Type::Started:
					Logger::Instance().Debug("���ҽ� �ε� ����: {}", event.path);
					break;
				case Event::ResourceEvent::Type::Completed:
					Logger::Instance().Debug("���ҽ� �ε� �Ϸ�: {}", event.path);
					break;
				case Event::ResourceEvent::Type::Failed:
					Logger::Instance().Error("���ҽ� �ε� ����: {}, ����: {}",
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
				// �Է� �̺�Ʈ ó��
				switch (event.type) {
				case Event::InputEvent::Type::KeyDown:
					Logger::Instance().Debug("Ű ����: {}", event.code);
					break;
				case Event::InputEvent::Type::KeyUp:
					Logger::Instance().Debug("Ű ��: {}", event.code);
					break;
				case Event::InputEvent::Type::MouseMove:
					Logger::Instance().Debug("���콺 �̵�: ({}, {})", event.x, event.y);
					break;
				case Event::InputEvent::Type::MouseButtonDown:
					Logger::Instance().Debug("���콺 ��ư ����: {}", event.code);
					break;
				case Event::InputEvent::Type::MouseButtonUp:
					Logger::Instance().Debug("���콺 ��ư ��: {}", event.code);
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
	// 1. ���� �������� Fence ������ Signal
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValues[m_frameIndex]));

	// 2. Fence ���� ���� �������� ���� ������ ������ ����϶�� �̺�Ʈ�� ����
	ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));

	// 3. ������ �̺�Ʈ�� �߻��� ������ ��� (Blocking)
	WaitForSingleObject(m_fenceEvent, INFINITE);
}

void Engine::MoveToNextFrame()
{
	// 1. ���� �������� Fence�� ����
	const UINT64 currentFenceValue = m_fenceValues[m_frameIndex];

	// 2. CommandQueue�� Signal
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), currentFenceValue));

	// 3. ���� ���������� �̵�
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// 4. ���� �������� Fence���� ���� �������� ���� ������ ������ ���
	if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex]) {
		// 4-1. Fence���� ������ ������ ��� (GPU�� ���� ������ �۾��� �Ϸ��� ������ ���)
		ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	// 5. ���� �������� Fence�� ����
	m_fenceValues[m_frameIndex] = currentFenceValue + 1;
}
