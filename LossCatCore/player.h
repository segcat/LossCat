#pragma once
#include <string>
#include "miniaudio.h"
#include <cstdio>
#include <atomic>
#include <mutex>
#include <filesystem>
#include <vector>
#include "equalizer.h"
#ifdef _WIN32
#include "asio_output.h"
#endif
//A comment means I probably didn't use the best approach but whatever I could come up with so keep it in mind

enum class State { NoTrack, Playing, Paused, Finished };

class Player
{
private:
	
	State state = State::NoTrack;
	std::atomic<bool> done{ false };
	std::string filePath;
	bool isLoaded = false;
	bool isInit = false;
	bool deviceOpen = false;
	ma_decoder decoder;
	ma_device device;

	// Exclusive mode: the device is opened per track in the file's own format and rate,
	// so nothing is mixed or resampled. Falls back to shared mode if the device refuses.
	bool wantExclusive = false;
	bool exclusiveNow = false;
	// ASIO: the driver's own route to the hardware, which can also set the device's sample rate.
	bool wantAsio = false;
	bool asioNow = false;
	// The ASIO driver keeps running while paused and plays silence: stopping and starting it
	// takes long enough to hear. The mutex keeps its thread out of the decoder while it changes.
	std::atomic<bool> asioSilent{ true };
	std::mutex decoderMutex;
	std::string asioDriver;   // "" = the first installed driver
	void* window = nullptr;
	ma_format sourceFormat = ma_format_unknown;
	ma_uint32 sourceChannels = 0;
	ma_uint32 sourceRate = 0;
	int sourceBits = 0;   // the file's real bit depth, from its tags; 0 for lossy or unknown
	ma_uint32 wantChannels = 0;
	ma_uint32 wantSampleRate = 0;
	float masterVolume = 1.0f;
	std::string problem;
	std::string attempts;   // what exclusive mode tried for the current track, for diagnosing devices

	// Which output: the system default unless one was chosen by name.
	ma_context context;
	bool contextReady = false;
	std::string chosenName;
	ma_device_id chosenId;
	bool haveChosenId = false;
	//
	Equalizer eq;
	void processEq(void* samples, ma_format format, ma_uint64 frames, ma_uint32 ch)
	{
		if (!eq.isActive() || ch != 2 || frames == 0) return;
		if (format == ma_format_f32)
		{
			eq.process((float*)samples, (int)frames, (int)ch);
			return;
		}
		ma_uint32 bytes = ma_get_bytes_per_sample(format);
		if (bytes == 0) return;
		// Small chunks keep this off the heap on the audio thread.
		float temp[1024];
		for (ma_uint64 f = 0; f < frames;)
		{
			int count = (int)std::min<ma_uint64>(frames - f, 512);
			void* out = (unsigned char*)samples + f * ch * bytes;
			ma_pcm_convert(temp, ma_format_f32, out, format, count * ch, ma_dither_mode_none);
			eq.process(temp, count, (int)ch);
			for (int i = 0; i < count * (int)ch; ++i)
				temp[i] = std::clamp(temp[i], -1.0f, 1.0f);
			ma_pcm_convert(out, format, temp, ma_format_f32, count * ch, ma_dither_mode_none);
			f += count;
		}
	}
	bool ensureContext()
	{
		if (!contextReady)
		{
			contextReady = ma_context_init(NULL, 0, NULL, &context) == MA_SUCCESS;
		}
		return contextReady;
	}

	// Looks the chosen name up again each time, so a device plugged back in is found.
	void resolveChosen()
	{
		haveChosenId = false;
		if (chosenName.empty() || !ensureContext())
		{
			return;
		}
		ma_device_info* infos = nullptr;
		ma_uint32 count = 0;
		if (ma_context_get_devices(&context, &infos, &count, NULL, NULL) != MA_SUCCESS)
		{
			return;
		}
		for (ma_uint32 i = 0; i < count; ++i)
		{
			if (chosenName == infos[i].name)
			{
				chosenId = infos[i].id;
				haveChosenId = true;
				return;
			}
		}
	}

