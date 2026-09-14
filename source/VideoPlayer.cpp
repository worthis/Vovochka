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
    if (w <= 0 || h <= 0)
    {
        plm_destroy(m_plm);
        m_plm = nullptr;
        return false;
    }

    m_stride = w * 3;
    m_rgb.assign((size_t)w * h * 3, 0);
    m_nextRgb.assign((size_t)w * h * 3, 0);
    m_nextTime = -1.0f;
    m_videoEnded = false;

    Image img{};
    img.width = w;
    img.height = h;
    img.mipmaps = 1;
    img.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8;
    img.data = m_rgb.data();
    m_tex = LoadTextureFromImage(img);
    SetTextureFilter(m_tex, TEXTURE_FILTER_BILINEAR);

    m_hasAudio = plm_get_num_audio_streams(m_plm) > 0;
    m_audioEnded = false;
    m_stage.clear();
    m_stageFrames = 0;
    if (m_hasAudio)
    {
        m_sr = plm_get_samplerate(m_plm);
        if (m_sr <= 0)
            m_sr = 44100;

        SetAudioStreamBufferSizeDefault(kAudioBufferFrames);
        m_stream = LoadAudioStream(m_sr, 32, 2);

        for (int i = 0; i < 2; ++i)
        {
            topUpStage();
            if (m_stageFrames == 0)
                break;
            writeChunk();
        }
    }

    m_playStart = GetTime();
    if (m_hasAudio)
        PlayAudioStream(m_stream);

    m_capSec = capSec;
    m_finished = false;

    TraceLog(LOG_INFO, "Video: %s (dur %.1fs, cap %.1fs, %dx%d @ %.2f fps)",
             mpgPath.c_str(), plm_get_duration(m_plm), capSec, w, h, m_framerate);

    return true;
}

void VideoPlayer::topUpStage()
{
    while (m_stageFrames < kAudioBufferFrames && !m_audioEnded)
    {
        plm_samples_t *s = plm_decode_audio(m_plm);
        if (!s)
        {
            m_audioEnded = true;
            break;
        }
        const int n = PLM_AUDIO_SAMPLES_PER_FRAME * 2; // stereo
        m_stage.insert(m_stage.end(), s->interleaved, s->interleaved + n);
        m_stageFrames += PLM_AUDIO_SAMPLES_PER_FRAME;
    }
}

void VideoPlayer::writeChunk()
{
    if (m_stageFrames < kAudioBufferFrames)
    {
        m_stage.resize((size_t)kAudioBufferFrames * 2, 0.0f); // тишина в хвосте
        m_stageFrames = kAudioBufferFrames;
    }
    UpdateAudioStream(m_stream, m_stage.data(), kAudioBufferFrames);
    m_stage.erase(m_stage.begin(), m_stage.begin() + (size_t)kAudioBufferFrames * 2);
    m_stageFrames -= kAudioBufferFrames;
}

void VideoPlayer::pumpAudio()
{
    while (IsAudioStreamProcessed(m_stream))
    {
        topUpStage();
        if (m_stageFrames == 0)
            break;
        writeChunk();
    }
}

void VideoPlayer::update(float dt)
{
    if (!m_plm || m_finished)
        return;

    const float t = timePlayed();
    if (m_capSec > 0.0f && t >= m_capSec)
    {
        m_finished = true;
        return;
    }

    if (m_hasAudio)
        pumpAudio();

    if (m_nextTime >= 0.0f && m_nextTime <= t)
    {
        UpdateTexture(m_tex, m_nextRgb.data());
        m_nextTime = -1.0f;
    }

    while (m_nextTime < 0.0f && !m_videoEnded)
    {
        plm_frame_t *fr = plm_decode_video(m_plm);
        if (!fr)
        {
            m_videoEnded = true;
            break;
        }
        plm_frame_to_rgb(fr, m_nextRgb.data(), m_stride);
        m_nextTime = (float)fr->time;
        if (m_nextTime <= t)
        {
            UpdateTexture(m_tex, m_nextRgb.data());
            m_nextTime = -1.0f;
        }
    }

    const bool audioEnd = !m_hasAudio || m_audioEnded;
    if (m_videoEnded && m_nextTime < 0.0f && audioEnd)
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
    if (!m_plm)
        return 0.0f;
    const double t = GetTime() - m_playStart;
    return (float)(t < 0.0 ? 0.0 : t);
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
    m_nextTime = -1.0f;
    m_videoEnded = false;
    m_rgb.clear();
    m_nextRgb.clear();
    m_stage.clear();
    m_stageFrames = 0;
    m_hasAudio = false;
    m_finished = false;
}