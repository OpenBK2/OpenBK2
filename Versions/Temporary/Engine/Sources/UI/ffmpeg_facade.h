#pragma once

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include <chrono>
#include <optional>
#include <vector>


namespace ffmpeg::utils
{
    struct BufferData {
        const uint8_t* ptr;
        size_t size;
        size_t pos;
    };

    // Custom read function for memory-based input
    static int ReadPacket(void* opaque, uint8_t* buf, int buf_size)
    {
        auto* buffer_data = static_cast<BufferData*>(opaque);
        const int remaining = static_cast<int>(buffer_data->size - buffer_data->pos);
        const int to_copy = FFMIN(buf_size, remaining);
        if (to_copy <= 0) {
            return AVERROR_EOF;
        }
        memcpy(buf, buffer_data->ptr + buffer_data->pos, to_copy);
        buffer_data->pos += to_copy;
        return to_copy;
    }

    // Optional seek callback (if you want seeking support)
    static int64_t SeekPacket(void* opaque, int64_t offset, int whence)
    {
        auto* buffer_data = static_cast<BufferData*>(opaque);
        if (whence == AVSEEK_SIZE) {
            return buffer_data->size;
        }

        size_t new_pos;
        switch (whence) {
            case SEEK_SET: new_pos = offset; break;
            case SEEK_CUR: new_pos = buffer_data->pos + offset; break;
            case SEEK_END: new_pos = buffer_data->size + offset; break;
            default: return -1;
        }
        if (new_pos > buffer_data->size) {
            return -1;
        }
        buffer_data->pos = new_pos;
        return static_cast<int64_t>(buffer_data->pos);
    }
}

struct AVIOContextDeleter {
    void operator()(AVIOContext * context) const noexcept {
        if (context) {
            auto buffer = context->buffer;
            avio_context_free(&context);
            if (buffer) {
                av_freep(&buffer);
            }
        }
    }
};

struct AVFormatContextDeleter {
    void operator()(AVFormatContext * context) const noexcept {
        if (context) {
            avformat_close_input(&context);
        }
    }
};

struct AVCodecContextDeleter {
    void operator()(AVCodecContext * context) const noexcept {
        if (context) {
            avcodec_free_context(&context);
        }
    }
};

struct AVFrameDeleter {
    void operator()(AVFrame * frame) const noexcept {
        if (frame) {
            av_frame_free(&frame);
        }
    }
};

struct AVPacketDeleter {
    void operator()(AVPacket * packet) const noexcept {
        if (packet) {
            av_packet_free(&packet);
        }
    }
};

struct SwrContextDeleter {
    void operator()(SwrContext * context) const noexcept {
        if (context) {
            swr_free(&context);
        }
    }
};

struct SwsContextDeleter {
    void operator()(SwsContext * context) const noexcept {
        if (context) {
            sws_freeContext(context);
        }
    }
};

struct MixMixerDeleter {
    void operator()(MIX_Mixer * mixer) const noexcept {
        if (mixer) {
            MIX_DestroyMixer(mixer);
        }
    }
};

struct MixAudioDeleter {
    void operator()(MIX_Audio * audio) const noexcept {
        if (audio) {
            MIX_DestroyAudio(audio);
        }
    }
};

struct MixTrackDeleter {
    void operator()(MIX_Track * track) const noexcept {
        if (track) {
            MIX_DestroyTrack(track);
        }
    }
};

/**
 * Simple struct for storing FFMPEG data and video player data.
 * It does similar functionality like Bink
 */
struct FFmpeg
{
    int FrameRate = 0;
    int Frames = 0;
    int FrameNum = 0;
    int Width = 0;
    int Height = 0;

    using AVCodecContextPtr = std::unique_ptr<AVCodecContext, AVCodecContextDeleter>;
    using AVFormatContextPtr = std::unique_ptr<AVFormatContext, AVFormatContextDeleter>;
    using AVIOContextPtr = std::unique_ptr<AVIOContext, AVIOContextDeleter>;
    using SwrContextPtr = std::unique_ptr<SwrContext, SwrContextDeleter>;
    using SwsContextPtr = std::unique_ptr<SwsContext, SwsContextDeleter>;
    using MixMixerPtr = std::unique_ptr<MIX_Mixer, MixMixerDeleter>;
    using MixAudioPtr = std::unique_ptr<MIX_Audio, MixAudioDeleter>;
    using MixTrackPtr = std::unique_ptr<MIX_Track, MixTrackDeleter>;
    using AVFramePtr = std::unique_ptr<AVFrame, AVFrameDeleter>;
    using AVPacketPtr = std::unique_ptr<AVPacket, AVPacketDeleter>;

