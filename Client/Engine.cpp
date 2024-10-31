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

	// ����� ���̾� Ȱ��ȭ
	IFDEBUG(
		ComPtr<ID3D12Debug6> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
			debugController->EnableDebugLayer();
		}
	);

	// DirectX 12 ��ü ����
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

	return true;
}

void Engine::Update()
{
	// TODO: ������Ʈ ���� ����
}

void Engine::Render()
{
	// 1. Ŀ�ǵ� �Ҵ��ڿ� ����Ʈ ����
	ThrowIfFailed(m_commandAllocator->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

	// 2. ���ҽ� �踮�� ���� (Present �� Render Target)
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_renderTargets[m_frameIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_commandList->ResourceBarrier(1, &barrier);

	// 3. ���� Ÿ�� ���� �� Ŭ����
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
		m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		m_frameIndex, GetRtvDescriptorSize());

	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// 4. ���ҽ� �踮�� ���� (Render Target �� Present)
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_renderTargets[m_frameIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);
	m_commandList->ResourceBarrier(1, &barrier);

	// 5. Ŀ�ǵ� ����Ʈ ����
	ThrowIfFailed(m_commandList->Close());

	// 6. Ŀ�ǵ� ����Ʈ ����
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// 7. ȭ�� ǥ��
	ThrowIfFailed(m_swapChain->Present(1, 0));

	// 8. CPU�� GPU ����ȭ
	WaitForGpu();

	// 9. ����� �ε��� ����
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

	// ����� ���� FACTORY_FLAG�� ����� �÷��� �߰�
	IFDEBUG(dxgiFactoryFlag |= DXGI_CREATE_FACTORY_DEBUG;);

	// ���丮 ����
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlag, IID_PPV_ARGS(&factory)));

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
		ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));

		// 2-2. m_renderTargets[n]�� ���� RTV ����
		m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);

		// 2-3. ���� ��ũ���� ��ġ�� �̵�
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
	return false;
}

bool Engine::CreatePipelineState()
{
	return false;
}

bool Engine::CreateVertexBuffer()
{
	return false;
}

bool Engine::CreateSyncObjects()
{
	return false;
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
