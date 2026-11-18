#pragma once
#include "gfx.h"
#include "player.h"
#include "trackinfo.h"
#include "library.h"
#include "playlists.h"
#include <string>
#include <vector>
#include <random>
#include <thread>
#include <atomic>
#include <map>

// Everything that can be clicked. Drawing registers a rectangle for each one every
// frame; mouse handling and WM_NCHITTEST look them up from the last frame.
enum class Hit
{
	None,
	NavToggle, NavNow, NavHome, NavLibrary,
	Settings, WinMin, WinMax, WinClose,
	TabNext, TabAbout, TabLyrics,
	Cover, Shuffle, Prev, Play, Next, Repeat, Seek, Volume, QueueButton,
	OpenFiles, SignalPath,
	Panel,           // background that swallows clicks (not a drag area)
	QueueRow, QueueRemove, QueueClear, QueueUndo,
	SortHeader, SortMenu,
	SearchField, SearchClear, SearchArtist, SearchAlbum, SearchRow,
	LibraryRow,
	OpenLibraryFolder, ChangeLibraryFolder, AddMusicFolder, RemoveMusicFolder, OpenMusicFolder, ShowHidden,
	LibTabAlbums, LibTabArtists, LibTabSongs,
	AlbumTile,
	ArtistTile,
	RecentTile,
	AddedTile,
	ImportFiles, Rescan, AddToLibrary,
	MenuItem,
	PlayAll, ShuffleAll,
	ConfirmOk, ConfirmCancel,
	NewPlaylist, PlaylistNav, PlaylistRow,
	PlaylistPlay, PlaylistShuffle, PlaylistRename, PlaylistDelete,
	Favourite, EditBox,
	BarFavourite, BarAddTo, LibTabPlaylists, PlaylistTile,
	RowHeart,
	Link,            // + index into links
	Back,
	CancelImport,
	TilePlay,
	ModeShared, ModeExclusive, ModeAsio, DeviceMenu, ModeMenu, BarDevice,
	NowAlbumRow, GoToAlbum, NowMore,
	SmartNav, SmartRow,
	LyricLine, LyricScroll, LyricCopy, LyricExport, LyricPaste, LyricFile, LyricRemove,        // play button on a hovered album tile
	GroupBack, GroupPlay, GroupShuffle, AlbumRow, AlbumCover, AlbumArtist, ArtistAlbum,
};

enum class Page { Home, Now, Library, Settings, Playlist, License, Album, Artist, Smart, Search };

struct Track
{
	std::wstring path;
	TrackInfo info;
	std::wstring title, artist, album;
	std::vector<unsigned char> coverBytes;
	bool coverLoaded = false;
	unsigned long long bytes = 0;   // file size, for the totals in Settings and About album
	bool queued = false;            // added with Play next / Add to queue
	unsigned long long added = 0;   // when the file arrived (its creation time), for "Date added"
};

std::wstring fmtSize(unsigned long long bytes);   // "812 MB", "2.4 GB"
double nowMs();   // high-resolution clock for animations
std::string fsPath(const std::wstring& path);

struct Group
{
	std::wstring name, artist;
	std::vector<int> tracks;
};