	// In exclusive mode the device must really run at what was asked for; miniaudio may
	// otherwise quietly fall back to the device's own format and convert.
	bool runsAsAsked(ma_format format, ma_uint32 ch, ma_uint32 rate) const
	{
		return device.playback.internalFormat == format &&
			device.playback.internalChannels == ch &&
			device.playback.internalSampleRate == rate;
	}

	bool openDevice(ma_format format, ma_uint32 ch, ma_uint32 rate, bool exclusive)
	{
		if (deviceOpen)
		{
			if (exclusive == exclusiveNow && device.playback.format == format &&
				device.playback.channels == ch && device.sampleRate == rate)
			{
				return true;   // already open the same way
			}
			ma_device_uninit(&device);
			deviceOpen = false;
		}
		ma_device_config config = ma_device_config_init(ma_device_type_playback);
		resolveChosen();
		config.playback.pDeviceID = haveChosenId ? &chosenId : NULL;
		config.playback.format = format;
		config.playback.channels = ch;   // 0: the device's own
		config.sampleRate = rate;        // 0: the device's own
		config.playback.shareMode = exclusive ? ma_share_mode_exclusive : ma_share_mode_shared;
		config.dataCallback = data_callback;
		config.pUserData = this;
		if (ma_device_init(ensureContext() ? &context : NULL, &config, &device) != MA_SUCCESS)
		{
			// Let miniaudio try every backend on the default device instead.
			config.playback.pDeviceID = NULL;
			if (ma_device_init(NULL, &config, &device) != MA_SUCCESS)
			{
				return false;
			}
		}
		deviceOpen = true;
		exclusiveNow = exclusive;
		channels = device.playback.channels;
		sampleRate = device.sampleRate;
		ma_device_set_master_volume(&device, masterVolume);
		return true;
	}

	void closeDecoder()
	{
		if (!isLoaded)
		{
			return;
		}
		outputStop();
		ma_decoder_uninit(&decoder);
		isLoaded = false;
	}

	bool outputOpen() const { return deviceOpen || asioNow; }
	bool outputStart()
	{
#ifdef _WIN32
		if (asioNow)
		{
			asioSilent = false;
			return asio::start();
		}
#endif
		return deviceOpen && ma_device_start(&device) == MA_SUCCESS;
	}
	void outputStop()
	{
#ifdef _WIN32
		if (asioNow)
		{
			asioSilent = true;
			std::lock_guard<std::mutex> wait(decoderMutex);   // returns once the driver's thread is out of the decoder
			return;
		}
#endif
		if (deviceOpen) ma_device_stop(&device);
	}

#ifdef _WIN32
	// Runs on the ASIO driver's thread, like data_callback does for miniaudio.
	static long asioFill(void* user, int32_t* out, long frames, long ch)
	{
		Player* player = static_cast<Player*>(user);
		if (player->asioSilent || !player->decoderMutex.try_lock())
		{
			return 0;   // silence; never wait on the audio thread
		}
		std::lock_guard<std::mutex> held(player->decoderMutex, std::adopt_lock);
		if (player->asioSilent)
		{
			return 0;
		}
		ma_uint64 framesRead = 0;
		ma_decoder_read_pcm_frames(&player->decoder, out, (ma_uint64)frames, &framesRead);
		player->processEq(out, ma_format_s32, framesRead, (ma_uint32)ch);
		player->framesPlayed += framesRead;
		float v = player->masterVolume;
		if (v < 0.9999f)
		{
			for (ma_uint64 i = 0; i < framesRead * (ma_uint64)ch; ++i) out[i] = (int32_t)(out[i] * (double)v);
		}
		if (framesRead < (ma_uint64)frames)
		{
			player->done = true;
		}
		return (long)framesRead;
	}
#endif

	ma_result openDecoder(const std::string& path, ma_format format, ma_uint32 ch, ma_uint32 rate)
	{
		ma_decoder_config decoderConfig = ma_decoder_config_init(format, ch, rate);
#ifdef _WIN32
		std::filesystem::path p(std::u8string(path.begin(), path.end()));
		return ma_decoder_init_file_w(p.c_str(), &decoderConfig, &decoder);
#else
		return ma_decoder_init_file(path.c_str(), &decoderConfig, &decoder);
#endif
	}
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
		player->processEq(pOutput, pDevice->playback.format, framesRead, pDevice->playback.channels);
		player->framesPlayed += framesRead;
		if (framesRead < frameCount)	
		{
			player->done = true;
			return;
		}
	}
