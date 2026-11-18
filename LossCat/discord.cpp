#include "discord.h"
#include <windows.h>
#include <winhttp.h>
#include <chrono>

#pragma comment(lib, "winhttp")

namespace
{
	std::string escape(const std::string& s)
	{
		std::string out;
		for (unsigned char c : s)
		{
			if (c == '"' || c == '\\') { out += '\\'; out += (char)c; }
			else if (c < 0x20) { char b[8]; snprintf(b, sizeof b, "\\u%04x", c); out += b; }
			else out += (char)c;
		}
		return out;
	}

	// Discord rejects details/state shorter than 2 or longer than 128 characters.
	std::string field(std::string s)
	{
		if (s.size() > 120)
		{
			size_t cut = 120;
			while (cut > 0 && ((unsigned char)s[cut] & 0xC0) == 0x80) --cut;   // don't split a UTF-8 sequence
			s = s.substr(0, cut) + "\xE2\x80\xA6";
		}
		while (s.size() < 2) s += ' ';
		return "\"" + escape(s) + "\"";
	}

	std::wstring wide(const std::string& s)
	{
		int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
		std::wstring w(n, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &w[0], n);
		return w;
	}

	std::string urlEncode(const std::string& s)
	{
		std::string out;
		for (unsigned char c : s)
		{
			if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') out += (char)c;
			else { char b[4]; snprintf(b, sizeof b, "%%%02X", c); out += b; }
		}
		return out;
	}

	// A quoted Lucene phrase for a MusicBrainz search.
	std::string phrase(const std::string& s)
	{
		std::string out = "\"";
		for (char c : s) { if (c == '"' || c == '\\') out += '\\'; out += c; }
		return out + "\"";
	}

