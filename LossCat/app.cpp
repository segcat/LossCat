#include "app.h"
#include "resource.h"
#include "names.h"
#include <windowsx.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <knownfolders.h>
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <cwctype>
#include <map>
#include <filesystem>
#include <algorithm>

#pragma comment(lib, "dwmapi")
#pragma comment(lib, "shell32")
#pragma comment(lib, "comdlg32")
#pragma comment(lib, "ole32")

// Not in older SDK headers.
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#define LC_DWMWA_WINDOW_CORNER_PREFERENCE 33
#define LC_DWMWA_BORDER_COLOR 34
#define LC_DWMWCP_ROUND 2
#define WM_IMPORT_DONE (WM_APP + 1)
#define WM_COVER_DONE (WM_APP + 2)
#define WM_SCAN_DONE (WM_APP + 3)
#define WM_MOVE_DONE (WM_APP + 4)

static const wchar_t* kClassName = L"LossCat";
static const UINT_PTR kTimer = 1;


std::string fsPath(const std::wstring& path)
{
	return narrow(path);   // Core takes UTF-8 paths and opens them with the Unicode API
}

static std::wstring fileStem(const std::wstring& path)
{
	size_t slash = path.find_last_of(L"\\/");
	std::wstring name = slash == std::wstring::npos ? path : path.substr(slash + 1);
	size_t dot = name.find_last_of(L'.');
	return dot == std::wstring::npos ? name : name.substr(0, dot);
}

static std::wstring extUpper(const std::wstring& path)
{
	size_t dot = path.find_last_of(L'.');
	std::wstring e = dot == std::wstring::npos ? L"" : path.substr(dot + 1);
	for (auto& c : e) c = (wchar_t)towupper(c);
	return e;
}

static bool canPlay(const std::wstring& path)
{
	std::wstring e = extUpper(path);
	return e == L"FLAC" || e == L"WAV" || e == L"MP3" || e == L"AIF" || e == L"AIFF";
}

static Track makeTrack(const std::wstring& path, TrackInfo info, bool keepCover)
{
	Track t;
	t.path = path;
	t.title = info.title.empty() ? fileStem(path) : widen(info.title);
	t.artist = info.artist.empty() ? L"Unknown artist" : widen(info.artist);
	t.album = widen(info.album);
	if (keepCover)
	{
		t.coverBytes = std::move(info.coverData);
		t.coverLoaded = true;
	}
	info.coverData.clear();
	info.coverData.shrink_to_fit();
	t.info = std::move(info);
	return t;
}

bool App::create(HINSTANCE inst, int show)
{
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

	WNDCLASSEXW wc = { sizeof(wc) };
	wc.lpfnWndProc = WndProc;
	wc.hInstance = inst;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.lpszClassName = kClassName;
	wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(IDI_APPICON));
	wc.hIconSm = (HICON)LoadImageW(inst, MAKEINTRESOURCEW(IDI_APPICON), IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	RegisterClassExW(&wc);

	hwnd = CreateWindowExW(0, kClassName, L"LossCat", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 1280, 800, nullptr, nullptr, inst, this);
	if (!hwnd) return false;

	scale = GetDpiForWindow(hwnd) / 96.0f;
	HMONITOR mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi = { sizeof(mi) };
	GetMonitorInfoW(mon, &mi);
	int w = (int)(1280 * scale), h = (int)(800 * scale);
	int areaW = mi.rcWork.right - mi.rcWork.left, areaH = mi.rcWork.bottom - mi.rcWork.top;
	if (w > areaW) w = areaW;
	if (h > areaH) h = areaH;
	SetWindowPos(hwnd, nullptr, mi.rcWork.left + (areaW - w) / 2, mi.rcWork.top + (areaH - h) / 2, w, h,
		SWP_NOZORDER | SWP_FRAMECHANGED);

	BOOL dark = TRUE;
	DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
	DWORD corner = LC_DWMWCP_ROUND;
	DwmSetWindowAttribute(hwnd, LC_DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));
	COLORREF border = RGB(0x2A, 0x2A, 0x2A);
	DwmSetWindowAttribute(hwnd, LC_DWMWA_BORDER_COLOR, &border, sizeof(border));
	MARGINS margins = { 0, 0, 1, 0 };
	DwmExtendFrameIntoClientArea(hwnd, &margins);

	if (!g.init(hwnd)) return false;
	DragAcceptFiles(hwnd, TRUE);

	audioOk = player.init(0, 0);   // device's native channels and rate
	if (audioOk) player.setVolume(volume);
	else error = L"No audio output device";

	for (float* v : { &pageScroll, &listScroll, &sideScroll, &lyricScroll, &nowAlbumScroll })
		smooths.push_back({ v, *v, *v });
	loadState();   // also opens the library

	ShowWindow(hwnd, show);
	return true;
}

LRESULT CALLBACK App::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	App* app = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
	if (msg == WM_NCCREATE)
	{
		app = static_cast<App*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
		app->hwnd = hwnd;
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
	}
	if (app) return app->handle(msg, wp, lp);
	return DefWindowProcW(hwnd, msg, wp, lp);
}

void App::toClient(POINT pt, float& x, float& y)
{
	ScreenToClient(hwnd, &pt);
	x = pt.x / scale;
	y = pt.y / scale;
}

LRESULT App::hitTestFrame(POINT screen)
{
	float x, y;
	toClient(screen, x, y);

	if (!IsZoomed(hwnd))
	{
		const float b = 6;
		bool left = x < b, right = x >= W - b, top = y < b, bottom = y >= H - b;
		if (top && left) return HTTOPLEFT;
		if (top && right) return HTTOPRIGHT;
		if (bottom && left) return HTBOTTOMLEFT;
		if (bottom && right) return HTBOTTOMRIGHT;
		if (left) return HTLEFT;
		if (right) return HTRIGHT;
		if (top) return HTTOP;
		if (bottom) return HTBOTTOM;
	}

	Hit h = hitAt(x, y);
	if (h == Hit::WinMax) return HTMAXBUTTON;   // lets Windows 11 show Snap Layouts
	if (h != Hit::None) return HTCLIENT;

	// Empty space along the top is the title bar.
	if (y < 8) return HTCAPTION;
	if (mainPanel.contains(x, y) && y < mainPanel.y + 56) return HTCAPTION;
	if (sidebar.contains(x, y) && y < sidebar.y + 20) return HTCAPTION;
	if (queuePanel.contains(x, y) && y < queuePanel.y + 56) return HTCAPTION;
	return HTCLIENT;
}

