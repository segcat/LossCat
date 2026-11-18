#pragma once

#include <filesystem>
#include <string>
#include <vector>

struct Playlist
{
    std::string name;
    std::vector<std::string> paths;
};

class Playlists
{
private:
    std::filesystem::path libraryRoot;
    std::filesystem::path folder;
    std::vector<Playlist> lists;
    bool save(int index);
public:
    bool load(const std::string& rootUtf8);
    const std::vector<Playlist>& all() const { return lists; }

    int  create(const std::string& name);
    bool rename(int index, const std::string& name);
    bool remove(int index);

    bool add(int index, const std::string& absolutePathUtf8);
    bool removeAt(int index, int position);
    bool move(int index, int from, int to);
    const std::vector<Playlist>& playlists() const { return lists; }
};
