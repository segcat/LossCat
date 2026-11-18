#include "library.h"
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <map>
#include "names.h"
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
namespace
{
    struct CachedTags
    {
        unsigned long long size = 0;
        long long modified = 0;
        TrackInfo info;
    };

    std::filesystem::path u8path(const std::string& s)
    {
        return std::filesystem::path(std::u8string(s.begin(), s.end()));
    }

    std::string escapeField(const std::string& s)
    {
        std::string out;
        for (char c : s)
        {
            if (c == '\\') out += "\\\\";
            else if (c == '\t') out += "\\t";
            else if (c == '\n') out += "\\n";
            else if (c == '\r') out += "\\r";
            else out += c;
        }
        return out;
    }

    std::vector<std::string> splitFields(const std::string& line)
    {
        std::vector<std::string> fields(1);
        for (size_t i = 0; i < line.size(); ++i)
        {
            char c = line[i];
            if (c == '\t')
            {
                fields.emplace_back();
            }
            else if (c == '\\' && i + 1 < line.size())
            {
                char n = line[++i];
                fields.back() += n == 't' ? '\t' : n == 'n' ? '\n' : n == 'r' ? '\r' : n;
            }
            else
            {
                fields.back() += c;
            }
        }
        return fields;
    }

    // Only files with these extensions are opened, so folders full of scans and logs stay fast.
    bool audioExtension(const std::filesystem::path& p)
    {
        std::u8string e8 = p.extension().u8string();
        std::string e(e8.begin(), e8.end());
        for (char& c : e)
        {
            c = (char)std::tolower((unsigned char)c);
        }
        for (const char* ok : { ".flac", ".wav", ".aif", ".aiff", ".mp3", ".m4a", ".aac", ".ogg", ".oga", ".opus", ".wv", ".ape", ".wma", ".dsf", ".dff" })
        {
            if (e == ok) return true;
        }
        return false;
    }

    const int cacheFields = 20;

    std::map<std::string, CachedTags> loadCache(const std::string& file)
    {
        std::map<std::string, CachedTags> cache;
        if (file.empty()) return cache;
        std::ifstream in(u8path(file), std::ios::binary);
        std::string line;
        if (!std::getline(in, line) || line.rfind("LossCat cache 1", 0) != 0) return cache;
        while (std::getline(in, line))
        {
            std::vector<std::string> f = splitFields(line);
            if ((int)f.size() != cacheFields) continue;
            CachedTags c;
            c.size = std::strtoull(f[1].c_str(), nullptr, 10);
            c.modified = std::strtoll(f[2].c_str(), nullptr, 10);
            TrackInfo& t = c.info;
            t.valid = true;
            t.title = f[3];
            t.artist = f[4];
            t.album = f[5];
            t.year = (unsigned)std::strtoul(f[6].c_str(), nullptr, 10);
            t.track = (unsigned)std::strtoul(f[7].c_str(), nullptr, 10);
            t.durationMs = std::atoi(f[8].c_str());
            t.sampleRate = std::atoi(f[9].c_str());
            t.channels = std::atoi(f[10].c_str());
            t.bitsPerSample = std::atoi(f[11].c_str());
            t.bitrateKbps = std::atoi(f[12].c_str());
            t.hasTrackGain = f[13] == "1";
            t.trackGainDb = (float)std::atof(f[14].c_str());
            t.trackPeak = (float)std::atof(f[15].c_str());
            t.hasAlbumGain = f[16] == "1";
            t.albumGainDb = (float)std::atof(f[17].c_str());
            t.albumPeak = (float)std::atof(f[18].c_str());
            t.lyrics = f[19];
            cache[f[0]] = std::move(c);
        }
        return cache;
    }

    void saveCache(const std::string& file, const std::map<std::string, CachedTags>& cache)
    {
        if (file.empty()) return;
        std::filesystem::path target = u8path(file);
        std::filesystem::path tmp = target;
        tmp += ".tmp";
        {
            std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
            if (!out) return;
            out << "LossCat cache 1\n";
            for (const auto& [path, c] : cache)
            {
                const TrackInfo& t = c.info;
                out << escapeField(path) << '\t' << c.size << '\t' << c.modified << '\t'
                    << escapeField(t.title) << '\t' << escapeField(t.artist) << '\t' << escapeField(t.album) << '\t'
                    << t.year << '\t' << t.track << '\t' << t.durationMs << '\t' << t.sampleRate << '\t'
                    << t.channels << '\t' << t.bitsPerSample << '\t' << t.bitrateKbps << '\t'
                    << (t.hasTrackGain ? 1 : 0) << '\t' << t.trackGainDb << '\t' << t.trackPeak << '\t'
                    << (t.hasAlbumGain ? 1 : 0) << '\t' << t.albumGainDb << '\t' << t.albumPeak << '\t'
                    << escapeField(t.lyrics) << '\n';
            }
            if (!out) return;
        }
        std::error_code ec;
        std::filesystem::rename(tmp, target, ec);
    }
}

bool Library::open(const std::string& rootUtf8In,
                   const std::vector<std::string>& folders,
                   const std::vector<std::string>& hidden,
                   const std::string& cachePathUtf8,
                   const std::function<void(int)>& progress)
{
    list.clear();
    rootUtf8 = rootUtf8In;
    rootPath = u8path(rootUtf8);
    hiddenPaths = std::set<std::string>(hidden.begin(), hidden.end());

    std::error_code ec;
    std::filesystem::create_directories(rootPath, ec);
    if (ec)
    {
        return false;
    }

    std::map<std::string, CachedTags> cache = loadCache(cachePathUtf8);
    std::map<std::string, CachedTags> seen;  
    int looked = 0;

    std::vector<std::string> roots{ rootUtf8 };
    roots.insert(roots.end(), folders.begin(), folders.end());
    for (const std::string& rootUtf8Item : roots)
    {
        std::error_code walkEc;
        auto options = std::filesystem::directory_options::skip_permission_denied;
        for (std::filesystem::recursive_directory_iterator it(u8path(rootUtf8Item), options, walkEc), end;
             !walkEc && it != end; it.increment(walkEc))
        {
            std::error_code fileEc;
            if (!it->is_regular_file(fileEc) || !audioExtension(it->path()))
            {
                continue;
            }
            std::u8string u8 = it->path().u8string();
            std::string pathUtf8(u8.begin(), u8.end());
            if (seen.count(pathUtf8) || hiddenPaths.count(pathUtf8))
            {
                continue;   // folders inside other folders would list a file twice
            }

            CachedTags entry;
            entry.size = it->file_size(fileEc);
            entry.modified = (long long)it->last_write_time(fileEc).time_since_epoch().count();
            auto hit = cache.find(pathUtf8);
            if (hit != cache.end() && hit->second.size == entry.size && hit->second.modified == entry.modified)
            {
                entry.info = hit->second.info;
            }
            else
            {
                entry.info = readTrackInfo(pathUtf8);
                entry.info.coverData.clear();
                entry.info.coverData.shrink_to_fit();
            }
            if (progress && ++looked % 25 == 0)
            {
                progress(looked);
            }
            if (!entry.info.valid)
            {
                continue;
            }
            list.push_back({ pathUtf8, entry.info });
            seen[pathUtf8] = std::move(entry);
        }
    }
    saveCache(cachePathUtf8, seen);
    if (progress)
    {
        progress(looked);
    }
    return true;
}

bool Library::hide(int index)
{
    if (index < 0 || static_cast<std::size_t>(index) >= list.size())
    {
        return false;
    }
    hiddenPaths.insert(list[index].path);
    list.erase(list.begin() + index);
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