LRESULT App::handle(UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_NCCALCSIZE:
		if (wp)
		{
			// The whole window is client area. When maximised, Windows positions the window
			// past the screen edge by the frame width, so pull the client area back in.
			if (IsZoomed(hwnd))
			{
				auto* p = reinterpret_cast<NCCALCSIZE_PARAMS*>(lp);
				UINT dpi = GetDpiForWindow(hwnd);
				int f = GetSystemMetricsForDpi(SM_CXFRAME, dpi) + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
				p->rgrc[0].left += f;
				p->rgrc[0].right -= f;
				p->rgrc[0].top += f;
				p->rgrc[0].bottom -= f;
			}
			return 0;
		}
		break;

	case WM_NCHITTEST:
		return hitTestFrame({ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) });

	// The maximise dot is reported as HTMAXBUTTON, so its clicks arrive as non-client messages.
	case WM_NCLBUTTONDOWN:
		if (wp == HTMAXBUTTON) { pressHit = Hit::WinMax; InvalidateRect(hwnd, nullptr, FALSE); return 0; }
		break;
	case WM_NCLBUTTONUP:
		if (wp == HTMAXBUTTON)
		{
			if (pressHit == Hit::WinMax) ShowWindow(hwnd, IsZoomed(hwnd) ? SW_RESTORE : SW_MAXIMIZE);
			pressHit = Hit::None;
			return 0;
		}
		break;
	case WM_NCMOUSEMOVE:
	{
		float x, y;
		toClient({ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) }, x, y);
		int idx = -1;
		Hit h = wp == HTMAXBUTTON ? Hit::WinMax : hitAt(x, y, &idx);
		if (h != hoverHit) { hoverHit = h; hoverIndex = idx; InvalidateRect(hwnd, nullptr, FALSE); }
		TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE | TME_NONCLIENT, hwnd, 0 };
		TrackMouseEvent(&tme);
		mouseX = x; mouseY = y;
		break;
	}
	case WM_NCMOUSELEAVE:
	case WM_MOUSELEAVE:
	{
		// Moving between the client area and the maximise dot (non-client) sends a leave
		// for one and a move for the other, so look at where the cursor really is.
		tracking = false;
		if (dragging) return 0;
		POINT pt;
		GetCursorPos(&pt);
		Hit h = Hit::None;
		int idx = -1;
		if (WindowFromPoint(pt) == hwnd)
		{
			toClient(pt, mouseX, mouseY);
			h = hitAt(mouseX, mouseY, &idx);
		}
		else
		{
			mouseX = mouseY = -1;
		}
		if (h != hoverHit || idx != hoverIndex)
		{
			hoverHit = h; hoverIndex = idx;
			InvalidateRect(hwnd, nullptr, FALSE);
		}
		return 0;
	}

	case WM_MOUSEMOVE:
	{
		float x = GET_X_LPARAM(lp) / scale, y = GET_Y_LPARAM(lp) / scale;
		mouseX = x; mouseY = y;
		if (!tracking)
		{
			TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
			TrackMouseEvent(&tme);
			tracking = true;
		}
		if (selectingLyrics)
		{
			lyricSelB = lyricLineAt(y);
			if (y < lyricArea.y) lyricScroll -= 12;
			else if (y > lyricArea.bottom()) lyricScroll += 12;
			if (lyricScroll < 0) lyricScroll = 0;
			if (lyricScroll > lyricMaxScroll) lyricScroll = lyricMaxScroll;
			InvalidateRect(hwnd, nullptr, FALSE);
			return 0;
		}
		if (dragging)
		{
			if (pressHit == Hit::Seek && seekBar.w > 0)
			{
				float f = (x - seekBar.x) / seekBar.w;
				f = f < 0 ? 0 : f > 1 ? 1 : f;
				dragSeconds = f * player.duration();
			}
			else if (pressHit == Hit::LyricScroll && lyricArea.h > lyricThumbH)
			{
				float f = (y - lyricArea.y - lyricThumbH / 2) / (lyricArea.h - lyricThumbH);
				lyricScroll = (f < 0 ? 0 : f > 1 ? 1 : f) * lyricMaxScroll;
			}
			else if (pressHit == Hit::Volume && volumeBar.w > 0)
			{
				float f = (x - volumeBar.x) / volumeBar.w;
				volume = f < 0 ? 0 : f > 1 ? 1 : f;
				player.setVolume(volume);
			}
			InvalidateRect(hwnd, nullptr, FALSE);
			return 0;
		}
		int idx = -1;
		Hit h = hitAt(x, y, &idx);
		if (h != hoverHit || idx != hoverIndex)
		{
			hoverHit = h; hoverIndex = idx;
			InvalidateRect(hwnd, nullptr, FALSE);
		}
		else if (h == Hit::Seek || h == Hit::Volume)
		{
			InvalidateRect(hwnd, nullptr, FALSE);
		}
		return 0;
	}

	case WM_LBUTTONDOWN:
	{
		float x = GET_X_LPARAM(lp) / scale, y = GET_Y_LPARAM(lp) / scale;
		pressHit = hitAt(x, y, &pressIndex);
		if (editPlaylist >= 0 && pressHit != Hit::EditBox) commitRename();
		if (menuOpen && pressHit != Hit::MenuItem)
		{
			menuOpen = false;
			pressHit = Hit::None;
			InvalidateRect(hwnd, nullptr, FALSE);
			return 0;
		}
		if (pressHit == Hit::LyricLine)
		{
			lyricSelA = lyricSelB = pressIndex;
			selectingLyrics = true;
			SetCapture(hwnd);
		}
		else if (pressHit != Hit::LyricCopy && pressHit != Hit::LyricScroll) lyricSelA = lyricSelB = -1;
		if (pressHit == Hit::Seek || pressHit == Hit::Volume || pressHit == Hit::LyricScroll)
		{
			if (pressHit == Hit::Seek && current < 0) { pressHit = Hit::None; return 0; }
			dragging = true;
			SetCapture(hwnd);
			SendMessageW(hwnd, WM_MOUSEMOVE, wp, lp);   // apply the click position right away
		}
		InvalidateRect(hwnd, nullptr, FALSE);
		return 0;
	}
	case WM_RBUTTONUP:
	{
		if (confirmOpen) return 0;
		float x = GET_X_LPARAM(lp) / scale, y = GET_Y_LPARAM(lp) / scale;
		int idx = -1;
		Hit h = hitAt(x, y, &idx);
		openMenu(h, idx, x, y);
		return 0;
	}
	case WM_LBUTTONUP:
	{
		float x = GET_X_LPARAM(lp) / scale, y = GET_Y_LPARAM(lp) / scale;
		if (selectingLyrics)
		{
			selectingLyrics = false;
			ReleaseCapture();
		}
		else if (dragging)
		{
			dragging = false;
			ReleaseCapture();
			if (pressHit == Hit::Seek) seekTo(dragSeconds);
		}
		else
		{
			int idx = -1;
			Hit h = hitAt(x, y, &idx);
			if (h != Hit::None && h == pressHit && idx == pressIndex) click(h, idx);
		}
		pressHit = Hit::None;
		InvalidateRect(hwnd, nullptr, FALSE);
		return 0;
	}
	case WM_MOUSEWHEEL:
		if (hoverHit != Hit::Volume)
		{
			POINT pt{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
			float x, y;
			toClient(pt, x, y);
			int delta = GET_WHEEL_DELTA_WPARAM(wp);
			float step = delta / 120.0f * 90.0f;
			bool precise = delta % 120 != 0;   // touchpads send small steps; follow them directly
			if (sideList.contains(x, y)) wheelScroll(sideScroll, sideMaxScroll, step, precise);
			else if (page == Page::Now && npTab == 1 && nowAlbumArea.contains(x, y)) wheelScroll(nowAlbumScroll, nowAlbumMaxScroll, step, precise);
			else if (page == Page::Now && npTab == 2 && lyricArea.contains(x, y)) wheelScroll(lyricScroll, lyricMaxScroll, step, precise);
			else if (listArea.contains(x, y)) wheelScroll(listScroll, listMaxScroll, step, precise);
			else if (pageArea.contains(x, y)) wheelScroll(pageScroll, pageMaxScroll, step, precise);
			return 0;
		}
		if (hoverHit == Hit::Volume)
		{
			volume += GET_WHEEL_DELTA_WPARAM(wp) > 0 ? 0.05f : -0.05f;
			volume = volume < 0 ? 0 : volume > 1 ? 1 : volume;
			player.setVolume(volume);
			InvalidateRect(hwnd, nullptr, FALSE);
		}
		return 0;

	case WM_CHAR:
		if (editPlaylist >= 0)
		{
			wchar_t c = (wchar_t)wp;
			if (c == L'\r') commitRename();
			else if (c == 27) { editPlaylist = -1; }
			else if (c == L'\b') { if (!editText.empty()) editText.pop_back(); }
			else if (c >= 32 && editText.size() < 80) editText.push_back(c);
			InvalidateRect(hwnd, nullptr, FALSE);
			return 0;
		}
		break;
	case WM_KEYDOWN:
	{
		if (editPlaylist >= 0) return 0;
		bool ctrl = GetKeyState(VK_CONTROL) < 0;
		if (confirmOpen)
		{
			if (wp == VK_RETURN) click(Hit::ConfirmOk, -1);
			else if (wp == VK_ESCAPE) click(Hit::ConfirmCancel, -1);
			return 0;
		}
		if (menuOpen && wp == VK_ESCAPE) { menuOpen = false; InvalidateRect(hwnd, nullptr, FALSE); return 0; }
		bool onLyrics = page == Page::Now && npTab == 2 && current >= 0;
		if (onLyrics && ctrl && wp == 'C') { copyLyrics(); return 0; }
		if (onLyrics && ctrl && wp == 'A') { lyricSelA = 0; lyricSelB = (int)lyricLines.size() - 1; InvalidateRect(hwnd, nullptr, FALSE); return 0; }
		if (onLyrics && ctrl && wp == 'V') { pasteLyrics(); return 0; }
		if (onLyrics && wp == VK_ESCAPE && lyricSelA >= 0) { lyricSelA = lyricSelB = -1; InvalidateRect(hwnd, nullptr, FALSE); return 0; }
		if (wp == VK_SPACE) togglePlay();
		else if (wp == VK_ESCAPE && queueOpen) click(Hit::QueueButton, -1);
		else if (wp == VK_LEFT) seekTo(player.position() - 5);
		else if (wp == VK_RIGHT) seekTo(player.position() + 5);
		else if (ctrl && wp == 'O') openFileDialog();
		else if (ctrl && wp == VK_OEM_COMMA) { page = Page::Settings; InvalidateRect(hwnd, nullptr, FALSE); }
		else if (ctrl && wp == 'B') click(Hit::NavToggle, -1);
		else if (ctrl && wp == 'L') click(Hit::BarFavourite, -1);
		else if (wp == VK_F5) loadLibrary(true);
		return 0;
	}
	case WM_APPCOMMAND:
		switch (GET_APPCOMMAND_LPARAM(lp))
		{
		case APPCOMMAND_MEDIA_PLAY_PAUSE: togglePlay(); return TRUE;
		case APPCOMMAND_MEDIA_NEXTTRACK: next(true); return TRUE;
		case APPCOMMAND_MEDIA_PREVIOUSTRACK: prev(); return TRUE;
		}
		break;

	case WM_DROPFILES:
	{
		HDROP drop = reinterpret_cast<HDROP>(wp);
		UINT count = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
		std::vector<std::wstring> files;
		for (UINT i = 0; i < count; ++i)
		{
			UINT len = DragQueryFileW(drop, i, nullptr, 0);
			std::wstring f(len, L'\0');
			DragQueryFileW(drop, i, f.data(), len + 1);
			files.push_back(f);
		}
		DragFinish(drop);
		addFiles(files, true);
		return 0;
	}

	case WM_SIZE:
		g.resize(LOWORD(lp), HIWORD(lp));
		InvalidateRect(hwnd, nullptr, FALSE);
		return 0;
	case WM_DPICHANGED:
	{
		scale = HIWORD(wp) / 96.0f;
		g.setDpi((float)HIWORD(wp));
		const RECT* r = reinterpret_cast<const RECT*>(lp);
		SetWindowPos(hwnd, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top,
			SWP_NOZORDER | SWP_NOACTIVATE);
		return 0;
	}
	case WM_GETMINMAXINFO:
	{
		auto* mmi = reinterpret_cast<MINMAXINFO*>(lp);
		mmi->ptMinTrackSize.x = (LONG)(760 * scale);
		mmi->ptMinTrackSize.y = (LONG)(540 * scale);
		return 0;
	}
	case WM_ERASEBKGND:
		return 1;
	case WM_PAINT:
		ValidateRect(hwnd, nullptr);   // first, so paint() can ask for the next animation frame
		paint();
		return 0;
	case WM_TIMER:
		if (wp == 2 || wp == 3) { InvalidateRect(hwnd, nullptr, FALSE); return 0; }   // marquee text, scan progress
		tick();
		return 0;
	case WM_IMPORT_DONE:
		finishImport();
		return 0;
	case WM_COVER_DONE:
		finishCover();
		return 0;
	case WM_SCAN_DONE:
		finishScan();
		return 0;
	case WM_MOVE_DONE:
		finishLibraryMove();
		return 0;
	case WM_ENDSESSION:
		if (wp) saveState();   // Windows is shutting down or logging off; WM_DESTROY may never come
		return 0;
	case WM_DESTROY:
		importCancel = true;
		if (importThread.joinable()) importThread.join();   // stops after the file it's copying
		if (coverThread.joinable()) coverThread.join();
		if (scanThread.joinable()) scanThread.join();
		if (moveThread.joinable()) moveThread.join();
		saveState();
		KillTimer(hwnd, kTimer);
		player.uninit();
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcW(hwnd, msg, wp, lp);
}

// ---------------------------------------------------------------- actions