	// Plain HTTPS request. Follows redirects; returns the status code and the final URL.
	int request(const std::wstring& host, const std::wstring& path, bool head, std::string* body, std::wstring* finalUrl)
	{
		int status = 0;
		HINTERNET s = WinHttpOpen(L"LossCat/0.1 ( https://github.com/segcat/LossCat )", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
		if (!s) s = WinHttpOpen(L"LossCat/0.1 ( https://github.com/segcat/LossCat )", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
		if (!s) return 0;
		WinHttpSetTimeouts(s, 4000, 4000, 4000, 4000);
		HINTERNET c = WinHttpConnect(s, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
		HINTERNET r = c ? WinHttpOpenRequest(c, head ? L"HEAD" : L"GET", path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE) : nullptr;
		if (r && WinHttpSendRequest(r, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(r, nullptr))
		{
			DWORD code = 0, size = sizeof code;
			WinHttpQueryHeaders(r, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &code, &size, WINHTTP_NO_HEADER_INDEX);
			status = (int)code;
			if (finalUrl)
			{
				DWORD len = 0;
				WinHttpQueryOption(r, WINHTTP_OPTION_URL, nullptr, &len);
				if (len)
				{
					std::wstring u(len / sizeof(wchar_t), L'\0');
					if (WinHttpQueryOption(r, WINHTTP_OPTION_URL, &u[0], &len)) *finalUrl = u.c_str();
				}
			}
			while (body)
			{
				DWORD avail = 0;
				if (!WinHttpQueryDataAvailable(r, &avail) || !avail || body->size() > (1 << 20)) break;
				std::string chunk(avail, '\0');
				DWORD got = 0;
				if (!WinHttpReadData(r, &chunk[0], avail, &got) || !got) break;
				body->append(chunk, 0, got);
			}
		}
		if (r) WinHttpCloseHandle(r);
		if (c) WinHttpCloseHandle(c);
		WinHttpCloseHandle(s);
		return status;
	}

	std::string narrowAscii(const std::wstring& w)
	{
		std::string s;
		for (wchar_t c : w) s += c < 0x80 ? (char)c : '?';
		return s;
	}
}

// Looks the album up on MusicBrainz and returns a public Cover Art Archive URL for it,
// since Discord can only show images it can download, not files on this PC.
std::string DiscordPresence::findCover(const std::string& artist, const std::string& album)
{
	std::string q = "releasegroup:" + phrase(album) + (artist.empty() ? "" : " AND artist:" + phrase(artist));
	std::string body;
	if (request(L"musicbrainz.org", wide("/ws/2/release-group/?fmt=json&limit=1&query=" + urlEncode(q)), false, &body, nullptr) != 200) return "";
	size_t list = body.find("\"release-groups\":[");
	if (list == std::string::npos) return "";
	size_t idAt = body.find("\"id\":\"", list);
	size_t scoreAt = body.find("\"score\":", list);
	if (idAt == std::string::npos || scoreAt == std::string::npos) return "";
	if (atoi(body.c_str() + scoreAt + 8) < 90) return "";   // not confident it's the same album
	idAt += 6;
	std::string id = body.substr(idAt, body.find('"', idAt) - idAt);
	if (id.size() != 36) return "";

	std::wstring url;
	if (request(L"coverartarchive.org", wide("/release-group/" + id + "/front-500"), true, nullptr, &url) != 200 || url.empty()) return "";
	return narrowAscii(url);
}

DiscordPresence::DiscordPresence(std::string id) : clientId(std::move(id))
{
	worker = std::thread([this] { run(); });
}

DiscordPresence::~DiscordPresence()
{
	{
		std::lock_guard<std::mutex> lock(m);
		stop = true;
	}
	cv.notify_all();
	if (worker.joinable()) worker.join();
	disconnect();
}

void DiscordPresence::set(const Activity& a)
{
	{
		std::lock_guard<std::mutex> lock(m);
		want = a;
		has = true;
		dirty = true;
	}
	cv.notify_all();
}

void DiscordPresence::clear()
{
	{
		std::lock_guard<std::mutex> lock(m);
		has = false;
		dirty = true;
	}
	cv.notify_all();
}

void DiscordPresence::run()
{
	std::unique_lock<std::mutex> lock(m);
	while (!stop)
	{
		cv.wait(lock, [this] { return stop || dirty; });
		if (stop) break;

		if (!pipe)
		{
			lock.unlock();
			bool ok = connect();
			lock.lock();
			if (!ok)
			{
				// Discord isn't running; try again later with whatever is current then.
				cv.wait_for(lock, std::chrono::seconds(15), [this] { return stop; });
				continue;
			}
		}

		std::string image = "losscat";
		if (has && !want.album.empty())
		{
			std::string key = want.artist + "\x1F" + want.album;
			auto it = coverUrls.find(key);
			if (it == coverUrls.end())
			{
				std::string artist = want.artist, album = want.album;
				lock.unlock();
				std::string url = findCover(artist, album);
				lock.lock();
				it = coverUrls.emplace(key, url).first;
			}
			if (!it->second.empty()) image = it->second;
		}
		std::string json = activityJson(image);
		dirty = false;
		lock.unlock();
		unsigned op = 0;
		std::string reply;
		bool ok = send(1, json) && readFrame(op, reply, 3000) && op == 1;
		if (!ok) disconnect();
		lock.lock();
		if (!ok) dirty = true;

		// Discord rate-limits presence updates; changes made meanwhile are merged into the next one.
		cv.wait_for(lock, std::chrono::seconds(2), [this] { return stop; });
	}
}

std::string DiscordPresence::activityJson(const std::string& image)
{
	std::string s = "{\"cmd\":\"SET_ACTIVITY\",\"args\":{\"pid\":" + std::to_string(GetCurrentProcessId());
	if (has)
	{
		s += ",\"activity\":{\"type\":2,\"status_display_type\":2";
		s += ",\"details\":" + field(want.title);
		s += ",\"state\":" + field(want.artist);
		s += ",\"assets\":{\"large_image\":\"" + escape(image) + "\"";
		if (!want.album.empty()) s += ",\"large_text\":" + field(want.album);
		s += "}";
		if (want.startMs && want.endMs)
			s += ",\"timestamps\":{\"start\":" + std::to_string(want.startMs) + ",\"end\":" + std::to_string(want.endMs) + "}";
		s += "}";
	}
	s += "},\"nonce\":\"" + std::to_string(++nonce) + "\"}";
	return s;
}

bool DiscordPresence::connect()
{
	for (int i = 0; i < 10; ++i)
	{
		std::wstring name = L"\\\\.\\pipe\\discord-ipc-" + std::to_wstring(i);
		HANDLE h = CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
		if (h == INVALID_HANDLE_VALUE) continue;
		pipe = h;
		unsigned op = 0;
		std::string reply;
		if (send(0, "{\"v\":1,\"client_id\":\"" + escape(clientId) + "\"}") && readFrame(op, reply, 3000) && op == 1)
			return true;
		disconnect();
	}
	return false;
}

void DiscordPresence::disconnect()
{
	if (pipe) CloseHandle((HANDLE)pipe);
	pipe = nullptr;
}

bool DiscordPresence::send(unsigned op, const std::string& json)
{
	if (!pipe) return false;
	std::string frame(8, '\0');
	unsigned len = (unsigned)json.size();
	memcpy(&frame[0], &op, 4);
	memcpy(&frame[4], &len, 4);
	frame += json;
	DWORD written = 0;
	return WriteFile((HANDLE)pipe, frame.data(), (DWORD)frame.size(), &written, nullptr) && written == frame.size();
}

bool DiscordPresence::readFrame(unsigned& op, std::string& body, int timeoutMs)
{
	// Poll instead of a blocking read, so a Discord that never answers can't hang the thread.
	auto readExact = [&](void* dst, DWORD n) {
		DWORD got = 0;
		int waited = 0;
		while (got < n)
		{
			DWORD avail = 0;
			if (!PeekNamedPipe((HANDLE)pipe, nullptr, 0, nullptr, &avail, nullptr)) return false;
			if (!avail)
			{
				if (waited >= timeoutMs) return false;
				Sleep(10);
				waited += 10;
				continue;
			}
			DWORD r = 0;
			if (!ReadFile((HANDLE)pipe, (char*)dst + got, n - got < avail ? n - got : avail, &r, nullptr)) return false;
			got += r;
		}
		return true;
	};
	unsigned header[2];
	if (!pipe || !readExact(header, 8)) return false;
	op = header[0];
	if (header[1] > 1 << 20) return false;
	body.assign(header[1], '\0');
	return header[1] == 0 || readExact(&body[0], header[1]);
}
