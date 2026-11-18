#include "playlists.h"
#include <fstream>
#include <algorithm>
#include "names.h"
bool Playlists::load(const std::string& rootUtf8)
{
    libraryRoot = std::filesystem::path(std::u8string(rootUtf8.begin(), rootUtf8.end()));
    folder = libraryRoot / "Playlists";

    std::error_code ec;
    std::filesystem::create_directories(folder, ec);
    lists.clear();
    if (ec)
    {
        lists.push_back({ "Favourites", {} });
        return false;
    }

    std::filesystem::directory_iterator it(folder, ec), end;
    for (; !ec && it != end; it.increment(ec))
    {
        const auto& entry = *it;
        std::error_code fileEc;                                   
        if (!entry.is_regular_file(fileEc) || entry.path().extension() != ".m3u8")
        {
            continue;
        }

        Playlist p;
        std::u8string name = entry.path().stem().u8string();
        p.name = std::string(name.begin(), name.end());

        std::ifstream in(entry.path());
        std::string line;
        bool first = true;
        while (std::getline(in, line))
        {
            if (first && line.size() >= 3 && line.compare(0, 3, "\xEF\xBB\xBF") == 0)
            {
                line.erase(0, 3);                                 
            }
            first = false;

            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }
            if (line.empty() || line.front() == '#')
            {
                continue;
            }
            p.paths.push_back(line);
        }
        lists.push_back(std::move(p));
    }

    std::sort(lists.begin(), lists.end(), [](const Playlist& a, const Playlist& b) {
        return a.name < b.name;
        });
    auto favourites = std::find_if(lists.begin(), lists.end(), [](const Playlist& p) {
        return p.name == "Favourites";
        });
    if (favourites == lists.end())
    {
        lists.insert(lists.begin(), { "Favourites", {} });
    }
    else
    {
        std::rotate(lists.begin(), favourites, favourites + 1);
    }
    return !ec;
}

bool Playlists::save(int index)
{
    if (index < 0 || index >= (int)lists.size())
    {
        return false;
    }
    const Playlist& p = lists[index];
    std::filesystem::path file = folder / std::filesystem::path(std::u8string(p.name.begin(), p.name.end()));
    file += ".m3u8";
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    out << "#EXTM3U\n";
    for (const std::string& path : p.paths)
    {
        out << path << "\n";
    }
    return (bool)out;
}

int Playlists::create(const std::string& name)
{
    std::string clean = cleanName(name, "New playlist");
    for (int i = 0; i < (int)lists.size(); ++i)
    {
        if (sameName(lists[i].name, clean))
        {
            return i;
        }
    }

    lists.push_back({ clean, {} });
    int index = (int)lists.size() - 1;
    if (!save(index))
    {
        lists.pop_back();
        return -1;
    }
    return index;
}

bool Playlists::add(int index, const std::string& absolutePathUtf8)
{
    if (index < 0 || index >= (int)lists.size())
    {
        return false;
    }

    std::filesystem::path abs(std::u8string(absolutePathUtf8.begin(), absolutePathUtf8.end()));
    // Songs under the library folder are stored relative to it; songs from other folders by full path.
    std::filesystem::path rel = abs.lexically_relative(libraryRoot);
    if (rel.empty() || *rel.begin() == "..")
    {
        rel = abs;
    }

    std::u8string rel8 = rel.u8string();
    std::string relUtf8(rel8.begin(), rel8.end());
    auto& paths = lists[index].paths;
    if (index == 0 && std::find(paths.begin(), paths.end(), relUtf8) != paths.end())
    {
        return true;
    }

    paths.push_back(relUtf8);
    if (!save(index))
    {
        paths.pop_back();
        return false;
    }
    return true;
}

bool Playlists::removeAt(int index, int position)
{
    if (index < 0 || index >= (int)lists.size())
    {
        return false;
    }
    auto& paths = lists[index].paths;
    if (position < 0 || position >= (int)paths.size())
    {
        return false;
    }

    std::string removed = paths[position];
    paths.erase(paths.begin() + position);
    if (!save(index))
    {
        paths.insert(paths.begin() + position, removed);
        return false;
    }
    return true;
}

bool Playlists::move(int index, int from, int to)
{
    if (index < 0 || index >= (int)lists.size())
    {
        return false;
    }
    auto& paths = lists[index].paths;
    if (from < 0 || from >= (int)paths.size() || to < 0 || to >= (int)paths.size())
    {
        return false;
    }

    std::string path = paths[from];
    paths.erase(paths.begin() + from);
    paths.insert(paths.begin() + to, path);
    if (!save(index))
    {
        paths.erase(paths.begin() + to);
        paths.insert(paths.begin() + from, path);
        return false;
    }
    return true;
}

bool Playlists::rename(int index, const std::string& name)
{
    if (index <= 0 || index >= (int)lists.size())
    {
        return false;
    }

    std::string clean = cleanName(name, "New playlist");
    for (int i = 0; i < (int)lists.size(); ++i)
    {
        if (i != index && sameName(lists[i].name, clean))
        {
            return false;
        }
    }

    const std::string& oldName = lists[index].name;
    std::filesystem::path oldFile = folder / std::filesystem::path(std::u8string(oldName.begin(), oldName.end()));
    oldFile += ".m3u8";
    std::filesystem::path newFile = folder / std::filesystem::path(std::u8string(clean.begin(), clean.end()));
    newFile += ".m3u8";

    std::error_code ec;
    std::filesystem::rename(oldFile, newFile, ec);
    if (ec)
    {
        return false;
    }
    lists[index].name = clean;
    return true;
}

bool Playlists::remove(int index)
{
    if (index <= 0 || index >= (int)lists.size())
    {
        return false;
    }

    const std::string& name = lists[index].name;
    std::filesystem::path file = folder / std::filesystem::path(std::u8string(name.begin(), name.end()));
    file += ".m3u8";

    std::error_code ec;
    std::filesystem::remove(file, ec);
    if (ec)
    {
        return false;
    }
    lists.erase(lists.begin() + index);
    return true;
}
