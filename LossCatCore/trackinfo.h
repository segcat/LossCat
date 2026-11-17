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
};

TrackInfo readTrackInfo(const std::string& path);
