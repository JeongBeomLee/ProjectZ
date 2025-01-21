#pragma once
#include "Event.h"
#include "EventTypes.h"

class PhysicsObject;
class PhysicsEngine;
class GameObject;
class Scene;
class Camera;
class ShaderResource;
class Engine {
public:
	Engine();
	~Engine();

	static Engine& Instance();
	bool Initialize(HWND hwnd, UINT width, UINT height);
	void Update();
	void Render();
	void BeginRender();
	void ExecuteRender();
	void EndRender();
	void Cleanup();

	ID3D12Device10* GetDevice() const { return m_device.Get(); }
	ID3D12CommandQueue* GetCommandQueue() const { return m_commandQueue.Get(); }
	PhysicsEngine* GetPhysicsEngine() const { return m_physicsEngine.get(); }

	Camera* GetMainCamera() const { return m_mainCamera; }
	XMMATRIX GetViewMatrix() const;
	XMMATRIX GetProjectionMatrix() const;

	ID3D12DescriptorHeap* GetDescHeap() const { return m_descHeap.Get(); }
	UINT GetCbvDescriptorIndex() { return m_currentCbvIndex++; }
	UINT GetSrvDescriptorIndex() { return MAX_OBJECTS + 1 + m_currentSrvIndex++; }
	UINT GetDescriptorIncrementSize() const { return m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); }

	// 디스크립터 접근 메서드
	UINT GetTransformDescriptorOffset(UINT index) const { return DESCRIPTOR_TRANSFORM_CBV_START + index; }
	UINT GetLightDescriptorOffset() const { return DESCRIPTOR_LIGHT_CBV; }
	UINT GetMaterialDescriptorOffset(UINT index) const { return DESCRIPTOR_MATERIAL_START + index * 2; }  // Material CBV와 TextureFlags CBV가 연속으로 배치
	UINT GetTextureDescriptorOffset(UINT materialIndex) const { return DESCRIPTOR_TEXTURE_START + materialIndex * 5; }  // 5개의 텍스처가 연속으로 배치

private:
	// 화면 크기
	UINT	m_width;
	UINT	m_height;
	float	m_aspectRatio;

	// DirectX 12 객체
	ComPtr<ID3D12Device10> m_device;

	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12GraphicsCommandList7> m_commandList;

	ComPtr<IDXGISwapChain4> m_swapChain;
	ComPtr<ID3D12Resource2> m_renderTargets[FRAME_BUFFER_COUNT];
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;

	ComPtr<ID3D12DescriptorHeap> m_descHeap;

	ComPtr<ID3D12Fence1> m_fence;

	ComPtr<ID3D12Resource> m_depthStencilBuffer;
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;

	UINT64 m_fenceValues[FRAME_BUFFER_COUNT];
	HANDLE m_fenceEvent;
	UINT m_frameIndex;

	// 셰이더 관련 멤버
	//ComPtr<ID3DBlob> m_vertexShader;
	//ComPtr<ID3DBlob> m_pixelShader;

	// 디스크립터 힙 관리
	static constexpr UINT MAX_OBJECTS = 100;  // 최대 오브젝트 수
	static constexpr UINT MAX_MATERIALS = 100;

	// 디스크립터 레이아웃 관련 상수
	// Transform CBV 시작 위치 (0 ~ 99)
	static constexpr UINT DESCRIPTOR_TRANSFORM_CBV_START = 0;
	// Light CBV 위치 (100)
	static constexpr UINT DESCRIPTOR_LIGHT_CBV = MAX_OBJECTS;
	// Material CBV, TextureFlags CBV 시작 위치 (101 ~ 300)
	static constexpr UINT DESCRIPTOR_MATERIAL_START = MAX_OBJECTS + 1;
	// 텍스처 SRV 시작 위치 (301 ~)
	static constexpr UINT DESCRIPTOR_TEXTURE_START = MAX_OBJECTS + MAX_MATERIALS * 2 + 1;
	// CBV들 + 텍스처(albedo, normal, metallic-roughness, emissive, occlusion)
	static constexpr UINT TOTAL_DESCRIPTOR_COUNT = DESCRIPTOR_TEXTURE_START + MAX_MATERIALS * 5;

	UINT m_currentCbvIndex = 0;  // CBV 할당을 위한 인덱스
	UINT m_currentSrvIndex = 0;  // SRV 할당을 위한 인덱스

	// 라이팅 관련
	ComPtr<ID3D12Resource> m_lightConstantBuffer;
	UINT8* m_lightConstantBufferMappedData;
	LightConstants m_lightConstants;
	float m_rotationAngle = 0.0f;

	// 변환 행렬 (카메라)
	Camera* m_mainCamera = nullptr;

	// 물리 엔진
	std::unique_ptr<PhysicsEngine> m_physicsEngine;

	// 이벤트 핸들러 ID 저장용 변수들
	std::vector<Event::EventDispatcher<Event::CollisionEvent>::HandlerId> m_collisionHandlerIds;
	std::vector<Event::EventDispatcher<Event::ResourceEvent>::HandlerId> m_resourceHandlerIds;
	std::vector<Event::EventDispatcher<Event::InputEvent>::HandlerId> m_inputHandlerIds;

	// 초기화 헬퍼 함수들
	bool CreateDevice();
	bool CreateCommandQueue();
	bool CreateSwapChain(HWND hwnd);
	bool CreateRTVDescriptorHeaps();
	bool CreateRenderTargetViews();
	bool CreateCommandAllocatorAndList();
	bool CreateFence();
	bool CreateDepthStencilBuffer();
	bool CreateRootSignature();
	bool CreatePipelineState(ShaderResource* vertexShader, ShaderResource* pixelShader);
	//bool CompileShaders();
	bool CreateLightConstantBuffer();
	bool CreateDescHeap();

	void UpdateLightConstant(float deltaTime);

	void CreateCubeMeshData(std::vector<Vertex>& vertices, std::vector<UINT>& indices);
	void CreateSphereMeshData(std::vector<Vertex>& vertices, std::vector<UINT>& indices,
		float radius, int slices = 15, int stacks = 15);
	void CreateCapsuleMeshData(std::vector<Vertex>& vertices, std::vector<UINT>& indices,
		float radius, float height, int slices = 15, int stacks = 15);

	void CreateCube(const PxVec3& position, const PxVec3& dimensions = PxVec3(0.5f));
	void CreateSphere(const PxVec3& position, float radius = 0.5f);
	void CreateCapsule(const PxVec3& position, float radius = 0.3f, float height = 1.0f);

	void CreateDemonstrationObjects(Scene* scene, const PxVec3& position);
	void CreateDefaultScene();

	// 이벤트 핸들러 등록, 등록 해제 함수
	void RegisterEventHandlers();
	void UnregisterEventHandlers();

	// 렌더링 헬퍼 함수들
	void WaitForGpu();
	void MoveToNextFrame();
	UINT GetCurrentBackBufferIndex() const	{ return m_frameIndex; }
	UINT GetRtvDescriptorSize() const		{ return m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV); }
};