void App::click(Hit id, int index)
{
	switch (id)
	{
	case Hit::NavToggle:
		animValues(navGeo, labelAlpha, queueVis);
		navFrom = navGeo;
		collapsed = !collapsed;
		navStart = nowMs();
		break;
	case Hit::NavNow: page = Page::Now; break;
	case Hit::NavHome: page = Page::Home; pageScroll = 0; break;
	case Hit::NavLibrary: page = Page::Library; pageScroll = 0; break;
	case Hit::LibTabAlbums: libTab = 0; pageScroll = 0; break;
	case Hit::LibTabArtists: libTab = 1; pageScroll = 0; break;
	case Hit::LibTabSongs: libTab = 2; pageScroll = 0; break;
	case Hit::AlbumTile: albumFromArtist = false; openAlbum(index); break;
	case Hit::ArtistTile: openArtist(index); break;
	case Hit::ArtistAlbum: if (index >= 0 && index < (int)artistAlbums.size()) { albumFromArtist = true; openAlbum(artistAlbums[index]); } break;
	case Hit::TilePlay:
		if (page == Page::Artist && index >= 0 && index < (int)artistAlbums.size()) index = artistAlbums[index];
		if (index >= 0 && index < (int)albums.size()) playList(albums[index].tracks, 0);
		break;
	case Hit::GroupBack:
		if (page == Page::Album && albumFromArtist && findArtist() >= 0) page = Page::Artist;
		else page = Page::Library;
		pageScroll = 0;
		break;
	case Hit::GroupPlay:
	case Hit::GroupShuffle:
	{
		int g2 = page == Page::Album ? findAlbum() : findArtist();
		if (g2 < 0) break;
		const Group& grp = page == Page::Album ? albums[g2] : artists[g2];
		shuffle = id == Hit::GroupShuffle;
		playList(grp.tracks, shuffle ? (int)(rng() % grp.tracks.size()) : 0);
		break;
	}
	case Hit::AlbumRow: { int al = findAlbum(); if (al >= 0) playList(albums[al].tracks, index); break; }
	case Hit::NowAlbumRow: if (nowAlbum >= 0 && nowAlbum < (int)albums.size()) playList(albums[nowAlbum].tracks, index); break;
	case Hit::AlbumCover:
	{
		int al = findAlbum();
		if (al < 0) break;
		std::vector<std::wstring> paths;
		for (int i : albums[al].tracks) paths.push_back(libTracks[i].path);
		setAlbumCover(paths);
		break;
	}
	case Hit::AlbumArtist:
	{
		int al = findAlbum();
		if (al < 0) break;
		for (int i = 0; i < (int)artists.size(); ++i)
			if (_wcsicmp(artists[i].name.c_str(), albums[al].artist.c_str()) == 0) { openArtist(i); break; }
		break;
	}
	case Hit::AddedTile: if (index < (int)recentlyAdded.size()) playFromLibrary(recentlyAdded[index]); break;
	case Hit::RecentTile:
		if (index < (int)recentPlayed.size())
		{
			std::wstring p = recentPlayed[index];
			auto it = std::find_if(queue.begin(), queue.end(), [&](const Track& t) { return t.path == p; });
			if (it != queue.end()) playIndex((int)(it - queue.begin()));
			else addFiles({ p }, true);
		}
		break;
	case Hit::MenuItem:
		if (index >= 0 && index < (int)menu.size())
		{
			Act act = menu[index].act;
			int arg = menu[index].arg;
			std::vector<std::wstring> paths = menuPaths;
			menuOpen = false;
			if (act == Act::PlaylistMenu)
			{
				fillChooser();
				menuOpen = true;
				break;
			}
			if (act == Act::SetCover)
			{
				// a song stands for its whole album
				std::vector<std::wstring> album = paths;
				if (paths.size() == 1)
					if (const Group* g2 = albumOf(paths[0]))
					{
						album.clear();
						for (int i : g2->tracks) album.push_back(libTracks[i].path);
					}
				setAlbumCover(album);
				break;
			}
			if (act == Act::Favourite || act == Act::Unfavourite) { for (const auto& p : paths) toggleFavourite(p); break; }
			if (act == Act::AddTo) { addToPlaylist(arg, paths); break; }
			if (act == Act::NewWith)
			{
				int p = newPlaylist();
				if (p >= 0) { addToPlaylist(p, paths); startRename(p); }
				break;
			}
			if (act == Act::RemoveFromPlaylist)
			{
				if (menuPlaylist >= 0 && menuPosition >= 0) pls.removeAt(menuPlaylist, menuPosition);
				++plVersion;
				break;
			}
			if (act == Act::PlayPlaylist) { playPlaylist(arg, 0, false); break; }
			if (act == Act::RenamePlaylist) { startRename(arg); break; }
			if (act == Act::DeletePlaylist)
			{
				pendingDeletePlaylist = arg;
				confirmTitle = L"Delete the playlist \u201C" + widen(pls.all()[arg].name) + L"\u201D?";
				confirmOpen = true;
				break;
			}
			if (act == Act::Play)
			{
				queue.clear();
				order.clear();
				for (const auto& p : paths)
				{
					auto it = std::find_if(libTracks.begin(), libTracks.end(), [&](const Track& t) { return t.path == p; });
					if (it != libTracks.end()) queue.push_back(*it);
				}
				if (queue.empty()) addFiles(paths, true);
				else playIndex(0);
			}
			else if (act == Act::AddToLibrary) importFiles(paths);
			else
			{
				pendingRemove = paths;
				bool anyImported = false;
				for (const auto& p : paths) if (isImported(p)) anyImported = true;
				wchar_t t[96];
				if (!anyImported) swprintf(t, 96, paths.size() == 1 ? L"Hide this song from LossCat?" : L"Hide %zu songs from LossCat?", paths.size());
				else if (paths.size() == 1) swprintf(t, 96, L"Move this song to the Recycle Bin?");
				else swprintf(t, 96, L"Remove %zu songs from your library?", paths.size());
				confirmTitle = t;
				confirmOpen = true;
			}
		}
		break;
	case Hit::ConfirmOk:
		confirmOpen = false;
		confirmBody.clear();
		if (!pendingLibraryDir.empty()) startLibraryMove();
		else if (pendingRemoveLyrics)
		{
			pendingRemoveLyrics = false;
			saveLyrics(L"");
		}
		else if (pendingDeletePlaylist > 0)
		{
			if (pls.remove(pendingDeletePlaylist))
			{
				if (page == Page::Playlist && openPlaylist == pendingDeletePlaylist) page = Page::Home;
				else if (openPlaylist > pendingDeletePlaylist) --openPlaylist;
			}
			++plVersion;
		}
		else removeFromLibrary(pendingRemove);
		pendingRemove.clear();
		pendingDeletePlaylist = -1;
		break;
	case Hit::ConfirmCancel:
		confirmOpen = false;
		confirmBody.clear();
		pendingLibraryDir.clear();
		pendingRemoveLyrics = false;
		pendingRemove.clear();
		pendingDeletePlaylist = -1;
		break;
	case Hit::NewPlaylist:
	{
		int p = newPlaylist();
		if (p >= 0) { openPlaylist = p; page = Page::Playlist; startRename(p); }
		break;
	}
	case Hit::PlaylistNav: openPlaylist = index; page = Page::Playlist; break;
	case Hit::PlaylistRow: playPlaylist(openPlaylist, index, false); break;
	case Hit::PlaylistPlay: playPlaylist(openPlaylist, -1, false); break;
	case Hit::PlaylistShuffle: playPlaylist(openPlaylist, -1, true); break;
	case Hit::PlaylistRename: startRename(openPlaylist); break;
	case Hit::PlaylistDelete:
		if (openPlaylist > 0)
		{
			pendingDeletePlaylist = openPlaylist;
			confirmTitle = L"Delete the playlist \u201C" + widen(pls.all()[openPlaylist].name) + L"\u201D?";
			confirmOpen = true;
		}
		break;
	case Hit::Favourite: if (current >= 0) toggleFavourite(queue[current].path); break;
	case Hit::BarFavourite:
		if (current >= 0)
		{
			std::wstring p = queue[current].path;
			if (ensureInLibrary(p)) toggleFavourite(p);
		}
		break;
	case Hit::BarAddTo:
		if (current >= 0)
		{
			std::wstring p = queue[current].path;
			if (ensureInLibrary(p)) openPlaylistChooser({ p }, mouseX, mouseY);
		}
		break;
	case Hit::Link:
		if (index >= 0 && index < (int)links.size())
		{
			const std::wstring& l = links[index];
			if (l.rfind(L"license:", 0) == 0)
			{
				size_t bar = l.find(L'|');
				openLicense(_wtoi(l.c_str() + 8), l.substr(bar + 1));
			}
			else ShellExecuteW(hwnd, L"open", l.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
		}
		break;
	case Hit::Back: page = Page::Settings; pageScroll = 0; break;
	case Hit::CancelImport: importCancel = true; break;
	case Hit::RowHeart:
		if (index >= 0 && index < (int)rowHeartPaths.size()) toggleFavourite(rowHeartPaths[index]);
		break;
	case Hit::LibTabPlaylists: libTab = 3; pageScroll = 0; break;
	case Hit::PlaylistTile:
		if (tileToPlaylist(index) == -2) { page = Page::Smart; listScroll = 0; }
		else { openPlaylist = tileToPlaylist(index); page = Page::Playlist; }
		break;
	case Hit::SmartNav: page = Page::Smart; listScroll = 0; break;
	case Hit::SmartRow: { std::vector<int> idx = smartTracks(); if (index >= 0 && index < (int)idx.size()) playList(idx, index); break; }
	case Hit::GoToAlbum:
		if (current >= 0)
			for (int k = 0; k < (int)albums.size(); ++k)
				for (int t : albums[k].tracks)
					if (libTracks[t].path == queue[current].path) { albumFromArtist = false; openAlbum(k); k = (int)albums.size(); break; }
		break;
	case Hit::NowMore: openMenu(Hit::NowMore, -1, nowMoreAt.x, nowMoreAt.bottom() + 4); break;
	case Hit::ImportFiles: { auto f = pickFiles(); if (!f.empty()) importFiles(f); break; }
	case Hit::Rescan: notice.clear(); loadLibrary(true); break;
	case Hit::AddToLibrary: if (current >= 0 && !inLibrary(queue[current].path)) importFiles({ queue[current].path }); break;
	case Hit::Settings: case Hit::SignalPath: page = Page::Settings; pageScroll = 0; break;
	case Hit::WinMin: ShowWindow(hwnd, SW_MINIMIZE); break;
	case Hit::WinClose: PostMessageW(hwnd, WM_CLOSE, 0, 0); break;
	case Hit::TabNext: npTab = 0; break;
	case Hit::TabAbout: npTab = 1; break;
	case Hit::TabLyrics: npTab = 2; break;
	case Hit::LyricCopy: copyLyrics(); break;
	case Hit::LyricPaste: pasteLyrics(); break;
	case Hit::LyricFile: loadLyricsFile(); break;
	case Hit::LyricRemove:
		pendingRemoveLyrics = true;
		confirmTitle = L"Remove the lyrics from this song?";
		confirmOpen = true;
		break;
	case Hit::Cover: if (current >= 0) page = Page::Now; break;
	case Hit::Shuffle: shuffle = !shuffle; rebuildOrder(); break;
	case Hit::PlayAll:
	case Hit::ShuffleAll:
		if (!libTracks.empty())
		{
			shuffle = id == Hit::ShuffleAll;
			queue = libTracks;
			order.clear();
			playIndex(shuffle ? (int)(rng() % queue.size()) : 0);
		}
		break;
	case Hit::Repeat: repeat = (repeat + 1) % 3; break;
	case Hit::Prev: prev(); break;
	case Hit::Play: togglePlay(); break;
	case Hit::Next: next(true); break;
	case Hit::QueueButton:
		animValues(navGeo, labelAlpha, queueVis);
		queueFrom = queueVis;
		queueOpen = !queueOpen;
		queueStart = nowMs();
		break;
	case Hit::OpenFiles: openFileDialog(); break;
	case Hit::QueueRow: playIndex(index); break;
	case Hit::LibraryRow: playFromLibrary(index); break;
	case Hit::OpenLibraryFolder:
		if (!libraryDir.empty()) ShellExecuteW(hwnd, L"open", libraryDir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
		break;
	case Hit::OpenMusicFolder:
		if (index >= 0 && index < (int)musicFolders.size()) ShellExecuteW(hwnd, L"open", musicFolders[index].c_str(), nullptr, nullptr, SW_SHOWNORMAL);
		break;
	case Hit::AddMusicFolder: addMusicFolder(); break;
	case Hit::ChangeLibraryFolder: changeLibraryFolder(); break;
	case Hit::RemoveMusicFolder:
		if (index >= 0 && index < (int)musicFolders.size() && !busyImporting())
		{
			musicFolders.erase(musicFolders.begin() + index);
			saveState();
			loadLibrary(true);
		}
		break;
	case Hit::ShowHidden:
		if (!busyImporting())
		{
			hiddenFiles.clear();
			saveState();
			loadLibrary(true);
		}
		break;
	default: break;
	}
	updateTimer();
	InvalidateRect(hwnd, nullptr, FALSE);
}

void App::loadLibrary(bool background)
{
	if (busyImporting()) return;
	PWSTR music = nullptr;
	if (!customLibraryDir.empty()) libraryDir = customLibraryDir;
	else if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Music, 0, nullptr, &music)))
		libraryDir = std::wstring(music) + L"\\LossCat";
	CoTaskMemFree(music);
	if (libraryDir.empty()) return;

	std::vector<std::string> folders, hidden;
	for (const auto& f : musicFolders) folders.push_back(narrow(f));
	for (const auto& h : hiddenFiles) hidden.push_back(narrow(h));
	std::string root = narrow(libraryDir), cache = narrow(cachePath());

	if (!background)
	{
		// at startup: with the tag cache this is quick even for big folders
		if (!lib.open(root, folders, hidden, cache)) return;
		pls.load(root);
		refreshLibrary();
		return;
	}
	scanning = true;
	scanCount = 0;
	notice.clear();
	scanThread = std::thread([this, root, folders, hidden, cache]() {
		scanned = Library();
		scanned.open(root, folders, hidden, cache, [this](int n) { scanCount = n; PostMessageW(hwnd, WM_NULL, 0, 0); });
		PostMessageW(hwnd, WM_SCAN_DONE, 0, 0);
	});
	SetTimer(hwnd, 3, 100, nullptr);   // redraw the progress
	InvalidateRect(hwnd, nullptr, FALSE);
}