    AVIOContextPtr avio_;
    AVFormatContextPtr format_;

    AVCodecContextPtr video_codec_context_;  // Per-instance codec context
    std::optional<int> video_steam_index_;

    // Audio-related members
    std::optional<int> audio_stream_index_;
    AVCodecContextPtr audio_codec_context_;
    SwrContextPtr swr_context_;
    MixMixerPtr mixer_;    // the mixer/device used for playback
    MixAudioPtr audio_;    // loaded audio object (holds PCM)
    MixTrackPtr track_;    // track used to play the audio
    std::vector<uint8_t> audio_buffer_;

    std::chrono::time_point<std::chrono::steady_clock> time_started_;
    std::chrono::time_point<std::chrono::steady_clock> next_frame_time_;
    bool paused_ = false;
    long sound_volume_ = 0;
    AVFramePtr decoded_frame_;
    std::optional<long> last_decoded_index_;
    long goto_index_ = 0;

    ffmpeg::utils::BufferData buffer_data_;  // Store BufferData for cleanup

    ~FFmpeg() = default;

    /**
     * Creates the new FFmpeg video player instance
     * Alternative to BinkOpen
     */
    static std::unique_ptr<FFmpeg> Open(const uint8_t* buffer, size_t size)
    {
        if (!buffer || size == 0)
            return nullptr;

        auto video = std::make_unique<FFmpeg>();

        // Prepare BufferData
        video->buffer_data_.ptr = buffer;
        video->buffer_data_.size = size;
        video->buffer_data_.pos = 0;

        // Allocate AVIO buffer
        constexpr int avio_buffer_size = 64 * 1024;
        auto* avio_buffer = static_cast<unsigned char *>(av_malloc(avio_buffer_size));
        if (!avio_buffer) {
            return nullptr;
        }

        // Create custom AVIOContext
        video->avio_ = AVIOContextPtr(avio_alloc_context(
            avio_buffer,                            // internal buffer
            avio_buffer_size,                       // size
            0,                                      // write flag (0 = read)
            &video->buffer_data_,                   // user data
            &ffmpeg::utils::ReadPacket,             // read callback
            nullptr,                                // write callback
            &ffmpeg::utils::SeekPacket              // seek callback
        ));

        if (!video->avio_) {
            av_freep(&avio_buffer);
            return nullptr;
        }

        // Allocate and attach AVFormatContext
        video->format_ = AVFormatContextPtr(avformat_alloc_context());
        if (!video->format_) {
            return nullptr;
        }

        video->format_->pb = video->avio_.get();

        // Open the input (NULL filename because we're reading from memory)
        AVFormatContext* format = video->format_.release();
        if (avformat_open_input(&format, nullptr, nullptr, nullptr) < 0) {
            return nullptr;
        }
        video->format_.reset(format);

        // Retrieve stream info
        if (avformat_find_stream_info(video->format_.get(), nullptr) < 0) {
            return nullptr;
        }

        // Find the first video stream and set up decoder
        AVStream* video_stream = nullptr;
        for (unsigned int i = 0; i < video->format_->nb_streams; ++i) {
            if (video->format_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                video_stream = video->format_->streams[i];
                video->video_steam_index_ = i;
                break;
            }
        }

        if (video_stream) {
            video->Width = video_stream->codecpar->width;
            video->Height = video_stream->codecpar->height;

            // Get frame rate
            if (const AVRational fps = av_guess_frame_rate(video->format_.get(), video_stream, nullptr); fps.num && fps.den) {
                video->FrameRate = static_cast<int>(av_q2d(fps) + 0.5);
            } else {
                video->FrameRate = 30; // fallback
            }

            // Calculate total frames
            if (video_stream->nb_frames > 0) {
                video->Frames = static_cast<int>(video_stream->nb_frames);
            }
            else if (video->format_->duration != AV_NOPTS_VALUE) {
                const double duration_sec = static_cast<double>(video->format_->duration) / AV_TIME_BASE;
                video->Frames = static_cast<int>(duration_sec * video->FrameRate);
            }
            else {
                video->Frames = 0; // Unknown
            }

            // Initialize decoder
            AVCodecParameters* codecpar = video_stream->codecpar;
            const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
            if (!codec) {
                return nullptr;
            }

            video->video_codec_context_ = std::move(AVCodecContextPtr(avcodec_alloc_context3(codec)));
            if (!video->video_codec_context_) {
                return nullptr;
            }

            if (avcodec_parameters_to_context(video->video_codec_context_.get(), codecpar) < 0) {
                return nullptr;
            }

            if (avcodec_open2(video->video_codec_context_.get(), codec, nullptr) < 0) {
                return nullptr;
            }
        }
        else {
            // No video stream found
            return nullptr;
        }

        // Find and initialize audio stream
        AVStream* audio_stream = nullptr;
        for (unsigned int i = 0; i < video->format_->nb_streams; ++i) {
            if (video->format_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                audio_stream = video->format_->streams[i];
                video->audio_stream_index_ = i;
                break;
            }
        }

        if (audio_stream) {
            const AVCodecParameters* codec_parameters = audio_stream->codecpar;
            if (const AVCodec* codec = avcodec_find_decoder(codec_parameters->codec_id)) {
                video->audio_codec_context_ = AVCodecContextPtr(avcodec_alloc_context3(codec));
                if (video->audio_codec_context_) {
                    if (avcodec_parameters_to_context(video->audio_codec_context_.get(), codec_parameters) >= 0) {
                        if (avcodec_open2(video->audio_codec_context_.get(), codec, nullptr) < 0) {
                            video->audio_codec_context_ = nullptr;
                            video->audio_stream_index_ = std::nullopt;
                        }
                    }
                    else {
                        video->audio_codec_context_ = nullptr;
                        video->audio_stream_index_ = std::nullopt;
                    }
                }
            }
        }

        // Set timing
        video->time_started_ = std::chrono::steady_clock::now();
        video->next_frame_time_ = video->time_started_;

        return video;
    }

