#include "trackinfo.h"
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>
#include <taglib/flacproperties.h>
#include <taglib/wavproperties.h>
#include <taglib/aiffproperties.h>
#include <taglib/mp4properties.h>

TrackInfo readTrackInfo(const std::string& path)
{
	TrackInfo info;
	TagLib::FileRef f(path.c_str());
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
	info.valid = true;
	return info;
}
