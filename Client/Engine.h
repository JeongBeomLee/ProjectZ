#pragma once
class Engine {
public:
	Engine();
	~Engine();

	bool Initialize(HWND hwnd, UINT width, UINT height);
	void Update();
	void Render();
	void Cleanup();

private:
	// 화면 크기
	UINT	m_width;
	UINT	m_height;
	float	m_aspectRatio;

	// DirectX 12 객체
	ComPtr<ID3D12Device10>				m_device;
	ComPtr<ID3D12CommandQueue>			m_commandQueue;
	ComPtr<IDXGISwapChain4>				m_swapChain;
	ComPtr<ID3D12DescriptorHeap>		m_rtvHeap;
	ComPtr<ID3D12Resource2>				m_renderTargets[FRAME_BUFFER_COUNT];
	ComPtr<ID3D12CommandAllocator>		m_commandAllocator;
	ComPtr<ID3D12GraphicsCommandList7>	m_commandList;
	ComPtr<ID3D12Fence1>				m_fence;
	ComPtr<ID3D12RootSignature>			m_rootSignature;
	ComPtr<ID3D12PipelineState>			m_pipelineState;
	ComPtr<ID3D12Resource>				m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW			m_vertexBufferView;
	UINT64								m_fenceValues[FRAME_BUFFER_COUNT];
	HANDLE								m_fenceEvent;
	UINT								m_frameIndex;

	// 초기화 헬퍼 함수들
	bool CreateDevice();
	bool CreateCommandQueue();
	bool CreateSwapChain(HWND hwnd);
	bool CreateDescriptorHeaps();
	bool CreateRenderTargetViews();
	bool CreateCommandAllocatorAndList();
	bool CreateFence();
	bool CreateRootSignature();
	bool CreatePipelineState();
	bool CreateVertexBuffer();
	bool CreateSyncObjects();

	// 렌더링 헬퍼 함수들
	void WaitForGpu();
	void MoveToNextFrame();
	UINT GetCurrentBackBufferIndex() const	{ return m_frameIndex; }
	UINT GetRtvDescriptorSize() const		{ return m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV); }
};