    /**
     * Decodes all audio from the video and plays it through SDL_mixer
     */
    void Play()
    {
        if (!format_)
            return;

        if (!audio_stream_index_.has_value() || !audio_codec_context_)
            return;

        // Ensure we have a mixer. Create default playback mixer if necessary.
        if (!mixer_) {
            // device id: choose default playback device
            // second parameter is an SDL_AudioSpec hint; NULL is allowed.
            mixer_ = MixMixerPtr(MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr));
            if (!mixer_) {
                // failed to create mixer
                return;
            }
        }

        // Query mixer output format
        SDL_AudioSpec out_spec;
        if (!MIX_GetMixerFormat(mixer_.get(), &out_spec)) {
            // Can't get mixer format — abort
            return;
        }

        // Determine target AVSampleFormat from mixer format
        AVSampleFormat target_format = AV_SAMPLE_FMT_S16;
        if (out_spec.format == SDL_AUDIO_F32) {
            target_format = AV_SAMPLE_FMT_FLT;
        }
        // Build channel layout for converter (channels in SDL_AudioSpec)
        AVChannelLayout out_ch_layout;
        av_channel_layout_default(&out_ch_layout, out_spec.channels);

        // Set up swr_ctx similar to your original code but using out_spec info
        swr_context_ = SwrContextPtr(swr_alloc());
        if (!swr_context_)
            return;

        av_opt_set_chlayout(swr_context_.get(), "in_chlayout", &audio_codec_context_->ch_layout, 0);
        av_opt_set_chlayout(swr_context_.get(), "out_chlayout", &out_ch_layout, 0);
        av_opt_set_int(swr_context_.get(), "in_sample_rate", audio_codec_context_->sample_rate, 0);
        av_opt_set_int(swr_context_.get(), "out_sample_rate", out_spec.freq, 0);
        av_opt_set_sample_fmt(swr_context_.get(), "in_sample_fmt", audio_codec_context_->sample_fmt, 0);
        av_opt_set_sample_fmt(swr_context_.get(), "out_sample_fmt", target_format, 0);

        if (swr_init(swr_context_.get()) < 0) {
            swr_context_.reset();
            return;
        }