void App::finishScan()
{
	if (scanThread.joinable()) scanThread.join();
	KillTimer(hwnd, 3);
	lib = std::move(scanned);
	scanned = Library();
	scanning = false;
	pls.load(narrow(libraryDir));
	covers.clear();
	refreshLibrary();
	notice = libTracks.size() == 1 ? L"1 song in your library" : std::to_wstring(libTracks.size()) + L" songs in your library";
	InvalidateRect(hwnd, nullptr, FALSE);
}

void App::changeLibraryFolder()
{
	if (busyImporting()) return;
	std::wstring picked = pickFolder(L"Choose a new LossCat folder");
	if (picked.empty()) return;
	auto under = [](const std::wstring& path, const std::wstring& dir) {
		return path.size() >= dir.size() && _wcsnicmp(path.c_str(), dir.c_str(), dir.size()) == 0
			&& (path.size() == dir.size() || path[dir.size()] == L'\\' || dir.back() == L'\\');
	};
	if (under(picked, libraryDir))
	{
		notice = _wcsicmp(picked.c_str(), libraryDir.c_str()) == 0 ? L"That's already the LossCat folder" : L"The new folder can't be inside the current one";
		InvalidateRect(hwnd, nullptr, FALSE);
		return;
	}
	pendingLibraryDir = picked;
	int songs = 0;
	for (const Track& t : libTracks) if (isImported(t.path)) ++songs;
	confirmTitle = L"Move the LossCat folder?";
	confirmBody = L"Your imported songs (" + std::to_wstring(songs) + L") and playlists move to " + picked + L". Nothing else changes.";
	confirmOpen = true;
	InvalidateRect(hwnd, nullptr, FALSE);
}

// Moves everything in the LossCat folder to pendingLibraryDir on a worker thread.
// A rename on the same drive is instant; across drives each file is copied, then deleted.
void App::startLibraryMove()
{
	std::wstring from = libraryDir, to = pendingLibraryDir;
	pendingLibraryDir.clear();
	if (from.empty() || to.empty() || busyImporting()) return;

	// the decoder may hold a file from the folder open
	moveResumePath.clear();
	if (current >= 0 && isImported(queue[current].path))
	{
		moveResumePath = queue[current].path;
		moveResume = player.getState() == State::Playing;
		moveResumeAt = player.position();
		player.uninit();
		audioOk = player.init(0, 0);
		if (audioOk) player.setVolume(volume);
	}
	moveFrom = from;
	moving = true;
	moveDone = 0;
	moveFailed = 0;
	moveTotal = 0;
	moveTo = to;   // becomes the LossCat folder once the move is done
	moveThread = std::thread([this, from, to]() {
		namespace fs = std::filesystem;
		std::error_code ec;
		fs::path src(from), dst(to);
		std::vector<fs::path> files;
		for (fs::recursive_directory_iterator it(src, fs::directory_options::skip_permission_denied, ec), end; !ec && it != end; it.increment(ec))
			if (it->is_regular_file(ec)) files.push_back(it->path());
		moveTotal = (int)files.size();
		for (const fs::path& f : files)
		{
			fs::path target = dst / f.lexically_relative(src);
			fs::create_directories(target.parent_path(), ec);
			ec.clear();
			if (fs::exists(target, ec))
			{
				if (fs::file_size(target, ec) == fs::file_size(f, ec)) { fs::remove(f, ec); ++moveDone; continue; }   // already there
				fs::path base = target;
				for (int n = 2; fs::exists(target, ec); ++n)
				{
					target = base.parent_path() / base.stem();
					target += L" (" + std::to_wstring(n) + L")";
					target += base.extension();
				}
			}
			fs::rename(f, target, ec);
			if (ec)
			{
				ec.clear();
				fs::copy_file(f, target, ec);
				if (!ec) fs::remove(f, ec);
			}
			if (ec) ++moveFailed;
			++moveDone;
		}
		// clear out the folders left empty, deepest first
		std::vector<fs::path> dirs;
		for (fs::recursive_directory_iterator it(src, ec), end; !ec && it != end; it.increment(ec))
			if (it->is_directory(ec)) dirs.push_back(it->path());
		std::sort(dirs.begin(), dirs.end(), [](const fs::path& a, const fs::path& b) { return a.native().size() > b.native().size(); });
		for (const fs::path& d : dirs) fs::remove(d, ec);
		fs::remove(src, ec);
		PostMessageW(hwnd, WM_MOVE_DONE, 0, 0);
	});
	SetTimer(hwnd, 3, 100, nullptr);
	InvalidateRect(hwnd, nullptr, FALSE);
}

void App::finishLibraryMove()
{
	if (moveThread.joinable()) moveThread.join();
	KillTimer(hwnd, 3);
	moving = false;
	std::wstring from = moveFrom, to = moveTo;
	customLibraryDir = to;
	auto remap = [&](std::wstring& p) {
		if (p.size() > from.size() && _wcsnicmp(p.c_str(), from.c_str(), from.size()) == 0 && (p[from.size()] == L'\\' || p[from.size()] == L'/'))
			p = to + p.substr(from.size());
	};
	for (Track& t : queue) remap(t.path);
	for (std::wstring& p : recentPlayed) remap(p);
	remap(moveResumePath);
	{
		std::map<std::wstring, std::wstring> moved;
		for (auto& [k, v] : lyricsByPath) { std::wstring nk = k; remap(nk); moved[nk] = v; }
		lyricsByPath = std::move(moved);
	}
	covers.clear();
	coverFor = -1;

	// The tag cache is keyed by path; point its entries at the new folder so nothing is read again.
	std::wstring cache = cachePath();
	FILE* f = nullptr;
	if (!cache.empty() && _wfopen_s(&f, cache.c_str(), L"rb") == 0 && f)
	{
		std::string data;
		char chunk[8192];
		size_t n;
		while ((n = fread(chunk, 1, sizeof chunk, f)) > 0) data.append(chunk, n);
		fclose(f);
		auto esc = [](const std::string& s) { std::string o; for (char c : s) { if (c == '\\') o += "\\\\"; else o += c; } return o; };
		std::string a = esc(narrow(from)), b = esc(narrow(to));
		std::string out;
		size_t start = 0;
		while (start < data.size())
		{
			size_t end = data.find('\n', start);
			if (end == std::string::npos) end = data.size();
			std::string line = data.substr(start, end - start);
			if (line.size() > a.size() && _strnicmp(line.c_str(), a.c_str(), a.size()) == 0) line = b + line.substr(a.size());
			out += line + "\n";
			start = end + 1;
		}
		if (_wfopen_s(&f, cache.c_str(), L"wb") == 0 && f) { fwrite(out.data(), 1, out.size(), f); fclose(f); }
	}

	// a music folder that is now the LossCat folder would list everything twice
	musicFolders.erase(std::remove_if(musicFolders.begin(), musicFolders.end(), [&](const std::wstring& m) { return _wcsicmp(m.c_str(), to.c_str()) == 0; }), musicFolders.end());
	saveState();
	loadLibrary();

	if (!moveResumePath.empty() && current >= 0 && queue[current].path == moveResumePath && audioOk && player.load(fsPath(moveResumePath)))
	{
		player.seek(moveResumeAt);
		if (moveResume) player.play();
	}
	notice = moveFailed ? L"Moved, but " + std::to_wstring(moveFailed) + L" files couldn't be moved. They're still in " + from
		: L"The LossCat folder is now " + to;
	updateTimer();
	InvalidateRect(hwnd, nullptr, FALSE);
}

std::wstring App::cachePath() const
{
	std::wstring s = statePath();
	return s.empty() ? L"" : s.substr(0, s.find_last_of(L'\\')) + L"\\library-cache.txt";
}

std::wstring App::pickFolder(const wchar_t* title)
{
	std::wstring picked;
	IFileOpenDialog* dlg = nullptr;
	if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dlg))))
	{
		DWORD opts = 0;
		dlg->GetOptions(&opts);
		dlg->SetOptions(opts | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
		dlg->SetTitle(title);
		IShellItem* item = nullptr;
		if (SUCCEEDED(dlg->Show(hwnd)) && SUCCEEDED(dlg->GetResult(&item)))
		{
			PWSTR p = nullptr;
			if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &p))) { picked = p; CoTaskMemFree(p); }
			item->Release();
		}
		dlg->Release();
	}
	while (picked.size() > 3 && (picked.back() == L'\\' || picked.back() == L'/')) picked.pop_back();
	return picked;
}

void App::addMusicFolder()
{
	if (busyImporting()) return;
	std::wstring picked = pickFolder(L"Add a music folder");
	if (picked.empty()) return;

	auto under = [](const std::wstring& path, const std::wstring& dir) {
		return path.size() >= dir.size() && _wcsnicmp(path.c_str(), dir.c_str(), dir.size()) == 0
			&& (path.size() == dir.size() || path[dir.size()] == L'\\' || dir.back() == L'\\');
	};
	if (under(picked, libraryDir)) { notice = L"That folder is already part of your library"; InvalidateRect(hwnd, nullptr, FALSE); return; }
	for (const auto& f : musicFolders)
		if (under(picked, f)) { notice = L"That folder is already in your list"; InvalidateRect(hwnd, nullptr, FALSE); return; }
	// a new folder that contains ones already listed replaces them
	musicFolders.erase(std::remove_if(musicFolders.begin(), musicFolders.end(), [&](const std::wstring& f) { return under(f, picked); }), musicFolders.end());
	musicFolders.push_back(picked);
	saveState();
	loadLibrary(true);
}

