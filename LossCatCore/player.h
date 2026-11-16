#pragma once
#include <string>
#include "miniaudio.h"
#include <cstdio>
#include <atomic>
class Player
{
private:
	std::atomic<bool> done{ false };
	std::string filePath;
	bool isLoaded = false;
	bool isInit = false;
	ma_decoder decoder;
	ma_device device;
	ma_uint32 sampleRate = 0;
	ma_uint32 channels = 0;
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
		if (framesRead < frameCount)
		{
			player->done = true;
			return;
		}
	}
public:
	Player(const Player&) = delete;
	Player& operator=(const Player&) = delete;
	Player()	{
		printf("Constructor Called \n");
	}
	~Player()	{
		printf("Deconstructor Called \n");
		uninit();
	}
	bool isFinished() const { return done; }
	bool init(ma_uint32 channels, ma_uint32 sampleRate)
	{
		if (isInit)
		{
			return true;
		}
		this->channels = channels;
		this->sampleRate = sampleRate;
		ma_device_config config = ma_device_config_init(ma_device_type_playback);
		config.playback.format = ma_format_f32; // must match the decoder format in load()
		config.playback.channels = channels; // Set to 0 to use the device's native channel count.
		config.sampleRate = sampleRate; // Set to 0 to use the device's native sample rate.
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
		done = false;
		ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, channels, sampleRate);
		if (ma_decoder_init_file(path.c_str(), &decoderConfig, &decoder) != MA_SUCCESS)
		{
			printf("file open error \n");
			return false;
		}
		isLoaded = true;
		return true;
	}
	void play()
	{
		if (!isLoaded)
		{
			return;
		}
		ma_device_start(&device);
	}
};
