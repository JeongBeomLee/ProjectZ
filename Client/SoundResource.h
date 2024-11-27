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

    // 오디오 속성 접근자
    float GetDuration() const { return m_duration; }
    uint32_t GetChannels() const { return m_channels; }
    uint32_t GetSampleRate() const { return m_sampleRate; }
    bool IsStreaming() const { return m_isStreaming; }

    // 스트리밍 관련 함수
    bool LoadChunk(size_t offset, size_t size, void* buffer);
    bool IsChunkLoaded(size_t offset) const;

private:
    std::vector<uint8_t> m_audioData;    // 전체 오디오 데이터 (non-streaming)
    float m_duration;                     // 오디오 길이(초)
    uint32_t m_channels;                  // 채널 수
    uint32_t m_sampleRate;               // 샘플레이트
    bool m_isStreaming;                  // 스트리밍 여부

    // 스트리밍을 위한 추가 데이터
    struct StreamingChunk {
        size_t offset;
        std::vector<uint8_t> data;
        bool loaded;
    };

    static constexpr size_t CHUNK_SIZE = 32768;  // 32KB
    mutable std::shared_mutex m_streamMutex;
    std::unordered_map<size_t, StreamingChunk> m_streamingChunks;
};