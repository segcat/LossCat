#pragma once
#include "gfx.h"
#include "player.h"
#include "trackinfo.h"
#include <string>
#include <vector>

// Everything that can be clicked. Drawing registers a rectangle for each one every
// frame; mouse handling and WM_NCHITTEST look them up from the last frame.
enum class Hit
{
	None,
	NavToggle, NavNow, NavHome,
	Settings, WinMin, WinMax, WinClose,
	TabNext, TabAbout, TabLyrics,
	Cover, Shuffle, Prev, Play, Next, Repeat, Seek, Volume, QueueButton,
	OpenFiles, SignalPath,
	Panel,           // background that swallows clicks (not a drag area)
	QueueRow,        // + index in hotIndex
};

enum class Page { Home, Now, Settings };

struct Track
{
	std::wstring path;
	TrackInfo info;
	std::wstring title, artist, album;
	std::vector<unsigned char> coverBytes;
};

double nowMs();   // high-resolution clock for animations

class App
{
public:
	bool create(HINSTANCE inst, int show);
	void addFiles(const std::vector<std::wstring>& files, bool playFirst);
	static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

private:
	LRESULT handle(UINT msg, WPARAM wp, LPARAM lp);

	// window frame
	LRESULT hitTestFrame(POINT screen);
	void toClient(POINT screen, float& x, float& y);

	// drawing (ui.cpp)
	void paint();
	void layout();
	void drawSidebar();
	void drawMain();
	void drawWindowControls(const Rect& panel);
	void drawHome(const Rect& area);
	void drawNowPlaying(const Rect& area);
	void drawSettings(const Rect& area);
	void drawPlayerBar();
	void drawQueuePanel();
	void drawEmpty(const Rect& area, const wchar_t* heading, const wchar_t* body, bool button);
	void drawTrackRows(const Rect& area, int first, bool compact);
	void hot(const Rect& r, Hit id, int index = -1);
	bool hovered(Hit id, int index = -1) const { return hoverHit == id && (index < 0 || hoverIndex == index); }
	Hit hitAt(float x, float y, int* index = nullptr) const;

	// actions
	void click(Hit id, int index);
	void playIndex(int i);
	void next(bool userAction);
	void prev();
	void togglePlay();
	void seekTo(double seconds);
	void openFileDialog();
	void tick();
	void updateTimer();
	bool animating() const;
	void animValues(float& navGeo, float& labelAlpha, float& queueOpenT) const;
	ID2D1Bitmap* currentCover();
	std::wstring signalSource() const;
	std::wstring signalOutput() const;

	HWND hwnd = nullptr;
	Gfx g;
	Player player;
	bool audioOk = false;

	std::vector<Track> queue;
	int current = -1;
	ComPtr<ID2D1Bitmap> cover;
	int coverFor = -1;
	unsigned coverGen = 0;

	Page page = Page::Home;
	int npTab = 0;
	bool queueOpen = false;
	bool shuffle = false;
	int repeat = 0;            // 0 off, 1 all, 2 one
	float volume = 0.8f;

	// sidebar collapse animation: navT goes 0 (open) -> 1 (collapsed)
	bool collapsed = false;
	float navT = 0, navFrom = 0;
	double navStart = 0;     // ms, from nowMs(); 0 = never
	float queueT = 0, queueFrom = 0;
	double queueStart = 0;

	// input
	struct Hot { Rect r; Hit id; int index; };
	std::vector<Hot> hots;
	Hit hoverHit = Hit::None; int hoverIndex = -1;
	Hit pressHit = Hit::None; int pressIndex = -1;
	bool dragging = false;
	double dragSeconds = 0;
	float mouseX = -1, mouseY = -1;
	bool tracking = false;
	UINT timerMs = 0;

	// layout, in DIPs
	float W = 0, H = 0, scale = 1;
	float navGeo = 0, labelAlpha = 1, queueVis = 0;
	std::wstring error;
	Rect sidebar, mainPanel, queuePanel, playerBar, seekBar, volumeBar;
};