class App
{
public:
	bool create(HINSTANCE inst, int show);
	void addFiles(const std::vector<std::wstring>& files, bool playFirst);
	void loadLibrary(bool background = false);
	void finishScan();
	void addMusicFolder();
	std::wstring pickFolder(const wchar_t* title);
	void changeLibraryFolder();
	void startLibraryMove();
	void finishLibraryMove();
	bool isImported(const std::wstring& path) const;   // a copy in LossCat's own folder, not a song from a scanned folder
	void refreshLibrary();
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
	void drawLibrary(const Rect& area);
	struct Tile { std::wstring title, sub, coverPath; bool playing = false; bool heart = false; std::vector<std::wstring> mosaic; bool sparkle = false; };
	float drawTiles(const std::vector<Tile>& tiles, float x, float y, float w, Hit hit, bool round);
	void hotVisible(const Rect& r, Hit id, int index);
	ID2D1Bitmap* coverOf(const std::wstring& path);
	void drawNowPlaying(const Rect& area);
	void drawSettings(const Rect& area);
	void drawLicense(const Rect& area);
	void openLicense(int resourceId, const std::wstring& title);
	void drawPlayerBar();
	void drawQueuePanel();
	void drawMenu();
	void drawPlaylist(const Rect& area);
	void drawLyrics(const Track& tr, const Rect& body);
	void drawAlbum(const Rect& area);
	// Smart playlist "Lossless 24-bit": library songs that are lossless with 24-bit or more. Built on the fly.
	std::vector<int> smartTracks() const;
	void drawSmart(const Rect& area);
	int tileToPlaylist(int tile) const { return tile == 0 ? 0 : tile == 1 ? -2 : tile - 1; }
	void drawArtist(const Rect& area);
	void openAlbum(int album);
	void openArtist(int artist);
	int findAlbum() const;
	int findArtist() const;
	void drawSidebarPlaylists(float y);
	void drawEditBox(const Rect& r, float size, Font f);
	void drawConfirm();
	void drawEmpty(const Rect& area, const wchar_t* heading, const wchar_t* body, bool button);
	void drawTrackRows(const Rect& area, const std::vector<Track>& list, Hit rowHit, bool sortable = false);
	// A playlist's picture: its first album cover, or four of them in a grid; an icon when it has none.
	void drawMosaic(const Rect& r, const std::vector<Track>& list, const char* icon);

	// ---- sorting the Songs tab
	enum SortKey { SortDefault, SortTitle, SortArtist, SortAlbum, SortFormat, SortTime, SortAdded };
	int songSort = SortDefault;
	bool songSortDesc = false;
	std::vector<Track> songsView;          // libTracks in the chosen order; what the Songs tab shows and plays
	long long songsViewKey = -1;
	const std::vector<Track>& sortedSongs();
	void setSort(int key);                 // a header click: new column ascending (newest first for dates), same column flips
	Rect sortButton;

	// ---- search: the field at the top of the main panel, results on Page::Search
	std::wstring query;
	bool searchFocused = false, swallowSlash = false;
	Page searchFrom = Page::Home;          // where to go back to when the search is cleared
	std::vector<int> searchArtists, searchAlbums, searchSongIdx;   // indices into artists / albums / libTracks
	std::vector<Track> searchSongs;
	std::wstring searchedFor;              // the query the results above belong to
	long long searchedVer = -1;
	void runSearch();
	void setQuery(const std::wstring& q);
	void drawSearchField();
	void drawSearch(const Rect& area);
	Rect gearRect;
	void hot(const Rect& r, Hit id, int index = -1);
	bool hovered(Hit id, int index = -1) const { return hoverHit == id && (index < 0 || hoverIndex == index); }
	bool pressed(Hit id, int index = -1) const { return pressHit == id && hovered(id, index); }
	// Eases 0..1 toward `on` over ~120 ms, per control; keeps frames coming while it moves.
	float anim(Hit id, int index, bool on);
	struct AnimState { float value = 0; double t = 0; };
	std::map<long long, AnimState> anims;
	mutable bool animMoving = false;
	Hit hitAt(float x, float y, int* index = nullptr) const;

	// actions
	void click(Hit id, int index);
	void playIndex(int i);
	void rebuildOrder();
	void ensureOrder();
	std::vector<int> upcoming() const;
	void enqueue(const std::vector<std::wstring>& paths, bool playNext);   // after the current song, or at the end

