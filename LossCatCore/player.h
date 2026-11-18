#pragma once
#include <string>
#include "miniaudio.h"
#include <cstdio>
#include <atomic>
#include <filesystem>

enum class State { NoTrack, Playing, Paused, Finished };

class Player
{
private:
	State state = State::NoTrack;
	std::atomic<bool> done{ false };
	std::string filePath;
	bool isLoaded = false;
	bool isInit = false;
	ma_decoder decoder;
	ma_device device;
	ma_uint32 sampleRate = 0;
	ma_uint32 channels = 0;
	ma_uint64 totalFrames = 0;
	std::atomic<ma_uint64> framesPlayed{ 0 };
	static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
	{
		(void)pInput;
		Player* player = static_cast<Player*>(pDevice->pUserData);
		ma_uint64 framesRead = 0;
		if (player == nullptr)
		{
			return;
		}
		ma_decoder_read_pcm_frames(&player->decoder, pOutput, frameCount, &framesRead);
		player->framesPlayed += framesRead;
		if (framesRead < frameCount)
		{
			player->done = true;
			return;
		}
	}
public:
	Player(const Player&) = delete;
	Player& operator=(const Player&) = delete;
	Player() {
		printf("Constructor Called \n");
	}
	~Player() {
		printf("Deconstructor Called \n");
		uninit();
	}
	State getState() const
	{
		if (done && isLoaded)
		{
			return State::Finished;
		}

		return state;
	}
	bool seek(double seconds)
	{
		if (!isLoaded)
		{
			return false;
		}
		bool wasPlaying = getState() == State::Playing;
		if (seconds < 0)
		{
			seconds = 0;
		}
		ma_uint64 frame = static_cast<ma_uint64>(seconds * sampleRate);
		if (totalFrames > 0 && frame > totalFrames)
		{
			frame = totalFrames;
		}
		ma_device_stop(&device); //callback must not be reading decoder while we move it
		if (ma_decoder_seek_to_pcm_frame(&decoder, frame) != MA_SUCCESS)
		{
			if (wasPlaying)
			{
				ma_device_start(&device);
			}
			return false;
		}
		framesPlayed = frame;
		done = false;
		if (wasPlaying)
		{
			ma_device_start(&device);
			state = State::Playing;
		}
		else
		{
			state = State::Paused;
		}
		return true;
	}
	double duration() const
	{
		if (sampleRate == 0)
		{
			return 0.0;
		}
		return static_cast<double>(totalFrames) / sampleRate;
	}
	double position() const
	{
		if (sampleRate == 0)
		{
			return 0.0;
		}
		return static_cast<double>(framesPlayed.load()) / sampleRate;
	}
	float volume()
	{
		float v = 0.0f;
		if (isInit)
		{
			ma_device_get_master_volume(&device, &v);
		}
		return v;
	}
	bool isFinished() const
	{
		return getState() == State::Finished;
	}
	bool init(ma_uint32 wantChannels, ma_uint32 wantSampleRate)
	{
		if (isInit)
		{
			return true;
		}
		ma_device_config config = ma_device_config_init(ma_device_type_playback);
		config.playback.format = ma_format_f32; // must match the decoder format in load()
		config.playback.channels = wantChannels; // Set to 0 to use the device's native channel count.
		config.sampleRate = wantSampleRate; // Set to 0 to use the device's native sample rate.
		config.dataCallback = data_callback; // This function will be called when miniaudio needs more data.
		config.pUserData = this; // Can be accessed from the device object (device.pUserData).	
		if (ma_device_init(NULL, &config, &device) != MA_SUCCESS)
		{
			printf("failed to open audio device\n");
			isInit = false;
			return false;
		}
		this->channels = device.playback.channels;
		this->sampleRate = device.sampleRate;
		isInit = true;
		return true;
	}
	void uninit()
	{
		if (isLoaded)
		{
			ma_device_stop(&device);
			ma_decoder_uninit(&decoder);
			state = State::NoTrack;
			totalFrames = 0;
			framesPlayed = 0;
			filePath.clear();
			isLoaded = false;
		}
		if (isInit)
		{
			ma_device_uninit(&device);
			isInit = false;
		}
	}
	bool load(const std::string& path) //load always leaves the player paused
	{
		if (!isInit)
		{
			return false;
		}
		if (isLoaded)
		{
			ma_device_stop(&device);
			ma_decoder_uninit(&decoder);
			isLoaded = false;
		}
		ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, channels, sampleRate);
#ifdef _WIN32
		std::filesystem::path p(std::u8string(path.begin(), path.end()));
		ma_result opened = ma_decoder_init_file_w(p.c_str(), &decoderConfig, &decoder);
#else
		ma_result opened = ma_decoder_init_file(path.c_str(), &decoderConfig, &decoder);
#endif
		if (opened != MA_SUCCESS)
		{
			printf("file open error \n");
			state = State::NoTrack;
			totalFrames = 0;
			framesPlayed = 0;
			filePath.clear();
			return false;
		}
		if (ma_decoder_get_length_in_pcm_frames(&decoder, &totalFrames) != MA_SUCCESS)
		{
			totalFrames = 0;
		}

		done = false;
		framesPlayed = 0;
		filePath = path;
		isLoaded = true;
		state = State::Paused;
		return true;
	}
	bool play()
	{
		if (!isLoaded)
		{
			return false;
		}
		if (getState() == State::Playing)
		{
			return true;
		}
		if (getState() == State::Finished)
		{
			ma_device_stop(&device);
			ma_result r = ma_decoder_seek_to_pcm_frame(&decoder, 0);
			if (r != MA_SUCCESS)
			{
				printf("seek error: %s\n", ma_result_description(r));
				state = State::Paused;
				return false;
			}
			framesPlayed = 0;
			done = false;
		}
		if (ma_device_start(&device) != MA_SUCCESS)
		{
			state = State::Paused;
			return false;
		}
		state = State::Playing;
		return true;
	}
	void pause()
	{
		if (!isLoaded)
		{
			return;
		}
		ma_device_stop(&device);
		state = State::Paused;

	}
	void stop()
	{
		pause();
		seek(0);
	}
	bool setVolume(float v)
	{
		if (!isInit)
		{
			return false;
		}
		if (v < 0.0f) v = 0.0f;
		if (v > 1.0f) v = 1.0f;
		return ma_device_set_master_volume(&device, v) == MA_SUCCESS;
	}
	const std::string& currentPath() const
	{
		return filePath;
	}

	const char* deviceName() const { return isInit ? device.playback.name : ""; }
	const char* backendName() const { return isInit ? ma_get_backend_name(device.pContext->backend) : ""; }
	ma_uint32 deviceSampleRate() const { return isInit ? device.playback.internalSampleRate : 0; }
	ma_uint32 deviceChannels() const { return isInit ? device.playback.internalChannels : 0; }
	const char* deviceFormatName() const { return isInit ? ma_get_format_name(device.playback.internalFormat) : ""; }
	ma_uint32 outputSampleRate() const { return sampleRate; }
};