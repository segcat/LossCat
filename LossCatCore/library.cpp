#include "library.h"
#include <cstring>

static std::string cleanName(std::string s, const char* fallback)
{
    for (char& c : s)
    {
        if (std::strchr("<>:\"/\\|?*", c) || (unsigned char)c < 32)
        {
            c = '_';
        }
    }
    while (!s.empty() && (s.back() == ' ' || s.back() == '.'))
    {
        s.pop_back();         
    }
    return s.empty() ? fallback : s;
}
int Library::addTrack(const std::filesystem::path& p)
{
    std::u8string u8 = p.u8string();
    std::string pathUtf8(u8.begin(), u8.end());
    TrackInfo info = readTrackInfo(pathUtf8);
    if (!info.valid)
    {
        return -1;
    }
    info.coverData.clear();
    info.coverData.shrink_to_fit();
    list.push_back({ pathUtf8, info });
    return (int)list.size() - 1;
}
bool Library::open(const std::string& rootUtf8In)
{
    list.clear();
    rootUtf8 = rootUtf8In;
    rootPath = std::filesystem::path(std::u8string(rootUtf8.begin(), rootUtf8.end()));

    std::error_code ec;
    std::filesystem::create_directories(rootPath, ec);
    if (ec)
    {
        return false;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(rootPath, ec))
    {
        if (!entry.is_regular_file(ec))
        {
            continue;
        }

        std::u8string u8 = entry.path().u8string();
        std::string pathUtf8(u8.begin(), u8.end());

        TrackInfo info = readTrackInfo(pathUtf8);
        if (!info.valid)
        {
            continue;
        }
        info.coverData.clear();
        info.coverData.shrink_to_fit();
        list.push_back({ pathUtf8, info });
    }
    return true;
}

int Library::importFile(const std::string& sourceUtf8, std::string& error)
{
    auto u8 = [](const std::string& s) {
        return std::filesystem::path(std::u8string(s.begin(), s.end()));
    };
    std::filesystem::path src = u8(sourceUtf8);

  
    TrackInfo info = readTrackInfo(sourceUtf8);
    if (!info.valid)
    {
        error = "Not an audio file";
        return -1;
    }

    std::string artist = cleanName(info.artist, "Unknown artist");
    std::string album  = cleanName(info.album, "Unknown album");

    std::u8string stem8 = src.stem().u8string();          
    std::string stem(stem8.begin(), stem8.end());
    std::string title  = cleanName(info.title, stem.c_str());

    std::string fileName = title;
    if (info.track > 0)
    {
        char num[16];
        snprintf(num, sizeof num, "%02u ", info.track);
        fileName = num + title;
    };
    

    std::filesystem::path dest = rootPath / u8(artist) / u8(album) / u8(fileName);
    dest += src.extension();                              

   
    std::error_code ec;
    std::filesystem::create_directories(dest.parent_path(), ec);
    if (ec)
    {
        error = ec.message();
        return -1;
    }

 
    auto srcSize = std::filesystem::file_size(src, ec);
    std::filesystem::path base = dest;
    for (int n = 2; std::filesystem::exists(dest, ec); ++n)
    {
        if (std::filesystem::file_size(dest, ec) == srcSize)
        {
            std::u8string d8 = dest.u8string();
            std::string destUtf8(d8.begin(), d8.end());
            for (int i = 0; i < (int)list.size(); ++i)
            {
                if (list[i].path == destUtf8) return i;
            }
            return addTrack(dest);        
        }
        dest = base.parent_path() / base.stem();
        dest += u8(" (" + std::to_string(n) + ")");
        dest += base.extension();
    }

    std::filesystem::copy_file(src, dest, ec);
    if (ec)
    {
        error = ec.message();
        return -1;
    }
    return addTrack(dest);
    return -1;
}


bool Library::forget(int index)
{
    if (index < 0 || static_cast<std::size_t>(index) >= list.size())
    {
        return false;
    }

    const std::string& pathUtf8 = list[index].path;
    std::filesystem::path folder =
        std::filesystem::path(std::u8string(pathUtf8.begin(), pathUtf8.end())).parent_path();
    list.erase(list.begin() + index);

    auto insideRoot = [&](const std::filesystem::path& p) {
        auto rel = p.lexically_normal().lexically_relative(rootPath.lexically_normal());
        return !rel.empty() && rel != "." && *rel.begin() != "..";
    };

    std::error_code ec;
    while (insideRoot(folder) && std::filesystem::is_empty(folder, ec) && !ec)
    {
        if (!std::filesystem::remove(folder, ec) || ec)
        {
            break;
        }
        folder = folder.parent_path();
    }
    return true;
}