        // Decode all audio into vid->_audio_buffer (same logic as before)
        audio_buffer_.clear();
        auto packet = AVPacketPtr(av_packet_alloc());
        auto frame = AVFramePtr(av_frame_alloc());
        if (!packet || !frame) {
            swr_context_.reset();
            return;
        }

        av_seek_frame(format_.get(), audio_stream_index_.value(), 0, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(audio_codec_context_.get());

        const int bytes_per_sample = av_get_bytes_per_sample(target_format);

        while (av_read_frame(format_.get(), packet.get()) >= 0) {
            if (packet->stream_index == audio_stream_index_.value()) {
                int ret = avcodec_send_packet(audio_codec_context_.get(), packet.get());
                if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                    av_packet_unref(packet.get());
                    continue;
                }

                while (ret >= 0) {
                    ret = avcodec_receive_frame(audio_codec_context_.get(), frame.get());
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                        break;
                    }
                    if (ret < 0) {
                        goto cleanup_decode;
                    }

                    int out_samples = av_rescale_rnd(
                        swr_get_delay(swr_context_.get(), audio_codec_context_->sample_rate) + frame->nb_samples,
                        out_spec.freq,
                        audio_codec_context_->sample_rate,
                        AV_ROUND_UP
                    );

                    const int buffer_size = out_samples * out_spec.channels * bytes_per_sample;
                    const size_t old_size = audio_buffer_.size();
                    audio_buffer_.resize(old_size + buffer_size);
                    uint8_t* out_buf = audio_buffer_.data() + old_size;

                    const int converted = swr_convert(
                        swr_context_.get(),
                        &out_buf,
                        out_samples,
                        frame->data,
                        frame->nb_samples
                    );

                    if (converted > 0) {
                        audio_buffer_.resize(old_size + converted * out_spec.channels * bytes_per_sample);
                    }
                    else {
                        audio_buffer_.resize(old_size);
                    }
                }
            }
            av_packet_unref(packet.get());
        }

    cleanup_decode:
        packet.reset();
        frame.reset();

        // If we decoded audio, create MIX_Audio and play it
        if (!audio_buffer_.empty()) {
            // allocate buffer that we'll hand to mixer (we'll use NoCopy so mixer owns it)
            auto* audio_data = static_cast<uint8_t *>(malloc(audio_buffer_.size()));
            if (!audio_data) {
                // can't allocate -> abort
                swr_context_.reset();
                return;
            }
            memcpy(audio_data, audio_buffer_.data(), audio_buffer_.size());

            // Prepare SDL_AudioSpec structure for MIX_LoadRawAudio* calls
            SDL_AudioSpec spec_for_load;
            spec_for_load.format = out_spec.format;
            spec_for_load.channels = out_spec.channels;
            spec_for_load.freq = out_spec.freq;

            // Use MIX_LoadRawAudioNoCopy if available so mixer will free audio_data when done.
            // Fallback to MIX_LoadRawAudio (which copies) if NoCopy isn't available in your build.
            audio_ = MixAudioPtr(MIX_LoadRawAudioNoCopy(mixer_.get(), audio_data, audio_buffer_.size(), &spec_for_load, true));
            if (!audio_) {
                // fallback: try the copy version and free our buffer after
                audio_ = MixAudioPtr(MIX_LoadRawAudio(mixer_.get(), audio_data, audio_buffer_.size(), &spec_for_load));
                free(audio_data);
            }

            if (audio_) {
                // Create a track, assign audio and play it
                track_ = MixTrackPtr(MIX_CreateTrack(mixer_.get()));
                if (track_) {
                    if (!MIX_SetTrackAudio(track_.get(), audio_.get())) {
                        // failed to set audio on track
                        track_.reset();
                    }
                    else {
                        // Set volume/gain: convert your 0-10000 scale to 0.0-1.0
                        float gain = 1.0f;
                        if (sound_volume_ > 0)
                            gain = static_cast<float>(sound_volume_) / 32768.0f;
                        MIX_SetTrackGain(track_.get(), gain);

                        // Start playback (NULL properties = defaults)
                        MIX_PlayTrack(track_.get(), 0);
                    }
                }
            }
            else {
                // if mixer didn't take ownership, free
                // (MIX_LoadRawAudio allocated copy or returned NULL; if it returns NULL, audio_data must be freed)
                // we already handled free in fallback branches above.
            }
        }

        // Seek back to beginning for video decoding (same as before)
        if (video_steam_index_.has_value()) {
            av_seek_frame(format_.get(), video_steam_index_.value(), 0, AVSEEK_FLAG_BACKWARD);
            avcodec_flush_buffers(video_codec_context_.get());
        }
    }

    void ResetPlaybackTiming() {
        next_frame_time_ = time_started_ = std::chrono::steady_clock::now();
        last_decoded_index_ = std::nullopt;
    }

    /**
     * Jumps the video player to given frame
     */
    void Goto(int target_frame)
    {
        if (!format_ || !video_codec_context_)
            return;

        // Bounds check
        if (target_frame < 0) {
            target_frame = 0;
        }
        if (Frames > 0 && target_frame >= Frames) {
            target_frame = Frames - 1;
        }

        FrameNum = target_frame;
        goto_index_ = target_frame;

        const AVStream* stream = format_->streams[video_steam_index_.value()];
        const double fps = FrameRate > 0 ? FrameRate : 30.0;
        const double seconds = static_cast<double>(target_frame) / fps;
        const auto timestamp = static_cast<int64_t>(seconds / av_q2d(stream->time_base));

        // Seek to keyframe before timestamp
        if (av_seek_frame(format_.get(), video_steam_index_.value(), timestamp, AVSEEK_FLAG_BACKWARD) < 0) {
            return;
        }

        //Flush the decoder
        avcodec_flush_buffers(video_codec_context_.get());

        // Free any previous decoded frame
        decoded_frame_.reset();

        // --- Decode forward until we reach the desired frame ---
        AVPacket* packet = av_packet_alloc();
        AVFrame* frame = av_frame_alloc();
        if (!packet || !frame) {
            ResetPlaybackTiming();
            return;
        }

        int decoded_count = 0;

        while (av_read_frame(format_.get(), packet) >= 0) {
            if (packet->stream_index == video_steam_index_.value()) {
                int ret = avcodec_send_packet(video_codec_context_.get(), packet);
                if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                    break;
                }

                while (ret >= 0) {
                    ret = avcodec_receive_frame(video_codec_context_.get(), frame);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                        break;
                    }
                    if (ret < 0) {
                        ResetPlaybackTiming();
                        return;
                    }

                    if (decoded_count >= target_frame) {
                        // Found target frame
                        decoded_frame_ = AVFramePtr(av_frame_clone(frame));
                        ResetPlaybackTiming();
                        return;
                    }
                    decoded_count++;
                }
            }
            av_packet_unref(packet);
        }
    }

    /**
     * Should the next frame be played?
     * Alternative to BinkWait
     */
    bool Wait() const
    {
        return paused_ || (std::chrono::steady_clock::now() < next_frame_time_);
    }

    /**
     * Pauses the player
     * Alternative to BinkPause
     */
    bool Pause(bool pause)
    {
        const bool old = paused_;

        if (old && !pause) {
            // Resuming from pause - adjust timing
            next_frame_time_ = time_started_ = std::chrono::steady_clock::now();
        }

        paused_ = pause;

        // Pause/resume SDL3_mixer track if present
        if (track_) {
            if (pause) {
                MIX_PauseTrack(track_.get());
            }
            else {
                MIX_ResumeTrack(track_.get());
            }
        }

        return old != pause;
    }

    /**
     * Tells the player to internally decode the frame
     * Alternative to BinkDoFrame
     */
    void DoFrame()
    {
        if (!format_ || !video_codec_context_)
            return;

        if (!video_steam_index_.has_value())
            return;

        // Free previous frame if it exists
        decoded_frame_.reset();

        // Allocate new frame
        auto frame = AVFramePtr(av_frame_alloc());
        if (!frame)
            return;

        const auto packet = AVPacketPtr(av_packet_alloc());
        if (!packet) {
            return;
        }

        // Read packets until we decode the current frame
        bool frame_decoded = false;
        while (!frame_decoded && av_read_frame(format_.get(), packet.get()) >= 0) {
            if (packet->stream_index == video_steam_index_.value()) {
                // Send packet to decoder
                int ret = avcodec_send_packet(video_codec_context_.get(), packet.get());
                if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                    av_packet_unref(packet.get());
                    continue;
                }

                // Receive frame from decoder
                ret = avcodec_receive_frame(video_codec_context_.get(), frame.get());
                if (ret == 0) {
                    // Successfully decoded a frame
                    decoded_frame_ = std::move(frame);
                    frame_decoded = true;

                    last_decoded_index_ = FrameNum;
                }
                else if (ret != AVERROR(EAGAIN)) {
                    // Error or EOF
                    break;
                }
            }

            av_packet_unref(packet.get());
        }
    }

    /**
     * Tells the player to advance to the next frame
     */
    void NextFrame()
    {
        if (!format_ || !video_codec_context_)
            return;

        // Bounds check
        if (Frames > 0 && FrameNum >= Frames - 1) {
            // Already at last frame
            FrameNum++;
            return;
        }

        FrameNum++;

        // Adjust playback scheduling
        next_frame_time_ =
            time_started_ + std::chrono::milliseconds(
                static_cast<int>(((FrameNum - goto_index_ + 1) * 1000.0) / (FrameRate ? FrameRate : 30))
            );

        // Position AVstuff pointers to the new frame here
        if (last_decoded_index_.has_value() && last_decoded_index_ < FrameNum)
        {
            bool fast_forward = false;
            while (last_decoded_index_.value() + 1 < FrameNum)
            {
                DoFrame();
                fast_forward = true;
            }

            if (fast_forward)
            {
                time_started_ = std::chrono::steady_clock::now();
                next_frame_time_ = time_started_;
            }
        }
        // The actual decoding happens in DoFrame()
    }

    /**
     * Copies the (current) decoded frame to the given buffer
     * Alternative to BinkCopyToBuffer
     */
    long CopyToBuffer(void* dest, long stride, long dest_height, long dest_x, long dest_y) const
    {
        if (!decoded_frame_ || !dest) {
            return -1;
        }

        AVFrame* src = decoded_frame_.get();
        const int width = Width;
        const int height = Height;

        if (width <= 0 || height <= 0) {
            return -2;
        }

        // Validate destination bounds
        if (dest_x < 0 || dest_y < 0 || dest_y + height > dest_height) {
            return -6;
        }

        // Create a temporary RGBA frame for conversion
        const auto rgbaFrame = AVFramePtr(av_frame_alloc());
        if (!rgbaFrame) {
            return -3;
        }

        rgbaFrame->format = AV_PIX_FMT_BGRA;
        rgbaFrame->width = width;
        rgbaFrame->height = height;

        // Allocate buffer for converted frame
        if (av_frame_get_buffer(rgbaFrame.get(), 32) < 0) {
            return -4;
        }

        // Create and configure swscale context
        const auto sws = SwsContextPtr(sws_getContext(
            width, dest_height, static_cast<AVPixelFormat>(src->format),
            width, dest_height, AV_PIX_FMT_BGRA,
            SWS_BILINEAR, nullptr, nullptr, nullptr));

        if (!sws) {
            return -5;
        }

        // Perform color space conversion
        const int ret = sws_scale(
            sws.get(),
            src->data,
            src->linesize,
            0,
            height,
            rgbaFrame->data,
            rgbaFrame->linesize
        );

        if (ret < 0) {
            return -7;
        }

        // Now copy to destination with stride and offset
        auto* dstBytes = static_cast<uint8_t*>(dest);

        constexpr int bytes_per_pixel = 4; // RGBA
        const int copy_width_bytes = width * bytes_per_pixel;

        // Offset in destination buffer
        dstBytes += dest_y * stride + dest_x * bytes_per_pixel;

        // Copy line by line
        for (int y = 0; y < height && y + dest_y < dest_height; ++y) {
            memcpy(
                dstBytes + y * stride,
                rgbaFrame->data[0] + y * rgbaFrame->linesize[0],
                copy_width_bytes
            );
        }

        return 0; // OK
    }

    void SetVolume(const long volume)
    {
        sound_volume_ = volume;

        // Update track gain if track exists: SDL3_mixer takes a float gain (1.0 = normal)
        if (track_) {
            float gain = 1.0f;
            if (volume > 0)
                gain = static_cast<float>(volume) / 32768.0f;
            MIX_SetTrackGain(track_.get(), gain);
        }
    }
};