void App::refreshLibrary()
{
	++plVersion;
	libTracks.clear();
	for (const LibraryTrack& t : lib.tracks())
	{
		libTracks.push_back(makeTrack(widen(t.path), t.info, false));
	}

	albums.clear();
	artists.clear();
	std::map<std::wstring, size_t> albumAt, artistAt;
	auto lower = [](std::wstring s) { for (auto& c : s) c = (wchar_t)towlower(c); return s; };
	for (int i = 0; i < (int)libTracks.size(); ++i)
	{
		const Track& t = libTracks[i];
		std::wstring albumName = t.album.empty() ? L"Unknown album" : t.album;
		std::wstring key = lower(albumName) + L"\x1F" + lower(t.artist);
		auto a = albumAt.find(key);
		if (a == albumAt.end()) { a = albumAt.emplace(key, albums.size()).first; albums.push_back({ albumName, t.artist, {} }); }
		albums[a->second].tracks.push_back(i);
		auto r = artistAt.find(lower(t.artist));
		if (r == artistAt.end()) { r = artistAt.emplace(lower(t.artist), artists.size()).first; artists.push_back({ t.artist, t.artist, {} }); }
		artists[r->second].tracks.push_back(i);
	}
	auto byTrackNo = [&](int a, int b) {
		const TrackInfo& x = libTracks[a].info; const TrackInfo& y = libTracks[b].info;
		return x.track != y.track ? x.track < y.track : libTracks[a].path < libTracks[b].path;
	};
	for (auto& al : albums) std::sort(al.tracks.begin(), al.tracks.end(), byTrackNo);
	std::sort(albums.begin(), albums.end(), [&](const Group& a, const Group& b) {
		return lower(a.artist) != lower(b.artist) ? lower(a.artist) < lower(b.artist) : lower(a.name) < lower(b.name); });
	std::sort(artists.begin(), artists.end(), [&](const Group& a, const Group& b) { return lower(a.name) < lower(b.name); });

	std::vector<std::pair<ULONGLONG, int>> times;
	for (int i = 0; i < (int)libTracks.size(); ++i)
	{
		WIN32_FILE_ATTRIBUTE_DATA fa;
		ULONGLONG t = 0;
		if (GetFileAttributesExW(libTracks[i].path.c_str(), GetFileExInfoStandard, &fa))
			t = ((ULONGLONG)fa.ftCreationTime.dwHighDateTime << 32) | fa.ftCreationTime.dwLowDateTime;
		times.push_back({ t, i });
	}
	std::sort(times.begin(), times.end(), [](auto& a, auto& b) { return a.first > b.first; });
	recentlyAdded.clear();
	for (size_t i = 0; i < times.size() && i < 12; ++i) recentlyAdded.push_back(times[i].second);
	InvalidateRect(hwnd, nullptr, FALSE);
}

void App::playFromLibrary(int i)
{
	if (i < 0 || i >= (int)libTracks.size()) return;
	queue = libTracks;
	order.clear();
	playIndex(i);
}

void App::openAlbum(int album)
{
	if (album < 0 || album >= (int)albums.size()) return;
	albumName = albums[album].name;
	albumArtist = albums[album].artist;
	page = Page::Album;
	listScroll = 0;
}

void App::openArtist(int artist)
{
	if (artist < 0 || artist >= (int)artists.size()) return;
	artistName = artists[artist].name;
	page = Page::Artist;
	pageScroll = 0;
}

int App::findAlbum() const
{
	for (int i = 0; i < (int)albums.size(); ++i)
		if (_wcsicmp(albums[i].name.c_str(), albumName.c_str()) == 0 && _wcsicmp(albums[i].artist.c_str(), albumArtist.c_str()) == 0) return i;
	return -1;
}

int App::findArtist() const
{
	for (int i = 0; i < (int)artists.size(); ++i)
		if (_wcsicmp(artists[i].name.c_str(), artistName.c_str()) == 0) return i;
	return -1;
}

void App::playList(const std::vector<int>& idx, int start)
{
	if (idx.empty()) return;
	queue.clear();
	order.clear();
	for (int i : idx) queue.push_back(libTracks[i]);
	playIndex(start);
}

bool App::isImported(const std::wstring& path) const
{
	if (libraryDir.empty() || path.size() <= libraryDir.size()) return false;
	return _wcsnicmp(path.c_str(), libraryDir.c_str(), libraryDir.size()) == 0
		&& (path[libraryDir.size()] == L'\\' || path[libraryDir.size()] == L'/');
}

// In LossCat's own folder or one of the music folders, and not hidden.
bool App::inLibrary(const std::wstring& path) const
{
	if (isImported(path)) return true;
	bool inFolder = false;
	for (const auto& f : musicFolders)
		if (path.size() > f.size() && _wcsnicmp(path.c_str(), f.c_str(), f.size()) == 0
			&& (path[f.size()] == L'\\' || path[f.size()] == L'/' || f.back() == L'\\'))
			inFolder = true;
	return inFolder && std::find(hiddenFiles.begin(), hiddenFiles.end(), path) == hiddenFiles.end();
}

void App::openMenu(Hit target, int index, float x, float y)
{
	menu.clear();
	menuPaths.clear();
	auto songPath = [&]() -> std::wstring {
		switch (target)
		{
		case Hit::LibraryRow: return index < (int)libTracks.size() ? libTracks[index].path : L"";
		case Hit::NowAlbumRow: return nowAlbum >= 0 && nowAlbum < (int)albums.size() && index < (int)albums[nowAlbum].tracks.size() ? libTracks[albums[nowAlbum].tracks[index]].path : L"";
		case Hit::AlbumRow: { int al = findAlbum(); return al >= 0 && index < (int)albums[al].tracks.size() ? libTracks[albums[al].tracks[index]].path : L""; }
		case Hit::AddedTile: return index < (int)recentlyAdded.size() ? libTracks[recentlyAdded[index]].path : L"";
		case Hit::RecentTile: return index < (int)recentPlayed.size() ? recentPlayed[index] : L"";
		case Hit::QueueRow: return index < (int)queue.size() ? queue[index].path : L"";
		case Hit::PlaylistRow: { const auto& l = playlistTracks(openPlaylist); return index < (int)l.size() ? l[index].path : L""; }
		case Hit::Cover: case Hit::AddToLibrary: case Hit::NowMore: return current >= 0 ? queue[current].path : L"";
		case Hit::SmartRow: { std::vector<int> idx = smartTracks(); return index >= 0 && index < (int)idx.size() ? libTracks[idx[index]].path : L""; }
		default: return L"";
		}
	};
	if (target == Hit::PlaylistTile)
	{
		index = tileToPlaylist(index);
		if (index == -2) return;
		target = Hit::PlaylistNav;
	}
	if (target == Hit::ArtistAlbum) { target = Hit::AlbumTile; index = index >= 0 && index < (int)artistAlbums.size() ? artistAlbums[index] : -1; }
	if (target == Hit::TilePlay) return;
	if (target == Hit::PlaylistNav)
	{
		const auto& all = pls.all();
		if (index < 0 || index >= (int)all.size()) return;
		menuPlaylist = index;
		menu.push_back({ L"Play", Act::PlayPlaylist, false, index });
		if (index > 0)
		{
			menu.push_back({ L"Rename", Act::RenamePlaylist, false, index });
			menu.push_back({ L"Delete playlist", Act::DeletePlaylist, true, index });
		}
		menuX = x; menuY = y; menuOpen = true;
		InvalidateRect(hwnd, nullptr, FALSE);
		return;
	}
	menuPlaylist = target == Hit::PlaylistRow ? openPlaylist : -1;
	menuPosition = target == Hit::PlaylistRow ? index : -1;
	const Group* group = nullptr;
	if (target == Hit::AlbumTile && index >= 0 && index < (int)albums.size()) group = &albums[index];
	if (target == Hit::ArtistTile && index >= 0 && index < (int)artists.size()) group = &artists[index];

	if (group)
	{
		for (int i : group->tracks) menuPaths.push_back(libTracks[i].path);
		menu.push_back({ target == Hit::AlbumTile ? L"Play album" : L"Play all songs", Act::Play, false });
		if (target == Hit::AlbumTile) menu.push_back({ L"Set album cover\x2026", Act::SetCover, false });
		menu.push_back({ L"Add to playlist\x2026", Act::PlaylistMenu, false });
		menu.push_back({ target == Hit::AlbumTile ? L"Remove album from library" : L"Remove artist from library", Act::Remove, true });
	}
	else
	{
		std::wstring p = songPath();
		if (p.empty()) { if (menuOpen) { menuOpen = false; InvalidateRect(hwnd, nullptr, FALSE); } return; }
		menuPaths.push_back(p);
		bool fromNow = target == Hit::NowMore;   // Now playing already has Play, Favourite and Add to library buttons
		if (!fromNow) menu.push_back({ L"Play", Act::Play, false });
		if (inLibrary(p))
		{
			bool fav = inPlaylist(0, p);
			if (!fromNow) menu.push_back({ fav ? L"Remove from Favourites" : L"Add to Favourites", fav ? Act::Unfavourite : Act::Favourite, false });
			menu.push_back({ L"Add to playlist\x2026", Act::PlaylistMenu, false });
			menu.push_back({ L"Set album cover\x2026", Act::SetCover, false });
			if (target == Hit::PlaylistRow && menuPlaylist >= 0)
				menu.push_back({ L"Remove from this playlist", Act::RemoveFromPlaylist, false });
			menu.push_back({ isImported(p) ? L"Remove from library" : L"Hide from library", Act::Remove, true });
		}
		else menu.push_back({ L"Add to library", Act::AddToLibrary, false });
	}
	menuX = x;
	menuY = y;
	menuOpen = true;
	InvalidateRect(hwnd, nullptr, FALSE);
}

// Playlists keep songs in LossCat's folder relative to it, and songs from music folders by full path.
std::wstring App::absOf(const std::string& rel) const
{
	bool full = (rel.size() > 2 && rel[1] == ':') || rel.rfind("\\\\", 0) == 0;
	return full ? widen(rel) : libraryDir + L"\\" + widen(rel);
}

std::string App::relOf(const std::wstring& abs) const
{
	if (!inLibrary(abs)) return {};
	if (!isImported(abs)) return narrow(abs);
	return narrow(abs.substr(libraryDir.size() + 1));
}

bool App::inPlaylist(int p, const std::wstring& abs) const
{
	const auto& all = pls.all();
	if (p < 0 || p >= (int)all.size()) return false;
	std::string rel = relOf(abs);
	return !rel.empty() && std::find(all[p].paths.begin(), all[p].paths.end(), rel) != all[p].paths.end();
}

const std::vector<Track>& App::playlistTracks(int p)
{
	if (plTracksFor == p && plTracksVersion == plVersion) return plTracks;
	plTracks.clear();
	plTracksFor = p;
	plTracksVersion = plVersion;
	const auto& all = pls.all();
	if (p < 0 || p >= (int)all.size()) return plTracks;
	for (const std::string& rel : all[p].paths)
	{
		std::wstring abs = absOf(rel);
		auto it = std::find_if(libTracks.begin(), libTracks.end(), [&](const Track& t) { return t.path == abs; });
		if (it != libTracks.end()) { plTracks.push_back(*it); continue; }
		Track missing;
		missing.path = abs;
		missing.title = L"Missing: " + fileStem(abs);
		missing.coverLoaded = true;
		plTracks.push_back(missing);
	}
	return plTracks;
}

void App::addToPlaylist(int p, const std::vector<std::wstring>& paths)
{
	int added = 0;
	for (const auto& abs : paths)
		if (inLibrary(abs) && pls.add(p, narrow(abs))) ++added;
	++plVersion;
	const auto& all = pls.all();
	if (p >= 0 && p < (int)all.size())
	{
		if (added == 1) notice = L"Added to " + widen(all[p].name);
		else notice = L"Added " + std::to_wstring(added) + L" songs to " + widen(all[p].name);
	}
	InvalidateRect(hwnd, nullptr, FALSE);
}

