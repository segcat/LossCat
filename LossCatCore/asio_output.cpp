#ifdef _WIN32
#include "asio_output.h"
#include <windows.h>
#include "asiosys.h"
#include "asio.h"
#include "asiodrivers.h"
#include <cstring>

extern AsioDrivers* asioDrivers;
bool loadAsioDriver(char* name);

namespace
{
	struct State
	{
		bool loaded = false, prepared = false, running = false;
		std::string name;
		double rate = 0;
		long channels = 0;
		long bufferFrames = 0;
		ASIOSampleType type = ASIOSTInt32LSB;
		ASIOBufferInfo buffers[8] = {};
		ASIOCallbacks callbacks = {};
		asio::Fill fill = nullptr;
		void* user = nullptr;
		std::vector<int32_t> scratch;   // interleaved samples for one buffer, allocated before start
		bool postOutput = false;
	};
	State s;

	void write(long index)
	{
		long frames = s.bufferFrames, ch = s.channels;
		int32_t* in = s.scratch.data();
		long got = s.fill ? s.fill(s.user, in, frames, ch) : 0;
		if (got < frames)
		{
			std::memset(in + got * ch, 0, (size_t)(frames - got) * ch * sizeof(int32_t));
		}
		for (long c = 0; c < ch; ++c)
		{
			void* out = s.buffers[c].buffers[index];
			switch (s.type)
			{
			case ASIOSTInt32LSB:
				for (long f = 0; f < frames; ++f) ((int32_t*)out)[f] = in[f * ch + c];
				break;
			case ASIOSTInt32LSB24:
				for (long f = 0; f < frames; ++f) ((int32_t*)out)[f] = in[f * ch + c] >> 8;
				break;
			case ASIOSTInt32LSB16:
				for (long f = 0; f < frames; ++f) ((int32_t*)out)[f] = in[f * ch + c] >> 16;
				break;
			case ASIOSTInt24LSB:
				for (long f = 0; f < frames; ++f)
				{
					int32_t v = in[f * ch + c] >> 8;
					unsigned char* p = (unsigned char*)out + f * 3;
					p[0] = (unsigned char)(v & 0xFF);
					p[1] = (unsigned char)((v >> 8) & 0xFF);
					p[2] = (unsigned char)((v >> 16) & 0xFF);
				}
				break;
			case ASIOSTInt16LSB:
				for (long f = 0; f < frames; ++f) ((int16_t*)out)[f] = (int16_t)(in[f * ch + c] >> 16);
				break;
			case ASIOSTFloat32LSB:
				for (long f = 0; f < frames; ++f) ((float*)out)[f] = (float)(in[f * ch + c] / 2147483648.0);
				break;
			case ASIOSTFloat64LSB:
				for (long f = 0; f < frames; ++f) ((double*)out)[f] = in[f * ch + c] / 2147483648.0;
				break;
			default:
				break;   // a big-endian or unusual type: silence rather than noise
			}
		}
		if (s.postOutput)
		{
			ASIOOutputReady();
		}
	}

	void bufferSwitch(long index, ASIOBool)
	{
		write(index);
	}

	ASIOTime* bufferSwitchTimeInfo(ASIOTime* params, long index, ASIOBool)
	{
		write(index);
		return params;
	}

	void sampleRateDidChange(ASIOSampleRate) {}

	long asioMessage(long selector, long value, void*, double*)
	{
		switch (selector)
		{
		case kAsioSelectorSupported:
			return value == kAsioEngineVersion || value == kAsioSupportsTimeInfo || value == kAsioResetRequest ? 1L : 0L;
		case kAsioEngineVersion:
			return 2L;
		case kAsioSupportsTimeInfo:
			return 1L;
		default:
			return 0L;
		}
	}

	bool knownType(ASIOSampleType t)
	{
		return t == ASIOSTInt32LSB || t == ASIOSTInt32LSB24 || t == ASIOSTInt32LSB16 || t == ASIOSTInt24LSB ||
			t == ASIOSTInt16LSB || t == ASIOSTFloat32LSB || t == ASIOSTFloat64LSB;
	}
}

namespace asio
{
	std::vector<std::string> drivers()
	{
		std::vector<std::string> out;
		if (!asioDrivers)
		{
			asioDrivers = new AsioDrivers();
		}
		char storage[32][64];
		char* names[32];
		for (int i = 0; i < 32; ++i) names[i] = storage[i];
		long n = asioDrivers->getDriverNames(names, 32);
		for (long i = 0; i < n; ++i) out.push_back(names[i]);
		return out;
	}

