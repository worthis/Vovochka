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
    plm_t *m_plm = nullptr;
    Texture2D m_tex{};
    std::vector<unsigned char> m_rgb;
    AudioStream m_stream{};
    bool m_hasAudio = false;
    bool m_finished = false;
    float m_capSec = 0.0f;
    double m_framerate = 25.0;
    double m_videoAccum = 0.0;
    int m_stride = 0;
};