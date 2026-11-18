#pragma once
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <map>

// Discord Rich Presence over Discord's local IPC pipe. Everything is sent from a worker
// thread, so a slow or missing Discord never stalls the UI. Closing the connection
// (destroying this object) makes Discord drop the status.
class DiscordPresence
{
public:
	struct Activity
	{
		std::string title, artist, album;   // UTF-8
		long long startMs = 0, endMs = 0;   // unix time in ms; both 0 while paused
	};

	explicit DiscordPresence(std::string clientId);
	~DiscordPresence();
	void set(const Activity& a);
	void clear();

private:
	void run();
	bool connect();
	void disconnect();
	bool send(unsigned op, const std::string& json);
	bool readFrame(unsigned& op, std::string& body, int timeoutMs);
	std::string activityJson(const std::string& image);
	std::string findCover(const std::string& artist, const std::string& album);

	std::string clientId;
	void* pipe = nullptr;
	std::thread worker;
	std::mutex m;
	std::condition_variable cv;
	bool stop = false, dirty = false, has = false;
	Activity want;
	int nonce = 0;
	std::map<std::string, std::string> coverUrls;   // "artist\x1Falbum" -> image URL, empty if none was found
};
