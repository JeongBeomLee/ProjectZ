#pragma once
class PhysicsObject;
class PhysicsEngine;
class Engine {
public:
	Engine();
	~Engine();

	static Engine& GetInstance();
	bool Initialize(HWND hwnd, UINT width, UINT height);
	void Update();
	void Render();
	void Cleanup();

	ID3D12Device10* GetDevice() const { return m_device.Get(); }
	ID3D12CommandQueue* GetCommandQueue() const { return m_commandQueue.Get(); }

private:
	// ȭ�� ũ��
	UINT	m_width;
	UINT	m_height;
	float	m_aspectRatio;

	// DirectX 12 ��ü
	ComPtr<ID3D12Device10> m_device;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<IDXGISwapChain4> m_swapChain;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12DescriptorHeap> m_descHeap;  // �ϳ��� ������ ����
	ComPtr<ID3D12Resource2> m_renderTargets[FRAME_BUFFER_COUNT];
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12GraphicsCommandList7> m_commandList;
	ComPtr<ID3D12Fence1> m_fence;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;

	UINT64 m_fenceValues[FRAME_BUFFER_COUNT];
	HANDLE m_fenceEvent;
	UINT m_frameIndex;

	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
	UINT m_indexCount;

	// ���̴� ���� ���
	ComPtr<ID3DBlob> m_vertexShader;
	ComPtr<ID3DBlob> m_pixelShader;

	// ��� ����
	ComPtr<ID3D12Resource> m_constantBuffer;
	UINT8* m_constantBufferMappedData;

	// ������ ����
	ComPtr<ID3D12Resource> m_lightConstantBuffer;
	UINT8* m_lightConstantBufferMappedData;
	LightConstants m_lightConstants;

	// �ؽ�ó ���� ���
	ComPtr<ID3D12Resource> m_texture;
	
	// ��ȯ ��� (ī�޶�)
	XMMATRIX m_worldMatrix;
	XMMATRIX m_viewMatrix;
	XMMATRIX m_projectionMatrix;

	// for animation
	float m_rotationAngle;
	ULONGLONG m_lastTick;

	// ���� ����
	std::unique_ptr<PhysicsEngine> m_physicsEngine;

	// ���� ��ü
	std::shared_ptr<PhysicsObject> m_physicsBox;
	std::shared_ptr<PhysicsObject> m_ground;

	// ���� ��� ������Ʈ �Լ�
	void UpdateWorldMatrix();

	// �ʱ�ȭ ���� �Լ���
	bool CreateDevice();
	bool CreateCommandQueue();
	bool CreateSwapChain(HWND hwnd);
	bool CreateRTVDescriptorHeaps();
	bool CreateRenderTargetViews();
	bool CreateCommandAllocatorAndList();
	bool CreateFence();
	bool CreateRootSignature();
	bool CreatePipelineState();
	bool CreateVertexBuffer();
	bool CreateIndexBuffer();
	bool CompileShaders();
	bool CreateConstantBuffer();
	bool CreateLightConstantBuffer();

	bool CreateTexture(const wchar_t* filename);
	bool CreateDescHeap();

	void UpdateConstantBuffer();
	void LogInitializationError(const std::string& step, const std::string& error);

	// ������ ���� �Լ���
	void WaitForGpu();
	void MoveToNextFrame();
	UINT GetCurrentBackBufferIndex() const	{ return m_frameIndex; }
	UINT GetRtvDescriptorSize() const		{ return m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV); }
};