	// ---- editing the queue: drag to reorder, remove with undo
	void moveQueued(int qi, int slot);              // slot: position among the songs up next (0 = next)
	void removeQueued(std::vector<int> qis, const std::wstring& what);
	void undoQueue();
	int queueSlotAt(float y) const;
	struct QRow { int qi; int up; int list; Rect r; };   // up: position in upcoming(); list 1 = panel, 2 = Up next tab
	std::vector<QRow> qRows;                        // rows drawn this frame
	int qDragItem = -1, qDragList = 0, qDropSlot = -1;
	bool qDragging = false;
	float qPressY = 0;
	float queueScroll = 0, queueMaxScroll = 0;
	Rect queueList;
	struct QueueSnap { std::vector<Track> queue; std::vector<int> order; int current = -1; std::wstring path; };
	QueueSnap undoSnap;
	bool hasUndo = false;
	std::wstring toastText;
	void drawToast();
	void drawDragGhost();
	void playFromLibrary(int i);
	void playList(const std::vector<int>& libIndices, int start);
	std::vector<std::wstring> pickFiles();
	void importFiles(const std::vector<std::wstring>& files);
	void finishImport();
	void setAlbumCover(const std::vector<std::wstring>& paths);
	void finishCover();
	const Group* albumOf(const std::wstring& path) const;
	bool busyImporting();
	bool inLibrary(const std::wstring& path) const;
	void openMenu(Hit target, int index, float x, float y);
	void removeFromLibrary(const std::vector<std::wstring>& paths);
	std::wstring absOf(const std::string& rel) const;
	std::string relOf(const std::wstring& abs) const;
	bool inPlaylist(int p, const std::wstring& abs) const;
	const std::vector<Track>& playlistTracks(int p);
	void addToPlaylist(int p, const std::vector<std::wstring>& paths);
	void toggleFavourite(const std::wstring& abs);
	int newPlaylist();
	void startRename(int p);
	void commitRename();
	void playPlaylist(int p, int start, bool shuffled);
	void openPlaylistChooser(const std::vector<std::wstring>& paths, float x, float y);
	void fillChooser();
	bool ensureInLibrary(std::wstring& path);
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

	Library lib;
	std::wstring libraryDir;
	std::vector<Track> libTracks;
	std::vector<Group> albums, artists;
	std::vector<int> recentlyAdded;   // albums, newest first
	std::vector<std::wstring> recentPlayed;
	struct CachedCover { ComPtr<ID2D1Bitmap> bmp; unsigned long long lastUsed = 0; };
	std::unordered_map<std::wstring, CachedCover> covers;
	unsigned long long frameNo = 0;
	void trimCovers();
	unsigned coversGen = 0;
	int libTab = 0;
	float pageScroll = 0, pageMaxScroll = 0;
	Rect pageArea;
	std::wstring notice;

	enum class Act { Play, AddToLibrary, Remove, SetCover, Favourite, Unfavourite, PlaylistMenu, AddTo, NewWith, RemoveFromPlaylist, PlayPlaylist, RenamePlaylist, DeletePlaylist, PickDevice, PickMode, PlayNext, AddToQueue, PlayNow, MoveNext, MoveEnd, RemoveQueued, ClearUpNext, PickSort };
	enum class Look { Item, Heading, Action, Separator };   // Action: icon, bold, line under it
	struct MenuEntry { std::wstring label; Act act; bool danger; int arg = -1; Look look = Look::Item; bool checked = false; };
	bool menuOpen = false;
	// Name shown beside a hovered playlist when the sidebar is collapsed.
	std::wstring tipText;
	float tipX = 0, tipY = 0;
	void drawTip();
	float menuX = 0, menuY = 0;
	bool menuUp = false, menuLeft = false;   // open above / to the left of the anchor
	std::vector<MenuEntry> menu;
	std::vector<std::wstring> menuPaths;
	bool confirmOpen = false;
	std::wstring confirmTitle, confirmBody;   // body: overrides the default text when set
	std::vector<std::wstring> pendingRemove;
	int pendingDeletePlaylist = -1;
	bool pendingRemoveLyrics = false;

	// Lyrics tab: lines as shown, a line selection to copy, and its own scroll.
	std::map<std::wstring, std::wstring> lyricsByPath;   // what's in each file's tag, kept fresh after edits
	std::wstring lyricsPath;
	std::vector<std::wstring> lyricLines;
	std::vector<float> lyricTops;
	Rect lyricArea;
	float lyricScroll = 0, lyricMaxScroll = 0, lyricThumbH = 0;
	int lyricSelA = -1, lyricSelB = -1;
	bool selectingLyrics = false;
	const std::wstring& lyricsFor(const Track& t);
	int lyricLineAt(float y) const;
	void copyLyrics();
	void saveLyrics(const std::wstring& text);
	void pasteLyrics();
	void loadLyricsFile();
	void exportLyrics();

	// Volume, queue, history and a few view settings, kept in %APPDATA%\LossCat\state.txt.
	std::wstring statePath() const;
	void saveState();
	void loadState();
	double lastSaveMs = 0;
	int menuPlaylist = -1, menuPosition = -1;   // right-click on a playlist row