void App::toggleFavourite(const std::wstring& abs)
{
	if (!inLibrary(abs)) return;
	std::string rel = relOf(abs);
	const auto& fav = pls.all()[0].paths;
	auto it = std::find(fav.begin(), fav.end(), rel);
	if (it != fav.end()) pls.removeAt(0, (int)(it - fav.begin()));
	else pls.add(0, narrow(abs));
	++plVersion;
	InvalidateRect(hwnd, nullptr, FALSE);
}

int App::newPlaylist()
{
	std::wstring name = L"New playlist";
	for (int n = 2;; ++n)
	{
		bool taken = false;
		for (const auto& pl : pls.all()) if (sameName(pl.name, narrow(name))) taken = true;
		if (!taken) break;
		name = L"New playlist " + std::to_wstring(n);
	}
	int p = pls.create(narrow(name));
	++plVersion;
	return p;
}

void App::startRename(int p)
{
	if (p <= 0 || p >= (int)pls.all().size()) return;
	if (collapsed) click(Hit::NavToggle, -1);
	editPlaylist = p;
	editText = widen(pls.all()[p].name);
	InvalidateRect(hwnd, nullptr, FALSE);
}

void App::commitRename()
{
	if (editPlaylist < 0) return;
	int p = editPlaylist;
	editPlaylist = -1;
	std::string name = narrow(editText);
	if (!name.empty() && name != pls.all()[p].name && !pls.rename(p, name))
		notice = L"A playlist with that name already exists";
	++plVersion;
	InvalidateRect(hwnd, nullptr, FALSE);
}

void App::fillChooser()
{
	menu.clear();
	menu.push_back({ L"Add to playlist", Act::Play, false, -1, Look::Heading });
	menu.push_back({ L"New playlist", Act::NewWith, false, -1, Look::Action });
	const auto& all = pls.all();
	for (int p = 1; p < (int)all.size(); ++p) menu.push_back({ widen(all[p].name), Act::AddTo, false, p });
}

void App::openPlaylistChooser(const std::vector<std::wstring>& paths, float x, float y)
{
	menuPaths = paths;
	fillChooser();
	menuX = x;
	menuY = y;
	menuOpen = true;
	InvalidateRect(hwnd, nullptr, FALSE);
}

// Liking or adding a song that was only dropped on the window copies it into the library first.
bool App::ensureInLibrary(std::wstring& path)
{
	if (inLibrary(path)) return true;
	if (busyImporting()) return false;
	std::wstring original = path;
	importFiles({ original });
	if (current >= 0 && !queue.empty()) path = queue[current].path;
	return inLibrary(path);
}

// License texts are compiled into the exe (LossCat.rc), so they travel with every copy.
void App::openLicense(int resourceId, const std::wstring& title)
{
	licenseText.clear();
	if (HRSRC res = FindResourceW(nullptr, MAKEINTRESOURCEW(resourceId), MAKEINTRESOURCEW(10)))   // RT_RCDATA
	{
		if (HGLOBAL h = LoadResource(nullptr, res))
		{
			const char* data = static_cast<const char*>(LockResource(h));
			DWORD size = SizeofResource(nullptr, res);
			if (data) licenseText = widen(std::string(data, size));
		}
	}
	licenseTitle = title;
	page = Page::License;
	pageScroll = 0;
	InvalidateRect(hwnd, nullptr, FALSE);
}

void App::playPlaylist(int p, int start, bool shuffled)
{
	const auto& list = playlistTracks(p);
	if (list.empty()) return;
	if (shuffled) shuffle = true;
	queue = list;
	order.clear();
	if (start < 0) start = shuffled ? (int)(rng() % queue.size()) : 0;
	playIndex(start);
}

void App::removeFromLibrary(const std::vector<std::wstring>& paths)
{
	if (paths.empty() || busyImporting()) return;
	auto removing = [&](const std::wstring& p) { return std::find(paths.begin(), paths.end(), p) != paths.end(); };

	if (current >= 0 && removing(queue[current].path))
	{
		bool wasPlaying = player.getState() == State::Playing;
		int nextKept = -1;
		for (int i : upcoming())
			if (!removing(queue[i].path)) { nextKept = i; break; }
		if (nextKept >= 0)
		{
			playIndex(nextKept);
			if (!wasPlaying) player.pause();
		}
		else
		{
			player.uninit();
			audioOk = player.init(0, 0);
			if (audioOk) player.setVolume(volume);
			current = -1;
			SetWindowTextW(hwnd, L"LossCat");
		}
	}

	// Copies in LossCat's folder go to the Recycle Bin; songs from music folders are only hidden.
	std::wstring list;
	int recycling = 0;
	for (const auto& p : paths) if (isImported(p)) { list += p; list.push_back(L'\0'); ++recycling; }
	list.push_back(L'\0');
	if (recycling)
	{
		SHFILEOPSTRUCTW op = {};
		op.hwnd = hwnd;
		op.wFunc = FO_DELETE;
		op.pFrom = list.c_str();
		op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
		SHFileOperationW(&op);
	}

	int removed = 0, hid = 0;
	std::vector<std::wstring> gone;   // no longer in the library: deleted, or hidden
	for (const auto& p : paths)
	{
		bool imported = isImported(p);
		if (imported && GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES) continue;
		std::string u = narrow(p);
		const auto& tracks = lib.tracks();
		for (int i = 0; i < (int)tracks.size(); ++i)
		{
			if (tracks[i].path != u) continue;
			if (imported) { lib.forget(i); ++removed; }
			else { lib.hide(i); hiddenFiles.push_back(p); ++hid; }
			break;
		}
		gone.push_back(p);
		covers.erase(p);
	}
	auto isGone = [&](const std::wstring& p) { return std::find(gone.begin(), gone.end(), p) != gone.end(); };

	std::wstring playing = current >= 0 ? queue[current].path : L"";
	std::vector<Track> kept;
	for (auto& t : queue) if (!isGone(t.path)) kept.push_back(std::move(t));
	queue = std::move(kept);
	order.clear();
	current = -1;
	for (int i = 0; i < (int)queue.size(); ++i) if (queue[i].path == playing) { current = i; break; }
	rebuildOrder();
	recentPlayed.erase(std::remove_if(recentPlayed.begin(), recentPlayed.end(), isGone), recentPlayed.end());

	const auto& all = pls.all();
	for (int p = 0; p < (int)all.size(); ++p)
	{
		for (int pos = (int)all[p].paths.size() - 1; pos >= 0; --pos)
		{
			std::wstring abs = absOf(all[p].paths[pos]);
			if (isGone(abs)) pls.removeAt(p, pos);
		}
	}
	refreshLibrary();
	wchar_t buf[128];
	if (hid && !recycling)
		swprintf(buf, 128, hid == 1 ? L"Hid 1 song. The file is still in its folder" : L"Hid %d songs. The files are still in their folders", hid);
	else if (removed == recycling && !hid)
		swprintf(buf, 128, removed == 1 ? L"Moved 1 song to the Recycle Bin" : L"Moved %d songs to the Recycle Bin", removed);
	else if (removed == recycling)
		swprintf(buf, 128, L"Moved %d to the Recycle Bin, hid %d from your folders", removed, hid);
	else
		swprintf(buf, 128, L"Moved %d of %d songs; the rest are in use or locked", removed, recycling);
	notice = buf;
	saveState();
	updateTimer();
	InvalidateRect(hwnd, nullptr, FALSE);
}

const Group* App::albumOf(const std::wstring& path) const
{
	for (const Group& al : albums)
		for (int i : al.tracks)
			if (libTracks[i].path == path) return &al;
	return nullptr;
}

void App::setAlbumCover(const std::vector<std::wstring>& paths)
{
	if (paths.empty() || busyImporting()) return;
	std::vector<wchar_t> buf(MAX_PATH * 4, L'\0');
	OPENFILENAMEW ofn = { sizeof(ofn) };
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = L"Images\0*.jpg;*.jpeg;*.png\0";
	ofn.lpstrFile = buf.data();
	ofn.nMaxFile = (DWORD)buf.size();
	ofn.lpstrTitle = L"Choose a cover image";
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
	if (!GetOpenFileNameW(&ofn)) return;

	std::vector<unsigned char> bytes;
	FILE* f = nullptr;
	if (_wfopen_s(&f, buf.data(), L"rb") == 0 && f)
	{
		fseek(f, 0, SEEK_END);
		long n = ftell(f);
		fseek(f, 0, SEEK_SET);
		if (n > 0 && n < 64L * 1024 * 1024)
		{
			bytes.resize((size_t)n);
			if (fread(bytes.data(), 1, bytes.size(), f) != bytes.size()) bytes.clear();
		}
		fclose(f);
	}
	bytes = g.shrinkImage(bytes, 1200);
	if (bytes.empty()) { notice = L"That image couldn't be read"; InvalidateRect(hwnd, nullptr, FALSE); return; }

	coverPaths.clear();
	for (const auto& p : paths) if (inLibrary(p)) coverPaths.push_back(p);
	if (coverPaths.empty()) return;

	resumeAfterCover = false;
	if (current >= 0 && std::find(coverPaths.begin(), coverPaths.end(), queue[current].path) != coverPaths.end())
	{
		resumeAfterCover = player.getState() == State::Playing;
		resumeAt = player.position();
		resumePath = queue[current].path;
		player.uninit();
		audioOk = player.init(0, 0);
		if (audioOk) player.setVolume(volume);
	}
	else resumePath.clear();

	coverBusy = true;
	coverFailed = 0;
	coverError.clear();
	notice = L"Setting the cover\x2026";
	coverThread = std::thread([this, bytes]() {
		for (const auto& p : coverPaths)
		{
			std::string err;
			if (!writeCover(narrow(p), bytes, err)) { ++coverFailed; coverError = err; }
		}
		PostMessageW(hwnd, WM_COVER_DONE, 0, 0);
	});
	InvalidateRect(hwnd, nullptr, FALSE);
}

void App::finishCover()
{
	if (coverThread.joinable()) coverThread.join();
	coverBusy = false;
	for (const auto& p : coverPaths)
	{
		covers.erase(p);
		for (Track& t : queue) if (t.path == p) { t.coverBytes.clear(); t.coverLoaded = false; }
	}
	coverFor = -1;
	if (!resumePath.empty() && current >= 0 && queue[current].path == resumePath && audioOk)
	{
		if (player.load(fsPath(resumePath)))
		{
			player.seek(resumeAt);
			if (resumeAfterCover) player.play();
		}
	}
	int ok = (int)coverPaths.size() - coverFailed;
	notice = ok == 1 ? L"Cover saved into 1 file" : L"Cover saved into " + std::to_wstring(ok) + L" files";
	if (coverFailed) notice += L", " + std::to_wstring(coverFailed) + L" failed: " + widen(coverError);
	updateTimer();
	InvalidateRect(hwnd, nullptr, FALSE);
}

