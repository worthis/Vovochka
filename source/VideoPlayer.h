#pragma once

#include "raylib.h"
#include <string>
#include <vector>

struct plm_t; // forward из pl_mpeg.h

class VideoPlayer
{
public:
    ~VideoPlayer();

    bool open(const std::string &mpgPath, float capSec); // capSec = доступное время, 0 = без лимита
    void update(float dt);
    void draw(Rectangle dst) const;
    void close();

    bool isOpen() const { return m_plm != nullptr; }
    bool finished() const { return m_finished; }
    float timePlayed() const;
    float capSec() const { return m_capSec; }

private:
    static constexpr int kAudioBufferFrames = 4096 * 3;

    plm_t *m_plm = nullptr;

    Texture2D m_tex{};
    std::vector<unsigned char> m_rgb;
    std::vector<unsigned char> m_nextRgb;
    float m_nextTime = -1.0f;
    bool m_videoEnded = false;
    double m_framerate = 25.0;
    int m_stride = 0;

    AudioStream m_stream{};
    bool m_hasAudio = false;
    bool m_audioEnded = false;
    int m_sr = 44100;
    double m_playStart = 0.0;

    std::vector<float> m_stage;
    int m_stageFrames = 0;

    bool m_finished = false;
    float m_capSec = 0.0f;

    void topUpStage();
    void writeChunk();
    void pumpAudio();
};