#include "app.h"
#include "names.h"
#include <windowsx.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <commdlg.h>
#include <shlobj.h>
#include <knownfolders.h>
#include <cstdlib>
#include <iterator>
#include <cwctype>
#include <map>
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

static const wchar_t* kClassName = L"LossCat";
static const UINT_PTR kTimer = 1;


std::string fsPath(const std::wstring& path)
{
	wchar_t shortPath[MAX_PATH * 2];
	DWORD n = GetShortPathNameW(path.c_str(), shortPath, (DWORD)std::size(shortPath));
	const wchar_t* use = (n > 0 && n < std::size(shortPath)) ? shortPath : path.c_str();
	BOOL lossy = FALSE;
	int len = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, use, -1, nullptr, 0, nullptr, &lossy);
	if (len <= 0 || lossy) return narrow(path);
	std::string out(len - 1, '\0');
	WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, use, -1, out.data(), len, nullptr, nullptr);
	return out;
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
	wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
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

	loadLibrary();

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
		if (dragging)
		{
			if (pressHit == Hit::Seek && seekBar.w > 0)
			{
				float f = (x - seekBar.x) / seekBar.w;
				f = f < 0 ? 0 : f > 1 ? 1 : f;
				dragSeconds = f * player.duration();
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
		if (pressHit == Hit::Seek || pressHit == Hit::Volume)
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
		if (dragging)
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
			float step = GET_WHEEL_DELTA_WPARAM(wp) / 120.0f * 90.0f;
			if (sideList.contains(x, y))
			{
				sideScroll -= step;
				if (sideScroll < 0) sideScroll = 0;
				if (sideScroll > sideMaxScroll) sideScroll = sideMaxScroll;
				InvalidateRect(hwnd, nullptr, FALSE);
			}
			else if (listArea.contains(x, y))
			{
				listScroll -= step;
				if (listScroll < 0) listScroll = 0;
				if (listScroll > listMaxScroll) listScroll = listMaxScroll;
				InvalidateRect(hwnd, nullptr, FALSE);
			}
			else if (pageArea.contains(x, y))
			{
				pageScroll -= step;
				if (pageScroll < 0) pageScroll = 0;
				if (pageScroll > pageMaxScroll) pageScroll = pageMaxScroll;
				InvalidateRect(hwnd, nullptr, FALSE);
			}
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
		if (wp == VK_SPACE) togglePlay();
		else if (wp == VK_ESCAPE && queueOpen) click(Hit::QueueButton, -1);
		else if (wp == VK_LEFT) seekTo(player.position() - 5);
		else if (wp == VK_RIGHT) seekTo(player.position() + 5);
		else if (ctrl && wp == 'O') openFileDialog();
		else if (ctrl && wp == VK_OEM_COMMA) { page = Page::Settings; InvalidateRect(hwnd, nullptr, FALSE); }
		else if (ctrl && wp == 'B') click(Hit::NavToggle, -1);
		else if (ctrl && wp == 'L') click(Hit::BarFavourite, -1);
		else if (wp == VK_F5) loadLibrary();
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
		tick();
		return 0;
	case WM_DESTROY:
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
	case Hit::AlbumTile: if (index < (int)albums.size()) playList(albums[index].tracks, 0); break;
	case Hit::ArtistTile: if (index < (int)artists.size()) playList(artists[index].tracks, 0); break;
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
				wchar_t t[96];
				if (paths.size() == 1) swprintf(t, 96, L"Move this song to the Recycle Bin?");
				else swprintf(t, 96, L"Move %zu songs to the Recycle Bin?", paths.size());
				confirmTitle = t;
				confirmOpen = true;
			}
		}
		break;
	case Hit::ConfirmOk:
		confirmOpen = false;
		if (pendingDeletePlaylist > 0)
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
	case Hit::RowHeart:
		if (index >= 0 && index < (int)rowHeartPaths.size()) toggleFavourite(rowHeartPaths[index]);
		break;
	case Hit::LibTabPlaylists: libTab = 3; pageScroll = 0; break;
	case Hit::PlaylistTile: openPlaylist = index; page = Page::Playlist; break;
	case Hit::ImportFiles: { auto f = pickFiles(); if (!f.empty()) importFiles(f); break; }
	case Hit::Rescan: loadLibrary(); notice.clear(); break;
	case Hit::AddToLibrary: if (current >= 0 && !inLibrary(queue[current].path)) importFiles({ queue[current].path }); break;
	case Hit::Settings: case Hit::SignalPath: page = Page::Settings; break;
	case Hit::WinMin: ShowWindow(hwnd, SW_MINIMIZE); break;
	case Hit::WinClose: PostMessageW(hwnd, WM_CLOSE, 0, 0); break;
	case Hit::TabNext: npTab = 0; break;
	case Hit::TabAbout: npTab = 1; break;
	case Hit::TabLyrics: npTab = 2; break;
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
	default: break;
	}
	updateTimer();
	InvalidateRect(hwnd, nullptr, FALSE);
}

