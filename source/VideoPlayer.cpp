#include "VideoPlayer.h"

#define PL_MPEG_IMPLEMENTATION
#include "third_party/pl_mpeg.h"

#include <cstring>

VideoPlayer::~VideoPlayer()
{
    close();
}

bool VideoPlayer::open(const std::string &mpgPath, float capSec)
{
    close();

    m_plm = plm_create_with_filename(mpgPath.c_str());
    if (!m_plm)
    {
        TraceLog(LOG_WARNING, "Video: cannot open %s", mpgPath.c_str());
        return false;
    }

    plm_set_loop(m_plm, FALSE);
    plm_set_audio_enabled(m_plm, TRUE);

    m_framerate = plm_get_framerate(m_plm);
    if (m_framerate <= 0.0)
        m_framerate = 25.0;

    const int w = plm_get_width(m_plm);
    const int h = plm_get_height(m_plm);
    m_stride = w * 3;
    m_rgb.assign((size_t)w * h * 3, 0);

    // Текстура поверх нашего буфера: пример делает то же самое
    Image img{};
    img.width = w;
    img.height = h;
    img.mipmaps = 1;
    img.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8;
    img.data = m_rgb.data();
    m_tex = LoadTextureFromImage(img);
    SetTextureFilter(m_tex, TEXTURE_FILTER_BILINEAR);

    m_hasAudio = plm_get_num_audio_streams(m_plm) > 0;
    if (m_hasAudio)
    {
        const int sr = plm_get_samplerate(m_plm);
        m_stream = LoadAudioStream(sr, 32, 2); // float32 stereo, как в примере
        PlayAudioStream(m_stream);
        plm_set_audio_lead_time(m_plm, (double)PLM_AUDIO_SAMPLES_PER_FRAME / (double)sr);
    }

    m_capSec = capSec;
    m_finished = false;
    m_videoAccum = 0.0;

    TraceLog(LOG_INFO, "Video: %s (dur %.1fs, cap %.1fs, %dx%d @ %.2f fps)",
             mpgPath.c_str(), plm_get_duration(m_plm), capSec, w, h, m_framerate);
    return true;
}

void VideoPlayer::update(float dt)
{
    if (!m_plm || m_finished)
        return;

    if (m_capSec > 0.0f && plm_get_time(m_plm) >= (double)m_capSec)
    {
        m_finished = true;
        return;
    }

    // Видео: кадр каждые 1/framerate, догоняем не более 3 кадров за тик
    m_videoAccum += dt;
    const double frameDur = 1.0 / m_framerate;
    bool dirty = false;
    int guard = 0;
    while (m_videoAccum >= frameDur && guard < 3)
    {
        m_videoAccum -= frameDur;
        plm_frame_t *fr = plm_decode_video(m_plm);
        if (fr)
        {
            plm_frame_to_rgb(fr, m_rgb.data(), m_stride);
            dirty = true;
        }
        ++guard;
    }
    if (m_videoAccum > frameDur * 3.0) // сброс долга после фризов
        m_videoAccum = 0.0;
    if (dirty)
        UpdateTexture(m_tex, m_rgb.data());

    // Аудио: дозаправка буфера raudio, как в примере
    if (m_hasAudio)
    {
        while (IsAudioStreamProcessed(m_stream))
        {
            plm_samples_t *s = plm_decode_audio(m_plm);
            if (!s)
                break;
            UpdateAudioStream(m_stream, s->interleaved, PLM_AUDIO_SAMPLES_PER_FRAME * 2);
            //UpdateAudioStream(m_stream, s->interleaved, PLM_AUDIO_SAMPLES_PER_FRAME);
        }
    }

    if (plm_has_ended(m_plm))
        m_finished = true;
}

void VideoPlayer::draw(Rectangle dst) const
{
    if (!m_plm || m_tex.id == 0)
        return;
    Rectangle src{0.0f, 0.0f, (float)m_tex.width, (float)m_tex.height};
    DrawTexturePro(m_tex, src, dst, {0, 0}, 0.0f, WHITE);
}

float VideoPlayer::timePlayed() const
{
    return m_plm ? (float)plm_get_time(m_plm) : 0.0f;
}

void VideoPlayer::close()
{
    if (m_hasAudio && m_stream.buffer != nullptr)
    {
        StopAudioStream(m_stream);
        UnloadAudioStream(m_stream);
        m_stream = {};
    }
    if (m_tex.id != 0)
    {
        UnloadTexture(m_tex);
        m_tex = {};
    }
    if (m_plm)
    {
        plm_destroy(m_plm);
        m_plm = nullptr;
    }
    m_rgb.clear();
    m_hasAudio = false;
    m_finished = false;
}