	Playlists pls;
	Page lastPage = Page::Home;
	int openPlaylist = -1;
	// Album and artist pages hold names rather than indices, which shift when the library changes.
	std::wstring albumName, albumArtist, artistName;
	bool albumFromArtist = false;
	std::vector<int> artistAlbums;
	int nowAlbum = -1;
	Rect nowAlbumArea;
	float nowAlbumScroll = 0, nowAlbumMaxScroll = 0;
	std::wstring nowAlbumFor;
	// Text too long for its space slides back and forth instead of being cut off.
	void marquee(const std::wstring& s, const Rect& r, float size, Font f, uint32_t rgb);
	bool marqueeActive = false, marqueeTimer = false;
	double marqueeStart = 0;
	float npCoverBottom = 0;   // Now playing: where the tab separator was last frame, so the cover can end there   // album of the song on Now playing, for its About album tab
	std::vector<Track> plTracks;
	int plTracksFor = -1;
	unsigned plVersion = 1, plTracksVersion = 0;
	int editPlaylist = -1;
	std::wstring editText;
	float sideScroll = 0, sideMaxScroll = 0;
	Rect sideList;
	std::vector<std::wstring> rowHeartPaths;
	std::vector<std::wstring> links;   // URLs of the Link hits drawn this frame (or "license:<id>")
	std::wstring licenseTitle, licenseText;

	// Imports of more than one file run on a worker thread. While it runs, only the
	// worker touches lib; the GUI keeps drawing from its own copies (libTracks).
	struct ImportResult { std::wstring source; std::string copy; std::string error; };
	std::thread importThread;
	std::atomic<bool> importing{ false }, importCancel{ false };
	std::atomic<int> importDone{ 0 };
	int importTotal = 0;
	std::vector<ImportResult> importResults;
	// Music folders scanned in place (nothing copied), and songs hidden from them.
	std::vector<std::wstring> musicFolders, hiddenFiles;
	// Rescans run on a worker thread into a fresh Library that replaces lib when done.
	std::thread scanThread;
	std::atomic<bool> scanning{ false };
	std::atomic<int> scanCount{ 0 };
	Library scanned;
	// The LossCat folder can be moved; empty means the default, Music\LossCat.
	std::wstring customLibraryDir, pendingLibraryDir;
	std::thread moveThread;
	std::atomic<bool> moving{ false };
	std::atomic<int> moveDone{ 0 }, moveTotal{ 0 }, moveFailed{ 0 };
	std::wstring moveFrom, moveTo;
	double moveResumeAt = 0;
	bool moveResume = false;
	std::wstring moveResumePath;
	std::wstring cachePath() const;
	// Writing a cover into every file of an album also runs on a worker thread.
	std::thread coverThread;
	std::atomic<bool> coverBusy{ false };
	std::vector<std::wstring> coverPaths;
	std::atomic<int> coverFailed{ 0 };
	std::string coverError;
	bool resumeAfterCover = false;
	double resumeAt = 0;
	std::wstring resumePath;   // song table hearts this frame, by row index
	float listScroll = 0;
	float listMaxScroll = 0;
	Rect listArea;

	std::vector<Track> queue;
	std::vector<int> order;
	int orderPos = -1;
	std::mt19937 rng{ std::random_device{}() };
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

	// Smooth scrolling: the wheel moves a target, and each frame eases the real value toward it.
	struct Smooth { float* value; float target; float last; };
	std::vector<Smooth> smooths;
	double lastScrollFrame = 0;

	// WASAPI exclusive mode (bit-perfect output), saved in state.txt.
	int audioMode = 0;          // 0 shared, 1 WASAPI exclusive, 2 ASIO
	std::wstring asioDriver;    // chosen ASIO driver; empty = the first installed
	void setAudioMode(int mode);
	std::wstring outputDevice;   // chosen by name; empty = system default
	std::vector<std::string> deviceNames;
	Rect deviceButton, modeButton, barDeviceButton;
	void openDeviceMenu(float x, float y);
	void pickDevice(const std::wstring& name);
	void reopenAudio();
	bool scrollMoving = false;
	void wheelScroll(float& value, float max, float delta, bool precise);
	void stepScroll();
	UINT timerMs = 0;

	// layout, in DIPs
	float W = 0, H = 0, scale = 1;
	float navGeo = 0, labelAlpha = 1, queueVis = 0;
	std::wstring error;
	Rect sidebar, mainPanel, queuePanel, playerBar, seekBar, volumeBar, nowMoreAt;
};