void App::loadLibrary()
{
	PWSTR music = nullptr;
	if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Music, 0, nullptr, &music)))
	{
		libraryDir = std::wstring(music) + L"\\LossCat";
		CoTaskMemFree(music);
	}
	if (libraryDir.empty()) return;

	if (!lib.open(narrow(libraryDir))) return;
	pls.load(narrow(libraryDir));
	refreshLibrary();
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

void App::playList(const std::vector<int>& idx, int start)
{
	if (idx.empty()) return;
	queue.clear();
	order.clear();
	for (int i : idx) queue.push_back(libTracks[i]);
	playIndex(start);
}

bool App::inLibrary(const std::wstring& path) const
{
	if (libraryDir.empty() || path.size() <= libraryDir.size()) return false;
	return _wcsnicmp(path.c_str(), libraryDir.c_str(), libraryDir.size()) == 0
		&& (path[libraryDir.size()] == L'\\' || path[libraryDir.size()] == L'/');
}

void App::openMenu(Hit target, int index, float x, float y)
{
	menu.clear();
	menuPaths.clear();
	auto songPath = [&]() -> std::wstring {
		switch (target)
		{
		case Hit::LibraryRow: return index < (int)libTracks.size() ? libTracks[index].path : L"";
		case Hit::AddedTile: return index < (int)recentlyAdded.size() ? libTracks[recentlyAdded[index]].path : L"";
		case Hit::RecentTile: return index < (int)recentPlayed.size() ? recentPlayed[index] : L"";
		case Hit::QueueRow: return index < (int)queue.size() ? queue[index].path : L"";
		case Hit::PlaylistRow: { const auto& l = playlistTracks(openPlaylist); return index < (int)l.size() ? l[index].path : L""; }
		case Hit::Cover: case Hit::AddToLibrary: return current >= 0 ? queue[current].path : L"";
		default: return L"";
		}
	};
	if (target == Hit::PlaylistTile) target = Hit::PlaylistNav;
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
		menu.push_back({ L"Add to playlist\x2026", Act::PlaylistMenu, false });
		menu.push_back({ target == Hit::AlbumTile ? L"Remove album from library" : L"Remove artist from library", Act::Remove, true });
	}
	else
	{
		std::wstring p = songPath();
		if (p.empty()) { if (menuOpen) { menuOpen = false; InvalidateRect(hwnd, nullptr, FALSE); } return; }
		menuPaths.push_back(p);
		menu.push_back({ L"Play", Act::Play, false });
		if (inLibrary(p))
		{
			bool fav = inPlaylist(0, p);
			menu.push_back({ fav ? L"Remove from Favourites" : L"Add to Favourites", fav ? Act::Unfavourite : Act::Favourite, false });
			menu.push_back({ L"Add to playlist\x2026", Act::PlaylistMenu, false });
			if (target == Hit::PlaylistRow && menuPlaylist >= 0)
				menu.push_back({ L"Remove from this playlist", Act::RemoveFromPlaylist, false });
			menu.push_back({ L"Remove from library", Act::Remove, true });
		}
		else menu.push_back({ L"Add to library", Act::AddToLibrary, false });
	}
	menuX = x;
	menuY = y;
	menuOpen = true;
	InvalidateRect(hwnd, nullptr, FALSE);
}