std::vector<int> App::smartTracks() const
{
	std::vector<int> out;
	for (int i = 0; i < (int)libTracks.size(); ++i)
	{
		std::wstring e = extUpper(libTracks[i].path);
		bool lossless = e == L"FLAC" || e == L"WAV" || e == L"AIF" || e == L"AIFF" || e == L"WV" || e == L"APE";
		if (lossless && libTracks[i].info.bitsPerSample >= 24) out.push_back(i);
	}
	return out;
}

const std::wstring& App::lyricsFor(const Track& t)
{
	auto it = lyricsByPath.find(t.path);
	if (it == lyricsByPath.end()) it = lyricsByPath.emplace(t.path, widen(t.info.lyrics)).first;
	return it->second;
}

int App::lyricLineAt(float y) const
{
	if (lyricTops.empty()) return -1;
	float cy = y - lyricArea.y + lyricScroll;
	int i = (int)(std::upper_bound(lyricTops.begin(), lyricTops.end(), cy) - lyricTops.begin()) - 1;
	return i < 0 ? 0 : i;
}

void App::copyLyrics()
{
	if (lyricLines.empty()) return;
	int a = 0, b = (int)lyricLines.size() - 1;
	if (lyricSelA >= 0) { a = std::min(lyricSelA, lyricSelB); b = std::max(lyricSelA, lyricSelB); }
	std::wstring text;
	for (int i = a; i <= b && i < (int)lyricLines.size(); ++i) text += lyricLines[i] + (i < b ? L"\r\n" : L"");
	if (!OpenClipboard(hwnd)) return;
	EmptyClipboard();
	if (HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, (text.size() + 1) * sizeof(wchar_t)))
	{
		memcpy(GlobalLock(mem), text.c_str(), (text.size() + 1) * sizeof(wchar_t));
		GlobalUnlock(mem);
		if (!SetClipboardData(CF_UNICODETEXT, mem)) GlobalFree(mem);
	}
	CloseClipboard();
	notice = a == b && lyricSelA >= 0 ? L"Copied 1 line" : lyricSelA >= 0 ? L"Copied " + std::to_wstring(b - a + 1) + L" lines" : L"Copied the lyrics";
	InvalidateRect(hwnd, nullptr, FALSE);
}

void App::saveLyrics(const std::wstring& raw)
{
	if (current < 0 || busyImporting()) return;
	std::wstring path = queue[current].path;
	if (!inLibrary(path)) { notice = L"Add the song to your library to save lyrics"; InvalidateRect(hwnd, nullptr, FALSE); return; }
	std::wstring text;
	for (wchar_t c : raw) if (c != L'\r') text += c;
	while (!text.empty() && (text.back() == L'\n' || text.back() == L' ')) text.pop_back();

	// The decoder has the file open; let go of it while the tags are rewritten.
	bool wasPlaying = player.getState() == State::Playing;
	double at = player.position();
	player.uninit();
	audioOk = player.init(0, 0);
	if (audioOk) player.setVolume(volume);
	std::string err;
	bool ok = writeLyrics(narrow(path), narrow(text), err);
	if (audioOk && player.load(fsPath(path)))
	{
		player.seek(at);
		if (wasPlaying) player.play();
	}
	if (ok)
	{
		lyricsByPath[path] = text;
		lyricSelA = lyricSelB = -1;
		lyricScroll = 0;
		notice = text.empty() ? L"Lyrics removed from the file" : L"Lyrics saved into the song file";
	}
	else notice = L"Couldn't save the lyrics: " + widen(err);
	updateTimer();
	InvalidateRect(hwnd, nullptr, FALSE);
}

void App::pasteLyrics()
{
	std::wstring text;
	if (OpenClipboard(hwnd))
	{
		if (HANDLE h = GetClipboardData(CF_UNICODETEXT))
			if (const wchar_t* p = (const wchar_t*)GlobalLock(h)) { text = p; GlobalUnlock(h); }
		CloseClipboard();
	}
	bool blank = text.find_first_not_of(L" \t\r\n") == std::wstring::npos;
	if (blank) { notice = L"Copy the lyrics first, then paste them here"; InvalidateRect(hwnd, nullptr, FALSE); return; }
	saveLyrics(text);
}

void App::loadLyricsFile()
{
	std::vector<wchar_t> buf(MAX_PATH * 4, L'\0');
	OPENFILENAMEW ofn = { sizeof(ofn) };
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = L"Lyrics\0*.lrc;*.txt\0";
	ofn.lpstrFile = buf.data();
	ofn.nMaxFile = (DWORD)buf.size();
	ofn.lpstrTitle = L"Choose a lyrics file";
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
	if (!GetOpenFileNameW(&ofn)) return;
	std::string bytes;
	FILE* f = nullptr;
	if (_wfopen_s(&f, buf.data(), L"rb") == 0 && f)
	{
		char chunk[4096];
		size_t n;
		while ((n = fread(chunk, 1, sizeof chunk, f)) > 0 && bytes.size() < (1 << 20)) bytes.append(chunk, n);
		fclose(f);
	}
	if (bytes.size() >= 3 && bytes.compare(0, 3, "\xEF\xBB\xBF") == 0) bytes.erase(0, 3);
	// UTF-8 if it decodes as UTF-8, otherwise the system code page (older .lrc files)
	UINT cp = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(), (int)bytes.size(), nullptr, 0) > 0 ? CP_UTF8 : CP_ACP;
	int n = MultiByteToWideChar(cp, 0, bytes.data(), (int)bytes.size(), nullptr, 0);
	std::wstring text(n, L'\0');
	MultiByteToWideChar(cp, 0, bytes.data(), (int)bytes.size(), &text[0], n);
	if (text.find_first_not_of(L" \t\r\n") == std::wstring::npos) { notice = L"That file is empty"; InvalidateRect(hwnd, nullptr, FALSE); return; }
	saveLyrics(text);
}

void App::wheelScroll(float& value, float max, float delta, bool precise)
{
	for (Smooth& s : smooths)
	{
		if (s.value != &value) continue;
		bool moving = std::fabs(s.target - value) > 0.5f && s.last == value;
		float t = (moving ? s.target : value) - delta;   // keep adding to a scroll in flight
		t = t < 0 ? 0 : t > max ? max : t;
		s.target = t;
		if (precise) { value = t; s.last = t; }
		lastScrollFrame = nowMs();
		InvalidateRect(hwnd, nullptr, FALSE);
		return;
	}
}

void App::stepScroll()
{
	double now = nowMs();
	float dt = (float)(now - lastScrollFrame);
	if (dt > 50) dt = 50;   // first frame after a pause
	lastScrollFrame = now;
	scrollMoving = false;
	float k = 1 - std::exp(-dt / 65.0f);
	for (Smooth& s : smooths)
	{
		if (*s.value != s.last) s.target = *s.value;   // set elsewhere (page change, clamping, scrollbar drag)
		float d = s.target - *s.value;
		if (std::fabs(d) < 0.5f) *s.value = s.target;
		else { *s.value += d * k; scrollMoving = true; }
		s.last = *s.value;
	}
}

bool App::busyImporting()
{
	if (!importing && !coverBusy && !scanning && !moving) return false;
	notice = moving ? L"Wait for the LossCat folder to finish moving" : scanning ? L"Wait for the scan to finish" : L"Wait for the import to finish";
	InvalidateRect(hwnd, nullptr, FALSE);
	return true;
}

void App::importFiles(const std::vector<std::wstring>& filesIn)
{
	if (filesIn.empty() || busyImporting()) return;
	// songs already in a music folder are in the library as they are; no copy needed
	std::vector<std::wstring> files;
	for (const auto& f : filesIn) if (!inLibrary(f)) files.push_back(f);
	if (files.empty())
	{
		notice = filesIn.size() == 1 ? L"That song is already in your library" : L"Those songs are already in your library";
		InvalidateRect(hwnd, nullptr, FALSE);
		return;
	}
	importResults.clear();
	importDone = 0;
	importTotal = (int)files.size();
	importCancel = false;

	auto work = [this, files]() {
		for (const auto& f : files)
		{
			if (importCancel) break;
			ImportResult r{ f, {}, {} };
			int idx = lib.importFile(narrow(f), r.error);
			if (idx >= 0) r.copy = lib.tracks()[idx].path;
			importResults.push_back(std::move(r));
			++importDone;
		}
	};

	if (files.size() == 1)
	{
		work();          // one song: quick enough to do right here
		finishImport();
		return;
	}
	importing = true;
	notice.clear();
	importThread = std::thread([this, work]() {
		work();
		PostMessageW(hwnd, WM_IMPORT_DONE, 0, 0);
	});
	updateTimer();
	InvalidateRect(hwnd, nullptr, FALSE);
}

void App::finishImport()
{
	if (importThread.joinable()) importThread.join();
	importing = false;
	int ok = 0, failed = 0, skipped = 0;
	std::string lastError;
	for (const ImportResult& r : importResults)
	{
		if (r.copy.empty())
		{
			if (r.error == "Not an audio file") ++skipped;
			else { ++failed; lastError = r.error; }
			continue;
		}
		++ok;
		std::wstring copy = widen(r.copy);
		for (Track& t : queue) if (t.path == r.source) t.path = copy;
		for (std::wstring& p : recentPlayed) if (p == r.source) p = copy;
	}
	refreshLibrary();
	notice = ok == 1 ? L"Added 1 song" : L"Added " + std::to_wstring(ok) + L" songs";
	if (skipped) notice += L", skipped " + std::to_wstring(skipped) + L" non-audio";
	if (failed) notice += L", " + std::to_wstring(failed) + L" failed: " + widen(lastError);
	if (importCancel) notice += L" (cancelled)";
	importResults.clear();
	updateTimer();
	InvalidateRect(hwnd, nullptr, FALSE);
}

void App::addFiles(const std::vector<std::wstring>& files, bool playFirst)
{
	int firstNew = (int)queue.size();
	for (const auto& f : files)
	{
		DWORD attr = GetFileAttributesW(f.c_str());
		if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY)) continue;
		queue.push_back(makeTrack(f, readTrackInfo(fsPath(f)), true));
	}
	if (playFirst && firstNew < (int)queue.size() && player.getState() != State::Playing)
	{
		playIndex(firstNew);
		page = Page::Now;
	}
	InvalidateRect(hwnd, nullptr, FALSE);
}

void App::playIndex(int i)
{
	if (i < 0 || i >= (int)queue.size()) return;
	current = i;
	ensureOrder();
	orderPos = (int)(std::find(order.begin(), order.end(), i) - order.begin());
	error.clear();
	Track& t = queue[i];
	if (!t.coverLoaded)
	{
		t.coverBytes = std::move(readTrackInfo(fsPath(t.path)).coverData);
		t.coverLoaded = true;
	}
	if (!audioOk) { error = L"No audio output device"; return; }
	if (!canPlay(t.path))
	{
		player.pause();
		error = L"LossCat can't play " + extUpper(t.path) + L" files yet. We're working on it.";
	}
	else if (!player.load(fsPath(t.path)))
	{
		error = L"Couldn't open this file";
	}
	else
	{
		player.play();
	}
	auto old = std::find(recentPlayed.begin(), recentPlayed.end(), t.path);
	if (old != recentPlayed.end()) recentPlayed.erase(old);
	recentPlayed.insert(recentPlayed.begin(), t.path);
	if (recentPlayed.size() > 12) recentPlayed.pop_back();

	std::wstring title = queue[i].title + L" - LossCat";
	SetWindowTextW(hwnd, title.c_str());   // taskbar and Alt+Tab
	updateTimer();
	InvalidateRect(hwnd, nullptr, FALSE);
}

