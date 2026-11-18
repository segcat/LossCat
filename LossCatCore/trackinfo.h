#pragma once
#include <string>
#include <vector>

struct TrackInfo
{
    bool valid = false;

    std::string title;
    std::string artist;
    std::string album;
    unsigned int year = 0;
    unsigned int track = 0;

    int durationMs = 0;
    int sampleRate = 0;
    int channels = 0;
    int bitsPerSample = 0;   // 0 = not stored in the file (lossy formats like MP3)
    int bitrateKbps = 0;

    std::string coverMime;
    std::vector<unsigned char> coverData;

    bool hasTrackGain = false;
    float trackGainDb = 0.0f;
    float trackPeak = 0.0f;      // linear, 1.0 = full scale
    bool hasAlbumGain = false;
    float albumGainDb = 0.0f;
    float albumPeak = 0.0f;

    std::string lyrics;          // unsynced lyrics tag, UTF-8; empty if none
};

TrackInfo readTrackInfo(const std::string& path);

// Embeds image (JPEG or PNG bytes) as the front cover, replacing any existing pictures.
// Only the tags change; the audio data is untouched.
bool writeCover(const std::string& path, const std::vector<unsigned char>& image, std::string& error);
