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
    unsigned long long bytes = 0;   // file size on disk
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
    // rootUtf8In: LossCat's own folder, where imports are copied.
    // folders: more folders that are scanned in place, nothing copied.
    // hidden: files never listed (removed from the library but left on disk).
    // cachePathUtf8: remembers tags by file size and date so unchanged files aren't read again; "" for none.
    // progress: called with the number of files looked at so far.
    bool open(const std::string& rootUtf8In,
              const std::vector<std::string>& folders = {},
              const std::vector<std::string>& hidden = {},
              const std::string& cachePathUtf8 = "",
              const std::function<void(int)>& progress = nullptr);
    bool forget(int index);
    bool hide(int index);   // like forget, but the file stays and isn't listed again
    int importFile(const std::string& sourceUtf8, std::string& error);

    const std::vector<LibraryTrack>& tracks() const { return list; }
    const std::string& root() const { return rootUtf8; }
};