std::wstring App::absOf(const std::string& rel) const
{
	return libraryDir + L"\\" + widen(rel);
}

std::string App::relOf(const std::wstring& abs) const
{
	if (!inLibrary(abs)) return {};
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
	std::wstring original = path;
	importFiles({ original });
	if (current >= 0 && !queue.empty()) path = queue[current].path;
	return inLibrary(path);
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
	if (paths.empty()) return;
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

	std::wstring list;
	for (const auto& p : paths) { list += p; list.push_back(L'\0'); }
	list.push_back(L'\0');
	SHFILEOPSTRUCTW op = {};
	op.hwnd = hwnd;
	op.wFunc = FO_DELETE;
	op.pFrom = list.c_str();
	op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
	SHFileOperationW(&op);

	int removed = 0;
	for (const auto& p : paths)
	{
		if (GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES) continue;
		std::string u = narrow(p);
		const auto& tracks = lib.tracks();
		for (int i = 0; i < (int)tracks.size(); ++i)
		{
			if (tracks[i].path == u) { lib.forget(i); ++removed; break; }
		}
		covers.erase(p);
	}

	std::wstring playing = current >= 0 ? queue[current].path : L"";
	std::vector<Track> kept;
	for (auto& t : queue) if (!removing(t.path) || GetFileAttributesW(t.path.c_str()) != INVALID_FILE_ATTRIBUTES) kept.push_back(std::move(t));
	queue = std::move(kept);
	order.clear();
	current = -1;
	for (int i = 0; i < (int)queue.size(); ++i) if (queue[i].path == playing) { current = i; break; }
	rebuildOrder();
	recentPlayed.erase(std::remove_if(recentPlayed.begin(), recentPlayed.end(), [&](const std::wstring& p) {
		return removing(p) && GetFileAttributesW(p.c_str()) == INVALID_FILE_ATTRIBUTES; }), recentPlayed.end());

	const auto& all = pls.all();
	for (int p = 0; p < (int)all.size(); ++p)
	{
		for (int pos = (int)all[p].paths.size() - 1; pos >= 0; --pos)
		{
			std::wstring abs = absOf(all[p].paths[pos]);
			if (removing(abs) && GetFileAttributesW(abs.c_str()) == INVALID_FILE_ATTRIBUTES) pls.removeAt(p, pos);
		}
	}
	refreshLibrary();
	wchar_t buf[96];
	if (removed == (int)paths.size())
		swprintf(buf, 96, removed == 1 ? L"Moved 1 song to the Recycle Bin" : L"Moved %d songs to the Recycle Bin", removed);
	else
		swprintf(buf, 96, L"Moved %d of %zu songs; the rest are in use or locked", removed, paths.size());
	notice = buf;
	updateTimer();
	InvalidateRect(hwnd, nullptr, FALSE);
}

void App::importFiles(const std::vector<std::wstring>& files)
{
	int ok = 0, failed = 0;
	std::string lastError;
	for (const auto& f : files)
	{
		std::string err;
		int idx = lib.importFile(narrow(f), err);
		if (idx < 0) { ++failed; lastError = err; continue; }
		++ok;
		std::wstring copy = widen(lib.tracks()[idx].path);
		for (Track& t : queue) if (t.path == f) t.path = copy;
		for (std::wstring& r : recentPlayed) if (r == f) r = copy;
	}
	refreshLibrary();
	wchar_t buf[96];
	if (failed == 0) swprintf(buf, 96, ok == 1 ? L"Added 1 song" : L"Added %d songs", ok);
	else swprintf(buf, 96, L"Added %d, %d failed: ", ok, failed);
	notice = buf;
	if (failed) notice += widen(lastError);
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
	InvalidateRect(hwnd, nullptr, FALSE);
	updateTimer();
}

void App::updateTimer()
{
	// Animation frames are driven by paint() itself (see there), so the timer is only
	// for the clock and seek bar while playing.
	UINT want = player.getState() == State::Playing ? 200 : 0;
	if (want == timerMs) return;
	if (want) SetTimer(hwnd, kTimer, want, nullptr);
	else KillTimer(hwnd, kTimer);
	timerMs = want;
}
