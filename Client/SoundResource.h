#pragma once
#include "Resource.h"
class SoundResource : public Resource {
public:
    explicit SoundResource(const std::string& name)
        : Resource(name)
        , m_duration(0.0f)
        , m_channels(0)
        , m_sampleRate(0)
        , m_isStreaming(false) {}

    bool Load() override;
    void Unload() override;
    ResourceType GetType() const override { return ResourceType::Sound; }
    size_t GetMemoryUsage() const override;

    // ����� �Ӽ� ������
    float GetDuration() const { return m_duration; }
    uint32_t GetChannels() const { return m_channels; }
    uint32_t GetSampleRate() const { return m_sampleRate; }
    bool IsStreaming() const { return m_isStreaming; }

    // ��Ʈ���� ���� �Լ�
    bool LoadChunk(size_t offset, size_t size, void* buffer);
    bool IsChunkLoaded(size_t offset) const;

private:
    std::vector<uint8_t> m_audioData;    // ��ü ����� ������ (non-streaming)
    float m_duration;                     // ����� ����(��)
    uint32_t m_channels;                  // ä�� ��
    uint32_t m_sampleRate;               // ���÷���Ʈ
    bool m_isStreaming;                  // ��Ʈ���� ����

    // ��Ʈ������ ���� �߰� ������
    struct StreamingChunk {
        size_t offset;
        std::vector<uint8_t> data;
        bool loaded;
    };

    static constexpr size_t CHUNK_SIZE = 32768;  // 32KB
    mutable std::shared_mutex m_streamMutex;
    std::unordered_map<size_t, StreamingChunk> m_streamingChunks;
};