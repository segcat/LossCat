#include "app.h"
#include <windowsx.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <commdlg.h>
#include <cstdlib>
#include <iterator>

#pragma comment(lib, "dwmapi")
#pragma comment(lib, "shell32")
#pragma comment(lib, "comdlg32")

// Not in older SDK headers.
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#define LC_DWMWA_WINDOW_CORNER_PREFERENCE 33
#define LC_DWMWA_BORDER_COLOR 34
#define LC_DWMWCP_ROUND 2

static const wchar_t* kClassName = L"LossCat";
static const UINT_PTR kTimer = 1;


static std::string fsPath(const std::wstring& path)
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
		if (hoverHit == Hit::Volume)
		{
			volume += GET_WHEEL_DELTA_WPARAM(wp) > 0 ? 0.05f : -0.05f;
			volume = volume < 0 ? 0 : volume > 1 ? 1 : volume;
			player.setVolume(volume);
			InvalidateRect(hwnd, nullptr, FALSE);
		}
		return 0;

	case WM_KEYDOWN:
	{
		bool ctrl = GetKeyState(VK_CONTROL) < 0;
		if (wp == VK_SPACE) togglePlay();
		else if (wp == VK_ESCAPE && queueOpen) click(Hit::QueueButton, -1);
		else if (wp == VK_LEFT) seekTo(player.position() - 5);
		else if (wp == VK_RIGHT) seekTo(player.position() + 5);
		else if (ctrl && wp == 'O') openFileDialog();
		else if (ctrl && wp == VK_OEM_COMMA) { page = Page::Settings; InvalidateRect(hwnd, nullptr, FALSE); }
		else if (ctrl && wp == 'B') click(Hit::NavToggle, -1);
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
	case Hit::NavHome: page = Page::Home; break;
	case Hit::Settings: case Hit::SignalPath: page = Page::Settings; break;
	case Hit::WinMin: ShowWindow(hwnd, SW_MINIMIZE); break;
	case Hit::WinClose: PostMessageW(hwnd, WM_CLOSE, 0, 0); break;
	case Hit::TabNext: npTab = 0; break;
	case Hit::TabAbout: npTab = 1; break;
	case Hit::TabLyrics: npTab = 2; break;
	case Hit::Cover: if (current >= 0) page = Page::Now; break;
	case Hit::Shuffle: shuffle = !shuffle; break;
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
	default: break;
	}
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
		Track t;
		t.path = f;
		t.info = readTrackInfo(fsPath(f));
		t.title = t.info.title.empty() ? fileStem(f) : widen(t.info.title);
		t.artist = t.info.artist.empty() ? L"Unknown artist" : widen(t.info.artist);
		t.album = widen(t.info.album);
		t.coverBytes = std::move(t.info.coverData);
		queue.push_back(std::move(t));
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
	error.clear();
	if (!audioOk) { error = L"No audio output device"; return; }
	if (!player.load(fsPath(queue[i].path)))
	{
		error = L"Couldn't open this file";
	}
	else
	{
		player.play();
	}
	std::wstring title = queue[i].title + L" - LossCat";
	SetWindowTextW(hwnd, title.c_str());   // taskbar and Alt+Tab
	updateTimer();
	InvalidateRect(hwnd, nullptr, FALSE);
}

void App::next(bool userAction)
{
	if (queue.empty()) return;
	if (repeat == 2 && !userAction) { playIndex(current); return; }
	int n = (int)queue.size();
	if (shuffle && n > 1)
	{
		int i = current;
		while (i == current) i = std::rand() % n;
		playIndex(i);
		return;
	}
	if (current + 1 < n) playIndex(current + 1);
	else if (repeat == 1) playIndex(0);
	else if (!userAction) { player.pause(); updateTimer(); InvalidateRect(hwnd, nullptr, FALSE); }
}

void App::prev()
{
	if (current < 0) return;
	if (player.position() > 3 || current == 0)
	{
		if (current == 0 && repeat == 1 && player.position() <= 3) playIndex((int)queue.size() - 1);
		else seekTo(0);
		return;
	}
	playIndex(current - 1);
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
	std::vector<wchar_t> buf(64 * 1024, L'\0');
	OPENFILENAMEW ofn = { sizeof(ofn) };
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = L"Audio files\0*.flac;*.wav;*.mp3;*.aif;*.aiff\0All files\0*.*\0";
	ofn.lpstrFile = buf.data();
	ofn.nMaxFile = (DWORD)buf.size();
	ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_FILEMUSTEXIST;
	if (!GetOpenFileNameW(&ofn)) return;

	// Multi-select returns "folder\0file1\0file2\0\0"; single select returns the full path.
	std::vector<std::wstring> files;
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
	addFiles(files, true);
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
