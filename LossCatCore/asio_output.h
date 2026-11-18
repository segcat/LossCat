#pragma once
// ASIO output (Windows only). The ASIO SDK keeps one driver loaded per process, so this is a
// single global output. Playback hands it 32-bit integer samples, interleaved; it converts them
// to whatever the driver wants and splits them into the driver's per-channel buffers.
#ifdef _WIN32
#include <string>
#include <vector>
#include <cstdint>

namespace asio
{
	// Fills frames * channels samples, interleaved; returns how many frames were real audio.
	typedef long (*Fill)(void* user, int32_t* samples, long frames, long channels);

	std::vector<std::string> drivers();

	// Loads the driver, sets the sample rate and prepares stereo (or `channels`) output buffers.
	// window: the app's main window, which some drivers need for their control panel.
	bool open(const std::string& driver, double sampleRate, long channels, Fill fill, void* user, void* window, std::string& error);
	bool start();
	void stop();
	void close();

	bool isOpen();
	const std::string& driverName();
	double sampleRate();
	long channels();
	int sampleType();   // ASIOSampleType of the first output channel
	int bits();         // bits of each sample the driver really carries (24 for 24-in-32, etc.)
	bool isFloat();
	bool canSampleRate(double rate);
}
#endif