void App::rebuildOrder()
{
	int n = (int)queue.size();
	order.resize(n);
	for (int i = 0; i < n; ++i) order[i] = i;
	if (shuffle && n > 1)
	{
		if (current >= 0) std::swap(order[0], order[current]);
		std::shuffle(order.begin() + (current >= 0 ? 1 : 0), order.end(), rng);
		orderPos = current >= 0 ? 0 : -1;
	}
	else
	{
		orderPos = current;
	}
}

void App::ensureOrder()
{
	if (order.size() != queue.size()) rebuildOrder();
}

std::vector<int> App::upcoming() const
{
	std::vector<int> out;
	if (orderPos < 0 && current >= 0) return out;
	for (size_t i = (size_t)(orderPos + 1); i < order.size(); ++i) out.push_back(order[i]);
	return out;
}

void App::next(bool userAction)
{
	if (queue.empty()) return;
	if (repeat == 2 && !userAction) { playIndex(current); return; }
	ensureOrder();
	int pos = orderPos + 1;
	int n = (int)order.size();
	if (pos < n) { playIndex(order[pos]); return; }
	if (repeat == 1)
	{
		if (shuffle && n > 1)
		{
			std::shuffle(order.begin(), order.end(), rng);
			if (order[0] == current) std::swap(order[0], order[n - 1]);
		}
		playIndex(order[0]);
		return;
	}
	if (!userAction) { player.pause(); updateTimer(); InvalidateRect(hwnd, nullptr, FALSE); }
}

void App::prev()
{
	if (current < 0) return;
	ensureOrder();
	int pos = orderPos - 1;
	if (player.position() > 3 || pos < 0)
	{
		if (pos < 0 && repeat == 1 && player.position() <= 3 && !order.empty()) playIndex(order.back());
		else seekTo(0);
		return;
	}
	playIndex(order[pos]);
}

void App::togglePlay()
{
	if (current < 0)
	{
		if (!queue.empty()) playIndex(0);
		else openFileDialog();
		return;
	}
	if (player.getState() == State::Playing) player.pause();
	else player.play();
	updateTimer();
	InvalidateRect(hwnd, nullptr, FALSE);
}

void App::seekTo(double seconds)
{
	if (current < 0) return;
	double d = player.duration();
	if (seconds < 0) seconds = 0;
	if (d > 0 && seconds > d - 0.05) seconds = d - 0.05;
	player.seek(seconds);
	InvalidateRect(hwnd, nullptr, FALSE);
}

void App::openFileDialog()
{
	std::vector<std::wstring> files = pickFiles();
	if (!files.empty()) addFiles(files, true);
}

std::vector<std::wstring> App::pickFiles()
{
	std::vector<std::wstring> files;
	std::vector<wchar_t> buf(64 * 1024, L'\0');
	OPENFILENAMEW ofn = { sizeof(ofn) };
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = L"Audio files\0*.flac;*.wav;*.mp3;*.aif;*.aiff\0All files\0*.*\0";
	ofn.lpstrFile = buf.data();
	ofn.nMaxFile = (DWORD)buf.size();
	ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_FILEMUSTEXIST;
	if (!GetOpenFileNameW(&ofn)) return files;

	// Multi-select returns "folder\0file1\0file2\0\0"; single select returns the full path.
	std::wstring first = buf.data();
	const wchar_t* p = buf.data() + first.size() + 1;
	if (*p == L'\0')
	{
		files.push_back(first);
	}
	else
	{
		while (*p)
		{
			std::wstring name = p;
			files.push_back(first + L"\\" + name);
			p += name.size() + 1;
		}
	}
	return files;
}

void App::tick()
{
	if (current >= 0 && player.isFinished()) next(false);
	if (nowMs() - lastSaveMs > 30000) saveState();   // so a crash or power cut loses little
	InvalidateRect(hwnd, nullptr, FALSE);
	updateTimer();
}

void App::updateTimer()
{
	// Animation frames are driven by paint() itself (see there), so the timer is only
	// for the clock and seek bar while playing.
	UINT want = player.getState() == State::Playing || importing ? 200 : 0;
	if (want == timerMs) return;
	if (want) SetTimer(hwnd, kTimer, want, nullptr);
	else KillTimer(hwnd, kTimer);
	timerMs = want;
}

// ---------------------------------------------------------------- saved state

std::wstring App::statePath() const
{
	PWSTR dir = nullptr;
	std::wstring path;
	if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &dir)))
		path = std::wstring(dir) + L"\\LossCat\\state.txt";
	CoTaskMemFree(dir);
	return path;
}

void App::saveState()
{
	lastSaveMs = nowMs();
	std::wstring path = statePath();
	if (path.empty()) return;
	CreateDirectoryW(path.substr(0, path.find_last_of(L'\\')).c_str(), nullptr);

	std::string s = "LossCat state 1\n";
	char num[64];
	snprintf(num, sizeof num, "%.3f", volume);
	s += "volume=" + std::string(num) + "\n";
	s += "shuffle=" + std::to_string(shuffle ? 1 : 0) + "\n";
	s += "repeat=" + std::to_string(repeat) + "\n";
	s += "sidebar=" + std::string(collapsed ? "collapsed" : "open") + "\n";
	s += "queuePanel=" + std::string(queueOpen ? "open" : "closed") + "\n";
	if (current >= 0)
	{
		s += "current=" + std::to_string(current) + "\n";
		snprintf(num, sizeof num, "%.2f", player.getState() == State::NoTrack ? 0.0 : player.position());
		s += "position=" + std::string(num) + "\n";
	}
	if (shuffle && order.size() == queue.size())
	{
		s += "order=";
		for (size_t i = 0; i < order.size(); ++i) s += (i ? "," : "") + std::to_string(order[i]);
		s += "\n";
	}
	for (const Track& t : queue) s += "track=" + narrow(t.path) + "\n";
	for (const std::wstring& p : recentPlayed) s += "recent=" + narrow(p) + "\n";
	for (const std::wstring& f : musicFolders) s += "folder=" + narrow(f) + "\n";
	if (!customLibraryDir.empty()) s += "libraryFolder=" + narrow(customLibraryDir) + "\n";
	for (const std::wstring& h : hiddenFiles) s += "hidden=" + narrow(h) + "\n";

	// Write a temporary file and swap it in, so a crash mid-write can't leave a broken state.
	std::wstring tmp = path + L".tmp";
	FILE* f = nullptr;
	if (_wfopen_s(&f, tmp.c_str(), L"wb") != 0 || !f) return;
	bool ok = fwrite(s.data(), 1, s.size(), f) == s.size();
	ok = fclose(f) == 0 && ok;
	if (ok) MoveFileExW(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
	else DeleteFileW(tmp.c_str());
}

void App::loadState()
{
	lastSaveMs = nowMs();
	std::wstring path = statePath();
	FILE* f = nullptr;
	std::string data;
	if (!path.empty() && _wfopen_s(&f, path.c_str(), L"rb") == 0 && f)
	{
		char chunk[8192];
		size_t n;
		while ((n = fread(chunk, 1, sizeof chunk, f)) > 0) data.append(chunk, n);
		fclose(f);
	}

	int savedCurrent = -1;
	double position = 0;
	std::vector<int> savedOrder;
	std::vector<std::wstring> tracks;
	recentPlayed.clear();
	size_t start = 0;
	while (start < data.size())
	{
		size_t end = data.find('\n', start);
		if (end == std::string::npos) end = data.size();
		std::string line = data.substr(start, end - start);
		start = end + 1;
		if (!line.empty() && line.back() == '\r') line.pop_back();
		size_t eq = line.find('=');
		if (eq == std::string::npos) continue;   // header, or a line from a newer version
		std::string key = line.substr(0, eq), val = line.substr(eq + 1);
		if (key == "volume") { volume = (float)atof(val.c_str()); if (volume < 0) volume = 0; if (volume > 1) volume = 1; }
		else if (key == "shuffle") shuffle = val == "1";
		else if (key == "repeat") { repeat = atoi(val.c_str()); if (repeat < 0 || repeat > 2) repeat = 0; }
		else if (key == "sidebar") collapsed = val == "collapsed";
		else if (key == "queuePanel") queueOpen = val == "open";
		else if (key == "current") savedCurrent = atoi(val.c_str());
		else if (key == "position") position = atof(val.c_str());
		else if (key == "order")
		{
			for (size_t p = 0; p < val.size();)
			{
				savedOrder.push_back(atoi(val.c_str() + p));
				size_t c = val.find(',', p);
				if (c == std::string::npos) break;
				p = c + 1;
			}
		}
		else if (key == "track" && tracks.size() < 5000) tracks.push_back(widen(val));
		else if (key == "folder") musicFolders.push_back(widen(val));
		else if (key == "libraryFolder") customLibraryDir = widen(val);
		else if (key == "hidden") hiddenFiles.push_back(widen(val));
		else if (key == "recent" && recentPlayed.size() < 12)
		{
			std::wstring p = widen(val);
			if (GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES) recentPlayed.push_back(p);
		}
	}
	if (audioOk) player.setVolume(volume);
	loadLibrary();   // needs the music folders read above
	if (data.empty()) return;

	// Rebuild the queue, skipping files that have gone. Library songs reuse the tags already read.
	std::map<std::wstring, int> libIndex;
	for (int i = 0; i < (int)libTracks.size(); ++i) libIndex[libTracks[i].path] = i;
	std::vector<int> newIndex(tracks.size(), -1);
	queue.clear();
	for (size_t i = 0; i < tracks.size(); ++i)
	{
		const std::wstring& p = tracks[i];
		auto lib = libIndex.find(p);
		if (lib != libIndex.end()) queue.push_back(libTracks[lib->second]);
		else if (GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES) queue.push_back(makeTrack(p, readTrackInfo(fsPath(p)), false));
		else continue;
		newIndex[i] = (int)queue.size() - 1;
	}
	current = savedCurrent >= 0 && savedCurrent < (int)newIndex.size() ? newIndex[savedCurrent] : -1;
	if (current < 0 && !queue.empty()) { current = 0; position = 0; }

	order.clear();
	if (shuffle && savedOrder.size() == tracks.size())
	{
		for (int o : savedOrder)
			if (o >= 0 && o < (int)newIndex.size() && newIndex[o] >= 0) order.push_back(newIndex[o]);
		if (order.size() != queue.size()) order.clear();
	}
	if (order.empty()) rebuildOrder();
	orderPos = (int)(std::find(order.begin(), order.end(), current) - order.begin());
	if (orderPos >= (int)order.size()) orderPos = -1;

	// Back where it was, paused.
	if (current >= 0)
	{
		Track& t = queue[current];
		t.coverBytes = std::move(readTrackInfo(fsPath(t.path)).coverData);
		t.coverLoaded = true;
		if (position > t.info.durationMs / 1000.0 - 2) position = 0;   // it had finished; start it over
		if (audioOk && canPlay(t.path) && player.load(fsPath(t.path)) && position > 0) player.seek(position);
		std::wstring title = t.title + L" - LossCat";
		SetWindowTextW(hwnd, title.c_str());
	}
}