public:
	Equalizer& equalizer() { return eq; }
	bool equalizerActive() const { return channels == 2 && eq.isActive(); }
	Player(const Player&) = delete;
	Player& operator=(const Player&) = delete;
	Player() {
		printf("Constructor Called \n");
	}
	~Player() {
		printf("Deconstructor Called \n");
		uninit();
		if (contextReady)
		{
			ma_context_uninit(&context);
		}
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
		outputStop(); //callback must not be reading decoder while we move it
		if (ma_decoder_seek_to_pcm_frame(&decoder, frame) != MA_SUCCESS)
		{
			if (wasPlaying)
			{
				outputStart();
			}
			return false;
		}
		framesPlayed = frame;
		done = false;
		if (wasPlaying)
		{
			outputStart();
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
		return masterVolume;
	}
	bool isFinished() const
	{
		return getState() == State::Finished;
	}
	bool init(ma_uint32 channelsWanted, ma_uint32 sampleRateWanted)
	{
		if (isInit)
		{
			return true;
		}
		wantChannels = channelsWanted;     // 0: the device's own channel count
		wantSampleRate = sampleRateWanted; // 0: the device's own rate
		if (wantExclusive || wantAsio)
		{
			isInit = true;   // the device is opened by load(), in the file's format
			return true;
		}
		if (!openDevice(ma_format_f32, wantChannels, wantSampleRate, false))
		{
			printf("failed to open audio device\n");
			return false;
		}
		isInit = true;
		return true;
	}
	void uninit()
	{
		if (isLoaded)
		{
			closeDecoder();
			state = State::NoTrack;
			totalFrames = 0;
			framesPlayed = 0;
			filePath.clear();
		}
		if (deviceOpen)
		{
			ma_device_uninit(&device);
			deviceOpen = false;
		}
#ifdef _WIN32
		if (asioNow)
		{
			asio::close();
			asioNow = false;
		}
#endif
		isInit = false;
		exclusiveNow = false;
	}
	// 0 shared, 1 WASAPI exclusive, 2 ASIO. Takes effect after uninit() and init().
	void setMode(int mode)
	{
		wantExclusive = mode == 1;
		wantAsio = mode == 2;
	}
	void chooseAsioDriver(const std::string& name) { asioDriver = name; }
	void setWindow(void* hwnd) { window = hwnd; }
	bool asioActive() const { return asioNow; }
	// Takes effect from the next load(); call uninit() and init() to switch the open device.
	void setExclusive(bool on)
	{
		wantExclusive = on;
		wantAsio = false;
	}
	bool exclusiveWanted() const { return wantExclusive; }
	bool exclusiveActive() const { return deviceOpen && exclusiveNow; }
	// Why exclusive mode isn't in use for this track, if it was asked for.
	const std::string& exclusiveProblem() const { return problem; }
	const std::string& exclusiveAttempts() const { return attempts; }
	// The samples reach the device unchanged: exclusive, same rate and channels, a format with
	// room for every bit of the file, and full volume. fileBits is the file's real bit depth from
	// its tags (decoders often hand 24-bit audio over in 32-bit samples); 0 if unknown.
	bool bitPerfect(int fileBits = 0) const
	{
		if (equalizerActive()) return false;
		if (fileBits <= 0)
		{
			fileBits = sourceBits;
		}
#ifdef _WIN32
		if (asioNow)
		{
			return isLoaded && masterVolume >= 0.9999f && fileBits > 0 &&
				(ma_uint32)asio::sampleRate() == sourceRate && asio::bits() >= fileBits;
		}
#endif
		if (!exclusiveActive() || !isLoaded || masterVolume < 0.9999f)
		{
			return false;
		}
		if (device.playback.internalSampleRate != sourceRate || device.playback.internalChannels != sourceChannels)
		{
			return false;
		}
		ma_format out = device.playback.internalFormat;
		if (fileBits <= 0)
		{
			return sourceFormat == out;   // lossy: the decoder's output passes through untouched
		}
		int need = fileBits > 0 ? fileBits : (int)ma_get_bytes_per_sample(sourceFormat) * 8;
		int room = out == ma_format_f32 ? 24 : (int)ma_get_bytes_per_sample(out) * 8;   // float holds 24 bits exactly
		return room >= need;
	}
	// fileBits: the file's bit depth from its tags (0 for lossy or unknown), used by exclusive mode.
	bool load(const std::string& path, int fileBits = 0) //load always leaves the player paused
	{
		if (!isInit)
		{
			return false;
		}
		closeDecoder();
		problem.clear();
		ma_result opened;
#ifdef _WIN32
		if (wantAsio)
		{
			if (deviceOpen)
			{
				ma_device_uninit(&device);
				deviceOpen = false;
			}
			sourceBits = fileBits;
			attempts.clear();
			opened = openDecoder(path, ma_format_s32, 0, 0);
			if (opened == MA_SUCCESS)
			{
				sourceFormat = decoder.outputFormat;
				sourceChannels = decoder.outputChannels;
				sourceRate = decoder.outputSampleRate;
				ma_decoder_uninit(&decoder);

				std::string driver = asioDriver;
				if (driver.empty())
				{
					std::vector<std::string> all = asio::drivers();
					if (!all.empty()) driver = all.front();
				}
				long wantCh = sourceChannels < 2 ? 2 : (long)sourceChannels;
				bool reuse = asioNow && asio::driverName() == driver && (ma_uint32)asio::sampleRate() == sourceRate;
				if (!reuse)
				{
					std::string err = driver.empty() ? "No ASIO driver is installed" : "";
					asioNow = !driver.empty() && asio::open(driver, (double)sourceRate, wantCh, asioFill, this, window, err);
					if (!asioNow) problem = err;
				}
				if (asioNow)
				{
					channels = (ma_uint32)asio::channels();
					sampleRate = (ma_uint32)asio::sampleRate();
					opened = openDecoder(path, ma_format_s32, channels, sampleRate);
				}
				else
				{
					if (!openDevice(ma_format_f32, wantChannels, wantSampleRate, false))
					{
						return false;
					}
					problem += ". Playing in shared mode";
					opened = openDecoder(path, ma_format_f32, channels, sampleRate);
				}
			}
		}
		else
#endif
		if (!wantExclusive)
		{
			if ((!deviceOpen || exclusiveNow) && !openDevice(ma_format_f32, wantChannels, wantSampleRate, false))
			{
				return false;
			}
			opened = openDecoder(path, ma_format_f32, channels, sampleRate);
			if (opened == MA_SUCCESS)
			{
				sourceFormat = ma_format_f32;
				sourceChannels = channels;
				sourceRate = sampleRate;
			}
		}
		else
		{
			// Lossless files are decoded as integers: some decoders default to float, and float
			// back to integer isn't exact. 32-bit holds 16- and 24-bit samples unchanged.
			sourceBits = fileBits;
			opened = openDecoder(path, fileBits > 0 ? ma_format_s32 : ma_format_unknown, 0, 0);
			if (opened == MA_SUCCESS)
			{
				sourceFormat = decoder.outputFormat;
				sourceChannels = decoder.outputChannels;
				sourceRate = decoder.outputSampleRate;
				ma_decoder_uninit(&decoder);

				// The narrowest format that still holds every bit of the file first; dropping the
				// unused low bits of the 32-bit samples to get there changes nothing.
				std::vector<ma_format> tries;
				if (fileBits > 0 && fileBits <= 16) tries = { ma_format_s16, ma_format_s24, ma_format_s32, ma_format_f32 };
				else if (fileBits > 0 && fileBits <= 24) tries = { ma_format_s24, ma_format_s32, ma_format_f32 };
				else if (fileBits > 0) tries = { ma_format_s32 };
				else tries = { sourceFormat, ma_format_s32, ma_format_s24, ma_format_s16 };   // lossy: nothing to keep exact
				bool ok = false;
				attempts.clear();
				for (ma_format f : tries)
				{
					bool opened = openDevice(f, sourceChannels, sourceRate, true);
					char line[160];
					if (opened)
					{
						snprintf(line, sizeof line, "%s %u Hz: opened %s %u Hz; ", ma_get_format_name(f), sourceRate,
							ma_get_format_name(device.playback.internalFormat), device.playback.internalSampleRate);
					}
					else
					{
						snprintf(line, sizeof line, "%s %u Hz: refused; ", ma_get_format_name(f), sourceRate);
					}
					attempts += line;
					if (opened && runsAsAsked(f, sourceChannels, sourceRate))
					{
						ok = true;
						break;
					}
				}
				// None exact: exclusive at the device's own format still avoids the Windows mixer.
				if (!ok && openDevice(tries.front(), sourceChannels, sourceRate, true))
				{
					ok = true;
					problem = "The device can't play this file's format as is, so it's converted";
				}
				if (!ok)
				{
					problem = "The device doesn't accept this file's format in exclusive mode, or another app is using it";
					if (!openDevice(ma_format_f32, wantChannels, wantSampleRate, false))
					{
						return false;
					}
				}
				opened = openDecoder(path, device.playback.format, channels, sampleRate);
			}
		}
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
		eq.prepare((double)decoder.outputSampleRate);
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
			outputStop();
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
		if (!outputStart())
		{
			state = State::Paused;
			return false;
		}
		state = State::Playing;
		return true;
	}
	void pause()
	{
		if (!isLoaded || !outputOpen())
		{
			return;
		}
		outputStop();
		state = State::Paused;

	}
	void stop()
	{
		pause();
		seek(0);
	}
	bool setVolume(float v)
	{
		if (v < 0.0f) v = 0.0f;
		if (v > 1.0f) v = 1.0f;
		masterVolume = v;
		if (!deviceOpen)
		{
			return isInit;
		}
		return ma_device_set_master_volume(&device, v) == MA_SUCCESS;
	}
	const std::string& currentPath() const
	{
		return filePath;
	}

	// Output devices by name; choose one with chooseDevice ("" = system default), then uninit() and init().
	std::vector<std::string> outputDevices()
	{
#ifdef _WIN32
		if (wantAsio)
		{
			return asio::drivers();
		}
#endif
		std::vector<std::string> names;
		if (!ensureContext())
		{
			return names;
		}
		ma_device_info* infos = nullptr;
		ma_uint32 count = 0;
		if (ma_context_get_devices(&context, &infos, &count, NULL, NULL) == MA_SUCCESS)
		{
			for (ma_uint32 i = 0; i < count; ++i)
			{
				names.push_back(infos[i].name);
			}
		}
		return names;
	}
	void chooseDevice(const std::string& name) { chosenName = name; }
	const std::string& chosenDevice() const { return chosenName; }

#ifdef _WIN32
	const char* deviceName() const { return asioNow ? asio::driverName().c_str() : deviceOpen ? device.playback.name : ""; }
	const char* backendName() const { return asioNow ? "ASIO" : deviceOpen ? ma_get_backend_name(device.pContext->backend) : ""; }
	ma_uint32 deviceSampleRate() const { return asioNow ? (ma_uint32)asio::sampleRate() : deviceOpen ? device.playback.internalSampleRate : 0; }
	ma_uint32 deviceChannels() const { return asioNow ? (ma_uint32)asio::channels() : deviceOpen ? device.playback.internalChannels : 0; }
	ma_format deviceFormat() const
	{
		if (asioNow) return asio::isFloat() ? ma_format_f32 : asio::bits() == 16 ? ma_format_s16 : asio::bits() == 24 ? ma_format_s24 : ma_format_s32;
		return deviceOpen ? device.playback.internalFormat : ma_format_unknown;
	}
#else
	const char* deviceName() const { return deviceOpen ? device.playback.name : ""; }
	const char* backendName() const { return deviceOpen ? ma_get_backend_name(device.pContext->backend) : ""; }
	ma_uint32 deviceSampleRate() const { return deviceOpen ? device.playback.internalSampleRate : 0; }
	ma_uint32 deviceChannels() const { return deviceOpen ? device.playback.internalChannels : 0; }
	ma_format deviceFormat() const { return deviceOpen ? device.playback.internalFormat : ma_format_unknown; }
#endif
	const char* deviceFormatName() const { return deviceOpen ? ma_get_format_name(device.playback.internalFormat) : ""; }
	ma_uint32 outputSampleRate() const { return sampleRate; }
};