	bool open(const std::string& driver, double sampleRate, long wantChannels, Fill fill, void* user, void* window, std::string& error)
	{
		close();
		std::vector<char> name(driver.begin(), driver.end());
		name.push_back('\0');
		if (!loadAsioDriver(name.data()))
		{
			error = "Couldn't load the ASIO driver \"" + driver + "\"";
			return false;
		}
		s.loaded = true;
		s.name = driver;

		ASIODriverInfo info = {};
		info.asioVersion = 2;
		info.sysRef = window;
		if (ASIOInit(&info) != ASE_OK)
		{
			error = "The ASIO driver didn't start. Another app may be using it";
			close();
			return false;
		}

		if (ASIOCanSampleRate(sampleRate) != ASE_OK || ASIOSetSampleRate(sampleRate) != ASE_OK)
		{
			error = "The device can't run at this song's sample rate";
			close();
			return false;
		}
		ASIOSampleRate actual = 0;
		ASIOGetSampleRate(&actual);
		s.rate = actual > 0 ? actual : sampleRate;

		long ins = 0, outs = 0;
		ASIOGetChannels(&ins, &outs);
		s.channels = wantChannels < outs ? wantChannels : outs;
		if (s.channels > 8) s.channels = 8;
		if (s.channels < 1)
		{
			error = "The ASIO driver has no outputs";
			close();
			return false;
		}

		long minSize = 0, maxSize = 0, preferred = 0, granularity = 0;
		ASIOGetBufferSize(&minSize, &maxSize, &preferred, &granularity);
		s.bufferFrames = preferred > 0 ? preferred : (maxSize > 0 ? maxSize : 512);

		for (long c = 0; c < s.channels; ++c)
		{
			s.buffers[c] = {};
			s.buffers[c].isInput = ASIOFalse;
			s.buffers[c].channelNum = c;
		}
		s.callbacks.bufferSwitch = bufferSwitch;
		s.callbacks.sampleRateDidChange = sampleRateDidChange;
		s.callbacks.asioMessage = asioMessage;
		s.callbacks.bufferSwitchTimeInfo = bufferSwitchTimeInfo;
		s.fill = fill;
		s.user = user;
		s.scratch.assign((size_t)s.bufferFrames * s.channels, 0);
		if (ASIOCreateBuffers(s.buffers, s.channels, s.bufferFrames, &s.callbacks) != ASE_OK)
		{
			error = "The ASIO driver couldn't set up its buffers";
			close();
			return false;
		}
		s.prepared = true;

		ASIOChannelInfo ci = {};
		ci.channel = 0;
		ci.isInput = ASIOFalse;
		ASIOGetChannelInfo(&ci);
		s.type = ci.type;
		if (!knownType(s.type))
		{
			error = "The ASIO driver uses a sample format LossCat doesn't support";
			close();
			return false;
		}
		s.postOutput = ASIOOutputReady() == ASE_OK;
		return true;
	}

	bool start()
	{
		if (!s.prepared) return false;
		if (s.running) return true;
		s.running = ASIOStart() == ASE_OK;
		return s.running;
	}

	void stop()
	{
		if (s.running)
		{
			ASIOStop();
			s.running = false;
		}
	}

	void close()
	{
		stop();
		if (s.prepared)
		{
			ASIODisposeBuffers();
			s.prepared = false;
		}
		if (s.loaded)
		{
			ASIOExit();
			if (asioDrivers) asioDrivers->removeCurrentDriver();
			s.loaded = false;
		}
		s.fill = nullptr;
		s.rate = 0;
		s.channels = 0;
	}

	bool isOpen() { return s.prepared; }
	const std::string& driverName() { return s.name; }
	double sampleRate() { return s.rate; }
	long channels() { return s.channels; }
	int sampleType() { return (int)s.type; }

	int bits()
	{
		switch (s.type)
		{
		case ASIOSTInt16LSB: case ASIOSTInt32LSB16: return 16;
		case ASIOSTInt24LSB: case ASIOSTInt32LSB24: case ASIOSTFloat32LSB: return 24;
		case ASIOSTInt32LSB: return 32;
		case ASIOSTFloat64LSB: return 32;
		default: return 0;
		}
	}

	bool isFloat() { return s.type == ASIOSTFloat32LSB || s.type == ASIOSTFloat64LSB; }

	bool canSampleRate(double rate)
	{
		return s.loaded && ASIOCanSampleRate(rate) == ASE_OK;
	}
}
#endif
