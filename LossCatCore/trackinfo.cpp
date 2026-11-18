#include "trackinfo.h"
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>
#include <taglib/flacproperties.h>
#include <taglib/wavproperties.h>
#include <taglib/aiffproperties.h>
#include <taglib/mp4properties.h>
#include <cstdlib>
#include <filesystem>

static bool readNumber(const TagLib::PropertyMap& tags, const char* key, float& out)
{
	auto it = tags.find(key);
	if (it == tags.end() || it->second.isEmpty())
	{
		return false;
	}
	std::string text = it->second.front().to8Bit(true);
	char* end = nullptr;
	double value = std::strtod(text.c_str(), &end);
	if (end == text.c_str())
	{
		return false;
	}
	out = static_cast<float>(value);
	return true;
}

TrackInfo readTrackInfo(const std::string& path)
{
	TrackInfo info;
	// path is UTF-8; on Windows path::c_str() is wchar_t, which TagLib opens with the Unicode API
	std::filesystem::path p(std::u8string(path.begin(), path.end()));
	TagLib::FileRef f(p.c_str());
	if (f.isNull())
	{
		return info;
	}
	if (TagLib::Tag* tag = f.tag()) //text tags
	{
		info.title = tag->title().to8Bit(true);
		info.artist = tag->artist().to8Bit(true);
		info.album = tag->album().to8Bit(true);
		info.year = tag->year();
		info.track = tag->track();
	}
	if (TagLib::AudioProperties* props = f.audioProperties()) //audio properties
	{
		info.durationMs = props->lengthInMilliseconds();
		info.sampleRate = props->sampleRate();
		info.channels = props->channels();
		info.bitrateKbps = props->bitrate();

		if (auto* flac = dynamic_cast<TagLib::FLAC::Properties*>(props))
		{
			info.bitsPerSample = flac->bitsPerSample();
		}
		else if (auto* wav = dynamic_cast<TagLib::RIFF::WAV::Properties*>(props))
		{
			info.bitsPerSample = wav->bitsPerSample();
		}
		else if (auto* aiff = dynamic_cast<TagLib::RIFF::AIFF::Properties*>(props))
		{
			info.bitsPerSample = aiff->bitsPerSample();
		}
		else if (auto* mp4 = dynamic_cast<TagLib::MP4::Properties*>(props))
		{
			info.bitsPerSample = mp4->bitsPerSample();   // real for ALAC, nominal for AAC
		}
	}
	auto pictures = f.complexProperties("PICTURE"); //cover art if it exists
	if (!pictures.isEmpty())
	{
		const auto& pic = pictures.front();
		TagLib::ByteVector data = pic.value("data").toByteVector();
		info.coverData.assign(data.data(), data.data() + data.size());
		info.coverMime = pic.value("mimeType").toString().to8Bit(true);
	}
	TagLib::PropertyMap tags = f.properties();
	info.hasTrackGain = readNumber(tags, "REPLAYGAIN_TRACK_GAIN", info.trackGainDb);
	readNumber(tags, "REPLAYGAIN_TRACK_PEAK", info.trackPeak);
	info.hasAlbumGain = readNumber(tags, "REPLAYGAIN_ALBUM_GAIN", info.albumGainDb);
	readNumber(tags, "REPLAYGAIN_ALBUM_PEAK", info.albumPeak);

	for (const char* key : { "LYRICS", "UNSYNCEDLYRICS" })
	{
		auto it = tags.find(key);
		if (it != tags.end() && !it->second.isEmpty())
		{
			info.lyrics = it->second.front().to8Bit(true);
			break;
		}
	}

	info.valid = true;
	return info;
}

bool writeCover(const std::string& path, const std::vector<unsigned char>& image, std::string& error)
{
	std::filesystem::path p(std::u8string(path.begin(), path.end()));
	TagLib::FileRef f(p.c_str());
	if (f.isNull())
	{
		error = "Can't open this file's tags";	
		return false;
	}
	TagLib::ByteVector data(reinterpret_cast<const char*>(image.data()), static_cast<unsigned int>(image.size()));
	TagLib::String mime = data.startsWith("\x89PNG\x0d\x0a\x1a\x0a") ? "image/png" : "image/jpeg";
	if (!f.setComplexProperties("PICTURE", { { { "data", data }, { "pictureType", "Front Cover" }, { "mimeType", mime } } }))
	{
		error = "This format can't hold cover art";
		return false;
	}
	if (!f.save())
	{
		error = "Couldn't save the file";
		return false;
	}
	return true;
}

bool writeLyrics(const std::string& path, const std::string& lyrics, std::string& error)
{
	std::filesystem::path p(std::u8string(path.begin(), path.end()));
	TagLib::FileRef f(p.c_str());
	if (f.isNull())
	{
		error = "Can't open this file's tags";
		return false;
	}
	TagLib::PropertyMap tags = f.properties();
	tags.erase("UNSYNCEDLYRICS");
	if (lyrics.empty()) tags.erase("LYRICS");
	else tags.replace("LYRICS", TagLib::StringList(TagLib::String(lyrics, TagLib::String::UTF8)));
	TagLib::PropertyMap rejected = f.setProperties(tags);
	if (!lyrics.empty() && rejected.contains("LYRICS"))
	{
		error = "This format can't hold lyrics";
		return false;
	}
	if (!f.save())
	{
		error = "Couldn't save the file";
		return false;
	}
	return true;
}
