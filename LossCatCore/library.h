#pragma once
#include "trackinfo.h"
#include <string>
#include <vector>
#include <filesystem>

struct LibraryTrack
{
    std::string path;
    TrackInfo info;

};

class Library
{
private:
    int addTrack(const std::filesystem::path& p);
    std::filesystem::path rootPath;
    std::string rootUtf8;
    std::vector<LibraryTrack> list;
public:
    bool open(const std::string& rootUtf8In);
    bool forget(int index);
    int importFile(const std::string& sourceUtf8, std::string& error);

    const std::vector<LibraryTrack>& tracks() const { return list; }
    const std::string& root() const { return rootUtf8; }
};
