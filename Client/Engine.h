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
	// È­¸é Å©±â
	UINT	m_width;
	UINT	m_height;
	float	m_aspectRatio;

	// DirectX 12 °´Ã¼
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

	// ¼ÎÀÌ´õ °ü·Ã ¸â¹ö
	ComPtr<ID3DBlob>					m_vertexShader;
	ComPtr<ID3DBlob>					m_pixelShader;

	// ÃÊ±âÈ­ ÇïÆÛ ÇÔ¼öµé
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
	bool CompileShaders();
	void LogInitializationError(const std::string& step, const std::string& error);

	// ·»´õ¸µ ÇïÆÛ ÇÔ¼öµé
	void WaitForGpu();
	void MoveToNextFrame();
	UINT GetCurrentBackBufferIndex() const	{ return m_frameIndex; }
	UINT GetRtvDescriptorSize() const		{ return m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV); }
};
