#pragma once
#include "trackinfo.h"
#include <string>
#include <vector>
#include <filesystem>
#include <functional>
#include <set>

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
    std::set<std::string> hiddenPaths;
public:

    bool open(const std::string& rootUtf8In,
              const std::vector<std::string>& folders = {},
              const std::vector<std::string>& hidden = {},
              const std::string& cachePathUtf8 = "",
              const std::function<void(int)>& progress = nullptr);
    bool forget(int index);
    bool hide(int index);   // for scanned files we don't want to delete them
    int importFile(const std::string& sourceUtf8, std::string& error);

    const std::vector<LibraryTrack>& tracks() const { return list; }
    const std::string& root() const { return rootUtf8; }
};
