// Layout and drawing. Numbers come straight from the design canvas (CSS px = DIPs).
#include "app.h"
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cwchar>
#include <cwctype>

namespace col
{
	const uint32_t base = 0x0E0E0E, side = 0x161616, panel = 0x191919;
	const uint32_t text = 0xEAEAEA, white = 0xFFFFFF, soft = 0xD1D1D1, dim = 0xA3A3A3, muted = 0x858585;
	const uint32_t hover = 0x1F1F1F, selected = 0x212121, ctlHover = 0x2B2B2B;
	const uint32_t line = 0x262626, line2 = 0x282828, chip = 0x373737, tile = 0x3A3A3A, track = 0x333333;
}

namespace icons
{
	const char* cat = "M5.8 2.4L7.4 4.3H8.8L10.2 2.5L10.6 5.4C11 7 10.6 8 9.9 8.6C13.2 9.9 15.8 13 15.8 16.6C15.8 18.6 15 19.7 14.2 20H6.6C5.9 20 5.8 19.2 6.3 18.9C6.9 16 6.7 12.4 6.2 10.2C5.4 9.5 5.1 8.2 5.3 7C5.4 5.6 5.3 4 5.8 2.4Z";
	const char* catTail = "M14 19.3C16.8 19.8 19.6 19.9 20.6 18.6C21.4 17.5 20.7 16.2 19.5 16.6";
	const char* toggle = "M5 4H19A2 2 0 0 1 21 6V18A2 2 0 0 1 19 20H5A2 2 0 0 1 3 18V6A2 2 0 0 1 5 4ZM9 4v16";
	const char* now = "M21 12A9 9 0 1 1 3 12A9 9 0 1 1 21 12ZM15 12A3 3 0 1 1 9 12A3 3 0 1 1 15 12Z";
	const char* speaker = "M7 3h10a2 2 0 0 1 2 2v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2zM15.5 14a3.5 3.5 0 1 1-7 0 3.5 3.5 0 1 1 7 0ZM12 7.5h.01";
	const char* home = "M3 11l9-7 9 7v9a1 1 0 0 1-1 1h-5v-6H9v6H4a1 1 0 0 1-1-1z";
	const char* library = "M4 4v16M9 4v16M14 4l5 16";
	const char* check = "M5 12.5l4.5 4.5L19 7.5";
	const char* plus = "M12 5v14M5 12h14";
	const char* heart = "M12 20l-1.3-1.2C6 14.6 3 11.9 3 8.6 3 5.9 5.1 3.9 7.7 3.9c1.6 0 3.1.7 4.3 1.9 1.2-1.2 2.7-1.9 4.3-1.9 2.6 0 4.7 2 4.7 4.7 0 3.3-3 6-7.7 10.2z";
	const char* note = "M9 18V6l10-2v12M9 18a2.5 2.5 0 1 1-5 0 2.5 2.5 0 1 1 5 0ZM19 16a2.5 2.5 0 1 1-5 0 2.5 2.5 0 1 1 5 0Z";
	const char* search = "M18 11A7 7 0 1 1 4 11A7 7 0 1 1 18 11ZM20 20l-3.5-3.5";
	const char* gear = "M15 12A3 3 0 1 1 9 12A3 3 0 1 1 15 12ZM19.4 15a1.7 1.7 0 0 0 .3 1.8l.1.1a2 2 0 1 1-2.8 2.8l-.1-.1a1.7 1.7 0 0 0-1.8-.3 1.7 1.7 0 0 0-1 1.5V21a2 2 0 1 1-4 0v-.1a1.7 1.7 0 0 0-1.1-1.5 1.7 1.7 0 0 0-1.8.3l-.1.1a2 2 0 1 1-2.8-2.8l.1-.1a1.7 1.7 0 0 0 .3-1.8 1.7 1.7 0 0 0-1.5-1H3a2 2 0 1 1 0-4h.1a1.7 1.7 0 0 0 1.5-1.1 1.7 1.7 0 0 0-.3-1.8l-.1-.1a2 2 0 1 1 2.8-2.8l.1.1a1.7 1.7 0 0 0 1.8.3H9a1.7 1.7 0 0 0 1-1.5V3a2 2 0 1 1 4 0v.1a1.7 1.7 0 0 0 1 1.5 1.7 1.7 0 0 0 1.8-.3l.1-.1a2 2 0 1 1 2.8 2.8l-.1.1a1.7 1.7 0 0 0-.3 1.8V9a1.7 1.7 0 0 0 1.5 1H21a2 2 0 1 1 0 4h-.1a1.7 1.7 0 0 0-1.5 1z";
	const char* shuffle = "M16 3h5v5M4 20L21 3M21 16v5h-5M15 15l6 6M4 4l5 5";
	const char* prev = "M19 19L9 12l10-7zM6 5h1.2a1 1 0 0 1 1 1v12a1 1 0 0 1-1 1H6a1 1 0 0 1-1-1V6a1 1 0 0 1 1-1z";
	const char* next = "M5 5l10 7-10 7zM17.8 5H18a1 1 0 0 1 1 1v12a1 1 0 0 1-1 1h-.2a1 1 0 0 1-1-1V6a1 1 0 0 1 1-1z";
	const char* play = "M8 5l12 7-12 7z";
	const char* pause = "M7 5h2a1 1 0 0 1 1 1v12a1 1 0 0 1-1 1H7a1 1 0 0 1-1-1V6a1 1 0 0 1 1-1zM15 5h2a1 1 0 0 1 1 1v12a1 1 0 0 1-1 1h-2a1 1 0 0 1-1-1V6a1 1 0 0 1 1-1z";
	const char* repeat = "M17 2l4 4-4 4M3 11V9a3 3 0 0 1 3-3h15M7 22l-4-4 4-4M21 13v2a3 3 0 0 1-3 3H3";
	const char* queue = "M4 6h11M4 12h11M4 18h7M18 15l3-3-3-3";
	const char* volume = "M11 5L6 9H3v6h3l5 4zM16 9a4 4 0 0 1 0 6M19 6a8 8 0 0 1 0 12";
	const char* mute = "M11 5L6 9H3v6h3l5 4zM16 9l6 6M22 9l-6 6";
	const char* close = "M6 6l12 12M18 6L6 18";
	const char* disc = "M21 12A9 9 0 1 1 3 12A9 9 0 1 1 21 12ZM13.5 12A1.5 1.5 0 1 1 10.5 12A1.5 1.5 0 1 1 13.5 12Z";
	const char* sparkle = "M10 3l1.9 5.1L17 10l-5.1 1.9L10 17l-1.9-5.1L3 10l5.1-1.9zM18 14l.9 2.1L21 17l-2.1.9L18 20l-.9-2.1L15 17l2.1-.9z";
}

static float clamp01(float v) { return v < 0 ? 0 : v > 1 ? 1 : v; }
static float lerp(float a, float b, float t) { return a + (b - a) * t; }

// CSS cubic-bezier(x1, y1, x2, y2) evaluated at x = t.
static float bezier(float x1, float y1, float x2, float y2, float t)
{
	if (t <= 0) return 0;
	if (t >= 1) return 1;
	float u = t;
	for (int i = 0; i < 8; ++i)
	{
		float a = 1 - u;
		float x = 3 * a * a * u * x1 + 3 * a * u * u * x2 + u * u * u - t;
		float dx = 3 * a * a * x1 + 6 * a * u * (x2 - x1) + 3 * u * u * (1 - x2);
		if (std::fabs(dx) < 1e-5f) break;
		u -= x / dx;
		u = clamp01(u);
	}
	float a = 1 - u;
	return 3 * a * a * u * y1 + 3 * a * u * u * y2 + u * u * u;
}
static float easeGlide(float t) { return bezier(0.65f, 0, 0.35f, 1, t); }
static float easeDrawer(float t) { return bezier(0.32f, 0.72f, 0, 1, t); }
static float easeOut(float t) { return bezier(0.23f, 1, 0.32f, 1, t); }

static std::wstring fmtTime(double s)
{
	if (s < 0) s = 0;
	int t = (int)s;
	wchar_t buf[32];
	if (t >= 3600) swprintf(buf, 32, L"%d:%02d:%02d", t / 3600, (t / 60) % 60, t % 60);
	else swprintf(buf, 32, L"%d:%02d", t / 60, t % 60);
	return buf;
}

static std::wstring fmtRate(int hz)
{
	wchar_t buf[32];
	if (hz % 1000 == 0) swprintf(buf, 32, L"%d", hz / 1000);
	else swprintf(buf, 32, L"%.1f", hz / 1000.0);
	return buf;
}

static std::wstring codecOf(const std::wstring& path)
{
	size_t dot = path.find_last_of(L'.');
	std::wstring ext = dot == std::wstring::npos ? L"" : path.substr(dot + 1);
	for (auto& c : ext) c = (wchar_t)towupper(c);
	if (ext == L"AIF") ext = L"AIFF";
	return ext;
}

// Format plus a short quality label, e.g. "FLAC 24/96, Hi-Res".
static std::wstring qualityOf(const Track& t)
{
	std::wstring codec = codecOf(t.path);
	const TrackInfo& i = t.info;
	std::wstring spec = codec;
	if (i.sampleRate)
	{
		wchar_t r[32];
		if (i.sampleRate % 1000) swprintf(r, 32, L"%.1f", i.sampleRate / 1000.0);
		else swprintf(r, 32, L"%d", i.sampleRate / 1000);
		spec += L" " + (i.bitsPerSample ? std::to_wstring(i.bitsPerSample) + L"/" : L"") + r;
	}
	bool lossless = codec == L"FLAC" || codec == L"WAV" || codec == L"AIFF" || codec == L"ALAC" || codec == L"APE" || codec == L"WV";
	if (lossless)
	{
		if (i.bitsPerSample > 16 || i.sampleRate > 48000) return spec + L", Hi-Res";
		if (i.bitsPerSample == 16 && i.sampleRate == 44100) return spec + L", CD quality";
		return spec + L", Lossless";
	}
	if (i.bitrateKbps) return spec + L", Lossy " + std::to_wstring(i.bitrateKbps) + L"k";
	return spec;
}

// ---------------------------------------------------------------- animation state

std::wstring fmtSize(unsigned long long bytes)
{
	// Binary units like Explorer, so the numbers match what Windows shows.
	wchar_t b[32];
	double mb = bytes / (1024.0 * 1024.0);
	if (mb < 1) swprintf(b, 32, L"%llu KB", bytes / 1024);
	else if (mb < 1024) swprintf(b, 32, mb < 10 ? L"%.1f MB" : L"%.0f MB", mb);
	else swprintf(b, 32, mb < 10240 ? L"%.2f GB" : L"%.1f GB", mb / 1024);
	return b;
}

double nowMs()
{
	static LARGE_INTEGER freq = [] { LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f; }();
	LARGE_INTEGER t;
	QueryPerformanceCounter(&t);
	return (double)t.QuadPart * 1000.0 / (double)freq.QuadPart;
}

bool App::animating() const
{
	double now = nowMs();
	return scrollMoving || animMoving || (navStart > 0 && now - navStart < 700) || (queueStart > 0 && now - queueStart < 320);
}

void App::animValues(float& geo, float& labels, float& q) const
{
	double now = nowMs();
	float e = navStart > 0 ? (float)(now - navStart) : 10000.0f;
	if (collapsed)
	{
		// labels fade out fast, then everything glides into the rail
		labels = 1 - clamp01(e / 120.0f);
		geo = lerp(navFrom, 1, easeGlide(clamp01((e - 120) / 520.0f)));
	}
	else
	{
		// the panel opens first, labels fade in once there is room for them
		geo = lerp(navFrom, 0, easeDrawer(clamp01(e / 280.0f)));
		labels = clamp01((e - 140) / 160.0f);
	}
	float qe = queueStart > 0 ? (float)(now - queueStart) : 10000.0f;
	q = queueOpen ? lerp(queueFrom, 1, easeDrawer(clamp01(qe / 280.0f)))
		: lerp(queueFrom, 0, easeOut(clamp01(qe / 180.0f)));
}

float App::anim(Hit id, int index, bool on)
{
	AnimState& a = anims[((long long)id << 32) | (unsigned)(index + 2)];
	double now = nowMs();
	float dt = a.t > 0 ? (float)(now - a.t) : 16.0f;
	if (dt > 50) dt = 50;
	a.t = now;
	float target = on ? 1.0f : 0.0f;
	float step = dt / (on ? 90.0f : 160.0f);   // in quickly, out a little slower
	if (a.value < target) a.value = std::min(target, a.value + step);
	else if (a.value > target) a.value = std::max(target, a.value - step);
	if (a.value != target) animMoving = true;
	return a.value * a.value * (3 - 2 * a.value);   // smoothstep
}

// ---------------------------------------------------------------- hit registry

void App::hot(const Rect& r, Hit id, int index)
{
	hots.push_back({ r, id, index });
}

Hit App::hitAt(float x, float y, int* index) const
{
	// Last registered wins: later drawing is on top.
	for (auto it = hots.rbegin(); it != hots.rend(); ++it)
	{
		if (it->r.contains(x, y))
		{
			if (index) *index = it->index;
			return it->id;
		}
	}
	if (index) *index = -1;
	return Hit::None;
}

// ---------------------------------------------------------------- frame

void App::layout()
{
	RECT rc;
	GetClientRect(hwnd, &rc);
	scale = GetDpiForWindow(hwnd) / 96.0f;
	W = (rc.right - rc.left) / scale;
	H = (rc.bottom - rc.top) / scale;

	animValues(navGeo, labelAlpha, queueVis);
	float navW = lerp(232, 84, navGeo);
	float qW = 300 * queueVis, qGap = 8 * queueVis;
	float top = 8, bottom = H - 96;
	sidebar = { 8, top, navW, bottom - top };
	mainPanel = { 8 + navW + 8, top, W - (8 + navW + 8) - 8 - qW - qGap, bottom - top };
	queuePanel = { mainPanel.right() + qGap, top, qW, bottom - top };
	playerBar = { 0, bottom, W, 96 };
}

void App::paint()
{
	layout();
	stepScroll();
	animMoving = false;
	if (anims.size() > 4000) anims.clear();
	if (!g.begin()) return;
	++frameNo;
	hots.clear();
	qRows.clear();
	tipText.clear();
	marqueeActive = false;
	g.clear(col::base);
	drawSidebar();
	drawMain();
	drawQueuePanel();
	drawWindowControls({ 8, 8, W - 16, H - 104 });   // always in the window's top-right corner
	drawPlayerBar();
	drawToast();
	drawDragGhost();
	if (!menuOpen && !confirmOpen) drawTip();
	if (menuOpen) drawMenu();
	if (confirmOpen) drawConfirm();
	g.end();
	trimCovers();   // blocks until the next vsync, so this paces animation frames to the display

	// Content moved under a still mouse (scrolling, a page change): hover follows what's under it now.
	if (tracking && !dragging && !selectingLyrics)
	{
		int idx = -1;
		Hit h = hitAt(mouseX, mouseY, &idx);
		if (h != hoverHit || idx != hoverIndex) { hoverHit = h; hoverIndex = idx; InvalidateRect(hwnd, nullptr, FALSE); }
	}

	// Sliding text only needs ~30 frames a second, so it gets a timer rather than every vsync.
	if (marqueeActive != marqueeTimer)
	{
		if (marqueeActive) SetTimer(hwnd, 2, 33, nullptr);
		else KillTimer(hwnd, 2);
		marqueeTimer = marqueeActive;
	}

	// While something animates, ask for the next frame straight away instead of using a
	// timer: WM_TIMER only fires every ~15.6 ms and drifts against the display, which
	// makes motion stutter.
	if (animating()) InvalidateRect(hwnd, nullptr, FALSE);
}

// ---------------------------------------------------------------- sidebar

void App::drawSidebar()
{
	const Rect& s = sidebar;
	g.fill(s, col::side, 8);
	g.pushClip(s);

	float t = navGeo;
	float x = s.x + 12, w = s.w - 24;
	float y = s.y + lerp(22, 18, t);   // collapsed, the cat lines up with the page icons beside it

	// Brand row: the cat, or the collapse toggle while hovered.
	Rect brand{ s.x, y, s.w, 32 };
	float logoX = x + lerp(9, 21, t);
	bool brandHover = hovered(Hit::NavToggle);
	if (brandHover)
	{
		Rect btn{ logoX + 9 - 14, y + 2, 28, 28 };
		g.fill(btn, col::ctlHover, 3);
		g.icon(icons::toggle, { btn.cx() - 8, btn.cy() - 8, 16, 16 }, col::text, false);
	}
	else
	{
		Rect logo{ logoX, y + 7, 18, 18 };
		g.icon(icons::cat, logo, col::text, true);
		g.icon(icons::catTail, logo, col::text, false, 1.8f);
	}
	g.text(L"LossCat", { logoX + 18 + 12, y, 120, 32 }, 14, Font::UiSemibold, col::text, Align::Left, labelAlpha);
	hot(brand, Hit::NavToggle);
	y += 32 + lerp(18, 22, t);

	// Now playing / Home: these shrink away when collapsed (they move to the main panel).
	struct Item { Hit id; Page p; const char* icon; const wchar_t* label; };
	const Item items[] = {
		{ Hit::NavNow, Page::Now, icons::now, L"Now playing" },
		{ Hit::NavHome, Page::Home, icons::home, L"Home" },
		{ Hit::NavLibrary, Page::Library, icons::library, L"Library" },
	};
	float rowH = 32 * (1 - t);
	float rowAlpha = 1 - clamp01(t * 2);
	for (const Item& it : items)
	{
		if (rowH > 0.5f)
		{
			Rect r{ x, y, w, rowH };
			bool on = page == it.p;
			if (on) g.fill(r, col::selected, 3, rowAlpha);
			else if (hovered(it.id)) g.fill(r, col::hover, 3, rowAlpha);
			uint32_t fg = on ? col::text : hovered(it.id) ? col::text : col::dim;
			g.pushClip(r);
			g.icon(it.icon, { x + 10, y + rowH / 2 - 8, 16, 16 }, fg, false, 1.5f, rowAlpha);
			g.text(it.label, { x + 36, y, w - 46, rowH }, 13, Font::Ui, fg, Align::Left, rowAlpha * labelAlpha);
			g.popClip();
			if (t < 0.5f) hot(r, it.id);
		}
		y += rowH + 2 * (1 - t);
	}

	drawSidebarPlaylists(y);
	g.popClip();
}

void App::drawEditBox(const Rect& r, float size, Font f)
{
	g.fill(r, 0x101010, 3);
	g.stroke(r, 0x5A5A5A, 3);
	Rect tr{ r.x + 8, r.y, r.w - 16, r.h };
	g.text(editText, tr, size, f, col::text);
	float cw = g.textWidth(editText, size, f);
	if (cw > tr.w - 2) cw = tr.w - 2;
	g.fill({ tr.x + cw + 1, r.cy() - size * 0.6f, 1.5f, size * 1.2f }, col::text);
	hot(r, Hit::EditBox);
}

void App::drawSidebarPlaylists(float y)
{
	const Rect& s = sidebar;
	float t = navGeo;
	float x = s.x + 12, w = s.w - 24;

	float hdrH = 54 * (1 - t);
	if (hdrH > 0.5f)
	{
		float a = (1 - clamp01(t * 2)) * labelAlpha;
		g.text(L"Playlists", { x + 12, y + 22 * (1 - t), w - 60, 24 * (1 - t) }, 12, Font::Ui, col::muted, Align::Left, a);
		Rect plus{ x + w - 6 - 24, y + 22 * (1 - t), 24, 24 * (1 - t) };
		if (t < 0.5f)
		{
			if (hovered(Hit::NewPlaylist)) g.fill(plus, col::ctlHover, 3);
			g.icon(icons::plus, { plus.cx() - 8, plus.cy() - 8, 16, 16 }, col::soft, false, 1.8f, a);
			hot(plus, Hit::NewPlaylist);
		}
	}
	y += hdrH + 4 * t;

	sideList = { s.x, y, s.w, s.bottom() - 14 - y };
	g.pushClip(sideList);
	const auto& all = pls.all();
	float tile = lerp(24, 48, t), rowH = lerp(36, 48, t), gap = lerp(2, 8, t);
	float total = (all.size() + 1) * (rowH + gap);   // + the smart playlist
	sideMaxScroll = total - sideList.h;
	if (sideMaxScroll < 0) sideMaxScroll = 0;
	if (sideScroll > sideMaxScroll) sideScroll = sideMaxScroll;
	float ry = y - sideScroll;
	// The smart playlist sits right under Favourites.
	auto smartRow = [&](float sy) {
		if (sy + rowH < sideList.y || sy > sideList.bottom()) return;
		Rect row{ x, sy, w, rowH };
		bool on = page == Page::Smart;
		bool hov = hovered(Hit::SmartNav);
		if (t < 0.5f)
		{
			if (on) g.fill(row, col::selected, 3, 1 - t * 2);
			else if (hov) g.fill(row, col::hover, 3, 1 - t * 2);
		}
		Rect tl{ x + 6, sy + (rowH - tile) / 2, tile, tile };
		g.fill(tl, hov && t >= 0.5f ? 0x353535 : 0x2E2E2E, lerp(2, 4, t));
		float hs = lerp(14, 20, t);
		g.icon(icons::sparkle, { tl.cx() - hs / 2, tl.cy() - hs / 2, hs, hs }, col::text, false, 1.4f);
		if (on && t >= 0.5f) g.fill({ s.x + 5, tl.cy() - 9, 3, 18 }, col::text, 1.5f, clamp01((t - 0.5f) * 2));   // selected: a small bar at the edge
		g.text(L"Lossless 24-bit", { tl.right() + 6, sy, w - tile - 16 - 30, rowH }, 13, Font::Ui, on || hov ? col::text : col::dim, Align::Left, labelAlpha);
		g.text(std::to_wstring(smartTracks().size()), { x + w - 40, sy, 32, rowH }, 11, Font::Mono, col::muted, Align::Right, labelAlpha);
		if (hov && t > 0.95f) { tipText = L"Lossless 24-bit"; tipX = tl.right() + 14; tipY = tl.cy(); }
		Rect hit = t < 0.5f ? row : Rect{ x, sy, tile + 12, rowH };
		float top = hit.y < sideList.y ? sideList.y : hit.y;
		float bot = hit.bottom() > sideList.bottom() ? sideList.bottom() : hit.bottom();
		if (bot > top) hot({ hit.x, top, hit.w, bot - top }, Hit::SmartNav);
	};
	if (all.size() <= 1) smartRow(ry + (all.empty() ? 0 : rowH + gap));
	for (int i = 0; i < (int)all.size(); ++i, ry += rowH + gap)
	{
		if (i == 1) { smartRow(ry); ry += rowH + gap; }
		if (ry + rowH < sideList.y || ry > sideList.bottom()) continue;
		const Playlist& pl = all[i];
		Rect row{ x, ry, w, rowH };
		bool on = page == Page::Playlist && openPlaylist == i;
		bool hov = hovered(Hit::PlaylistNav, i);
		if (t < 0.5f)
		{
			if (on) g.fill(row, col::selected, 3, 1 - t * 2);
			else if (hov) g.fill(row, col::hover, 3, 1 - t * 2);
		}
		Rect tl{ x + 6, ry + (rowH - tile) / 2, tile, tile };
		float rad = lerp(2, 4, t);
		if (i == 0)
		{
			g.fill(tl, hov && t >= 0.5f ? 0x353535 : 0x2E2E2E, rad);
			float hs = lerp(14, 20, t);
			g.icon(icons::heart, { tl.cx() - hs / 2, tl.cy() - hs / 2, hs, hs }, col::text, true);
		}
		else
		{
			// 2x2 mosaic of the first different covers, grey cells where there are none
			std::vector<ID2D1Bitmap*> art;
			std::vector<std::wstring> folders;
			for (size_t k = 0; k < pl.paths.size() && art.size() < 4; ++k)
			{
				std::wstring abs = absOf(pl.paths[k]);
				std::wstring dir = abs.substr(0, abs.find_last_of(L"\\/"));
				if (std::find(folders.begin(), folders.end(), dir) != folders.end()) continue;
				folders.push_back(dir);
				if (ID2D1Bitmap* b = coverOf(abs)) art.push_back(b);
			}
			if (art.size() == 1) g.image(art[0], tl, rad);
			else
			{
				g.fill(tl, 0x2E2E2E, rad);
				if (art.empty())
				{
					float ns = tile * 0.55f;
					g.icon(icons::note, { tl.cx() - ns / 2, tl.cy() - ns / 2, ns, ns }, col::muted, false, 1.6f);
				}
				for (int k = 0; k < (int)art.size(); ++k)
				{
					float c = tile / 2;
					g.image(art[k], { tl.x + (k % 2) * c, tl.y + (k / 2) * c, c, c }, 0);
				}
			}
		}
		if (on && t >= 0.5f) g.fill({ s.x + 5, tl.cy() - 9, 3, 18 }, col::text, 1.5f, clamp01((t - 0.5f) * 2));   // selected: a small bar at the edge
		if (hov && t > 0.95f && editPlaylist != i)
		{
			tipText = widen(pl.name);
			tipX = tl.right() + 14;
			tipY = tl.cy();
		}

		Rect label{ tl.right() + 6, ry, w - tile - 16 - 30, rowH };
		if (editPlaylist == i && t < 0.5f && !(page == Page::Playlist && openPlaylist == i))
		{
			drawEditBox({ label.x - 4, ry + 4, w - tile - 12, rowH - 8 }, 13, Font::Ui);
		}
		else
		{
			g.text(widen(pl.name), label, 13, Font::Ui, on || hov ? col::text : col::dim, Align::Left, labelAlpha);
			wchar_t n[16];
			swprintf(n, 16, L"%zu", pl.paths.size());
			g.text(n, { x + w - 40, ry, 32, rowH }, 11, Font::Mono, col::muted, Align::Right, labelAlpha);
		}
		Rect hit = t < 0.5f ? row : Rect{ x, ry, tile + 12, rowH };
		float top = hit.y < sideList.y ? sideList.y : hit.y;
		float bot = hit.bottom() > sideList.bottom() ? sideList.bottom() : hit.bottom();
		if (bot > top && editPlaylist != i) hot({ hit.x, top, hit.w, bot - top }, Hit::PlaylistNav, i);
	}
	g.popClip();
}

void App::drawTip()
{
	if (tipText.empty()) return;
	float tw = g.textWidth(tipText, 12, Font::UiMedium);
	if (tw > 260) tw = 260;
	Rect r{ tipX, tipY - 14, tw + 20, 28 };
	g.fill(r, 0x2B2B2B, 4);
	g.stroke(r, col::chip, 4);
	g.text(tipText, { r.x + 10, r.y, tw, 28 }, 12, Font::UiMedium, col::text);
}

// ---------------------------------------------------------------- main panel

void App::drawWindowControls(const Rect& p)
{
	float cy = p.y + 18 + 16;
	float right = p.right() - 22;

	// three dots: minimise, maximise, close (close in the corner)
	float closeX = right - 12, maxX = closeX - 20, minX = maxX - 20;
	Rect group{ minX - 4, cy - 12, right - minX + 8, 24 };
	bool groupHover = group.contains(mouseX, mouseY) || hoverHit == Hit::WinMax;
	struct Dot { float x; Hit id; };
	const Dot dots[] = { { minX, Hit::WinMin }, { maxX, Hit::WinMax }, { closeX, Hit::WinClose } };
	for (const Dot& d : dots)
	{
		bool h = hovered(d.id);
		uint32_t c = h ? (d.id == Hit::WinClose ? 0xE5584F : 0x8A8A8A) : groupHover ? 0x5A5A5A : col::tile;
		float ccx = d.x + 6;
		g.circle(ccx, cy, 6, c);
		if (groupHover)
		{
			uint32_t gl = 0x161616;
			if (d.id == Hit::WinMin) g.line(ccx - 2.5f, cy, ccx + 2.5f, cy, gl, 1.2f);
			if (d.id == Hit::WinMax) g.stroke({ ccx - 2.6f, cy - 2.6f, 5.2f, 5.2f }, gl, 0, 1.1f);
			if (d.id == Hit::WinClose)
			{
				g.line(ccx - 2.2f, cy - 2.2f, ccx + 2.2f, cy + 2.2f, gl, 1.2f);
				g.line(ccx + 2.2f, cy - 2.2f, ccx - 2.2f, cy + 2.2f, gl, 1.2f);
			}
		}
		hot({ d.x - 4, cy - 12, 20, 24 }, d.id);
	}

	// separator, then the gear
	float sepX = minX - 16;
	g.line(sepX, cy - 8, sepX, cy + 8, 0x2E2E2E, 1);
	Rect gear{ sepX - 16 - 32, cy - 16, 32, 32 };
	bool on = page == Page::Settings;
	if (on) g.fill(gear, col::selected, 3);
	else if (hovered(Hit::Settings)) g.fill(gear, col::ctlHover, 3);
	g.icon(icons::gear, { gear.cx() - 8, gear.cy() - 8, 16, 16 }, on ? col::white : col::soft, false);
	hot(gear, Hit::Settings);
	gearRect = gear;
}

void App::drawMain()
{
	const Rect& p = mainPanel;
	g.fill(p, col::panel, 8);
	g.pushClip(p);

	// Collapsed sidebar: its three pages appear as icons at the top left.
	if (navGeo > 0.01f)
	{
		float a = clamp01((navGeo - 0.5f) * 2);
		struct Item { Hit id; Page pg; const char* icon; };
		const Item items[] = { { Hit::NavNow, Page::Now, icons::now }, { Hit::NavHome, Page::Home, icons::home }, { Hit::NavLibrary, Page::Library, icons::library } };
		float x = p.x + 38 - 6 * (1 - a);
		for (const Item& it : items)
		{
			Rect b{ x, p.y + 16, 36, 36 };
			bool on = page == it.pg;
			if (on) g.fill(b, col::selected, 3, a);
			else if (hovered(it.id)) g.fill(b, col::ctlHover, 3, a);
			g.icon(it.icon, { b.cx() - 8, b.cy() - 8, 16, 16 }, on ? col::text : col::soft, false, 1.5f, a);
			if (a > 0.5f) hot(b, it.id);
			x += 38;
		}
	}

	Rect area{ p.x + 38, p.y + 56 + 18, p.w - 76, p.h - 56 - 18 - 24 };
	g.pushClip({ p.x, p.y + 56, p.w, p.h - 56 });
	switch (page)
	{
	case Page::Home: drawHome(area); break;
	case Page::Library: drawLibrary(area); break;
	case Page::Playlist: drawPlaylist(area); break;
	case Page::Album: drawAlbum(area); break;
	case Page::Smart: drawSmart(area); break;
	case Page::Artist: drawArtist(area); break;
	case Page::License: drawLicense(area); break;
	case Page::Now: drawNowPlaying(area); break;
	case Page::Settings: drawSettings(area); break;
	case Page::Search: drawSearch(area); break;
	}
	g.popClip();
	drawSearchField();

	g.popClip();
}

static void sectionTitle(Gfx& g, const wchar_t* s, float x, float y, float w);

void App::drawSearchField()
{
	// Centred in the main panel's top strip, up to 460 wide, clear of the collapsed-sidebar
	// icons on the left and the gear on the right.
	const Rect& p = mainPanel;
	float left = p.x + (navGeo > 0.01f ? 38 + 3 * 38 + 16 : 32);
	float right = (gearRect.w > 0 ? gearRect.x : p.right() - 140) - 16;
	float w = std::min(460.0f, right - left);
	if (w < 160) return;
	float x = std::max(left, std::min(p.cx() - w / 2, right - w));
	Rect r{ x, p.y + 16, w, 32 };
	bool hov = hovered(Hit::SearchField) || hovered(Hit::SearchClear);
	float f = anim(Hit::SearchField, -1, searchFocused);
	g.fill(r, 0x1D1D1D, 3);
	if (f > 0.01f) g.fill(r, 0x202020, 3, f);
	g.stroke(r, searchFocused ? 0x5A5A5A : hov ? 0x3A3A3A : 0x2B2B2B, 3);
	g.icon(icons::search, { r.x + 12, r.cy() - 8, 16, 16 }, 0xA3A3A3, false, 2.0f);
	Rect tr{ r.x + 38, r.y, r.w - 38 - 34, r.h };
	if (query.empty())
	{
		g.text(L"Search songs, albums, artists", tr, 13, Font::Ui, 0x858585);
		if (searchFocused) g.fill({ tr.x, r.cy() - 8, 1.5f, 16 }, col::text);
	}
	else
	{
		// a long query shows its end, where you're typing
		float qw = g.textWidth(query, 13, Font::Ui);
		g.pushClip(tr);
		g.text(query, { qw > tr.w ? tr.right() - qw - 2 : tr.x, r.y, qw + 4, r.h }, 13, Font::Ui, col::text);
		g.popClip();
		if (searchFocused) g.fill({ std::min(tr.x + qw + 1, tr.right()), r.cy() - 8, 1.5f, 16 }, col::text);
	}
	hot(r, Hit::SearchField);
	if (!query.empty())
	{
		Rect cb{ r.right() - 30, r.cy() - 12, 24, 24 };
		bool ch = hovered(Hit::SearchClear);
		if (ch) g.fill(cb, col::ctlHover, 3);
		g.icon(icons::close, { cb.cx() - 6, cb.cy() - 6, 12, 12 }, ch ? col::text : 0x858585, false, 1.6f);
		hot(cb, Hit::SearchClear);
	}
}

void App::drawSearch(const Rect& a)
{
	listArea = {};
	pageArea = a;
	runSearch();
	float y = a.y;
	if (searchSongs.empty() && searchAlbums.empty() && searchArtists.empty())
	{
		g.text(L"Nothing in your library matches “" + query + L"”", { a.x + 6, y, a.w - 12, 24 }, 15, Font::UiMedium, col::text);
		g.text(L"Search looks at titles, artists and albums. Check the spelling, or try fewer words.", { a.x + 6, y + 30, a.w - 12, 20 }, 13, Font::Ui, col::dim);
		return;
	}
	const std::wstring* playing = current >= 0 ? &queue[current].path : nullptr;
	// One row each of artists and albums, as many as fit across; the songs fill the rest.
	auto fit = [&](bool round) {
		float minW = round ? 110.0f : 140.0f, gap = round ? 20.0f : 8.0f;
		return std::max(1, (int)((a.w + gap) / (minW + gap)));
	};
	if (!searchArtists.empty() && a.h > 420)
	{
		sectionTitle(g, L"Artists", a.x, y, a.w);
		y += 34;
		std::vector<Tile> tiles;
		for (size_t i = 0; i < searchArtists.size() && (int)i < fit(true); ++i)
		{
			const Group& ar = artists[searchArtists[i]];
			tiles.push_back({ ar.name, L"", libTracks[ar.tracks.front()].path });
		}
		y += drawTiles(tiles, a.x, y, a.w, Hit::SearchArtist, true) + 22;
	}
	if (!searchAlbums.empty() && a.h > 320)
	{
		sectionTitle(g, L"Albums", a.x, y, a.w);
		y += 34;
		std::vector<Tile> tiles;
		for (size_t i = 0; i < searchAlbums.size() && (int)i < fit(false); ++i)
		{
			const Group& al = albums[searchAlbums[i]];
			bool on = false;
			for (int t : al.tracks) if (playing && *playing == libTracks[t].path) on = true;
			tiles.push_back({ al.name, al.artist, libTracks[al.tracks.front()].path, on });
		}
		y += drawTiles(tiles, a.x, y, a.w, Hit::SearchAlbum, false) + 22;
	}
	if (!searchSongs.empty())
	{
		std::wstring head = searchSongs.size() == 1 ? L"Songs" : L"Songs · " + std::to_wstring(searchSongs.size());
		sectionTitle(g, head.c_str(), a.x, y, a.w);
		y += 34;
		drawTrackRows({ a.x, y, a.w, a.bottom() - y }, searchSongs, Hit::SearchRow);
	}
}

void App::hotVisible(const Rect& r, Hit id, int index)
{
	float top = r.y > pageArea.y ? r.y : pageArea.y;
	float bot = r.bottom() < pageArea.bottom() ? r.bottom() : pageArea.bottom();
	if (bot > top) hot({ r.x, top, r.w, bot - top }, id, index);
}

void App::drawEmpty(const Rect& a, const wchar_t* heading, const wchar_t* body, bool button)
{
	float w = a.w < 420 ? a.w : 420;
	float x = a.cx() - w / 2, y = a.y + 78;
	g.text(heading, { x, y, w, 24 }, 18, Font::UiMedium, col::text);
	y += 36;
	y += g.textBlock(body, { x, y, w, 80 }, 13, Font::Ui, col::dim, 4, 21) + 14;
	if (button)
	{
		Rect b{ x, y, g.textWidth(L"Open files", 13, Font::UiMedium) + 28, 32 };
		g.fill(b, hovered(Hit::OpenFiles) ? 0xFFFFFF : col::text, 3);
		g.text(L"Open files", b, 13, Font::UiMedium, 0x131313, Align::Center);
		hot(b, Hit::OpenFiles);
	}
}

void App::drawTrackRows(const Rect& a, const std::vector<Track>& list, Hit rowHit, bool sortable)
{
	float flexW = (a.right() - 12 - 56 - 36 - 16 - 110) - (a.x + 52);
	bool showAlbum = flexW >= 420;
	float wTitle = flexW * (showAlbum ? 0.44f : 0.6f), wArtist = flexW * (showAlbum ? 0.28f : 0.4f), wAlbum = showAlbum ? flexW * 0.28f : 0;
	float colX[] = { a.x + 12, a.x + 52, a.x + 52 + wTitle, a.x + 52 + wTitle + wArtist, a.right() - 12 - 56 - 36 - 16 - 110, a.right() - 12 - 56 };

	float y = a.y;
	g.text(L"#", { colX[0], y, 30, 30 }, 12, Font::Ui, col::muted);
	// Headers; in a sortable list each is a button, the sorted one bright with an arrow.
	auto header = [&](const wchar_t* label, float hx, float hw, int key, bool right) {
		bool on = sortable && songSort == key;
		bool hv = sortable && hovered(Hit::SortHeader, key);
		uint32_t c = on || hv ? col::text : col::muted;
		float lw = g.textWidth(label, 12, on ? Font::UiMedium : Font::Ui);
		float lx = right ? hx + hw - lw - (on ? 16 : 0) : hx;
		g.text(label, { lx, y, lw + 2, 30 }, 12, on ? Font::UiMedium : Font::Ui, c);
		if (on) g.icon(songSortDesc ? "M6 9l6 6 6-6" : "M6 15l6-6 6 6", { lx + lw + 3, y + 8, 13, 13 }, col::text, false, 1.8f);
		if (sortable) hot({ right ? hx + hw - lw - 22 : hx - 4, y, lw + 26, 30 }, Hit::SortHeader, key);
	};
	header(L"Title", colX[1], wTitle - 16, SortTitle, false);
	header(L"Artist", colX[2], wArtist - 16, SortArtist, false);
	if (showAlbum) header(L"Album", colX[3], wAlbum - 16, SortAlbum, false);
	header(L"Format", colX[4], 110, SortFormat, false);
	header(L"Time", colX[5], 56, SortTime, true);
	y += 30;
	g.line(a.x, y - 0.5f, a.right(), y - 0.5f, col::line, 1);

	listArea = { a.x, y, a.w, a.bottom() - y };
	listMaxScroll = (float)list.size() * 30 - listArea.h;
	if (listMaxScroll < 0) listMaxScroll = 0;
	if (listScroll > listMaxScroll) listScroll = listMaxScroll;
	const std::wstring* playing = current >= 0 ? &queue[current].path : nullptr;
	rowHeartPaths.assign(list.size(), L"");

	g.pushClip(listArea);
	int first = (int)(listScroll / 30);
	for (int i = first; i < (int)list.size(); ++i)
	{
		float ry = listArea.y + i * 30 - listScroll;
		if (ry >= listArea.bottom()) break;
		const Track& t = list[i];
		Rect r{ a.x, ry, a.w, 30 };
		bool on = playing && *playing == t.path;
		bool rowHov = hovered(rowHit, i) || hovered(Hit::RowHeart, i);
		if (rowHov) g.fill(r, col::hover);
		uint32_t fg = on ? col::white : col::text;
		wchar_t num[16];
		swprintf(num, 16, L"%d", i + 1);
		g.text(num, { colX[0], ry, 30, 30 }, 12, Font::Mono, on ? col::white : col::muted);
		g.text(t.title, { colX[1], ry, wTitle - 16, 30 }, 13, on ? Font::UiMedium : Font::Ui, fg);
		g.text(t.artist, { colX[2], ry, wArtist - 16, 30 }, 13, Font::Ui, col::soft);
		if (showAlbum) g.text(t.album, { colX[3], ry, wAlbum - 16, 30 }, 13, Font::Ui, col::soft);
		std::wstring fmt = codecOf(t.path);
		if (t.info.sampleRate)
			fmt += L" " + (t.info.bitsPerSample ? std::to_wstring(t.info.bitsPerSample) + L"/" : L"") + fmtRate(t.info.sampleRate);
		g.text(fmt, { colX[4], ry, 110, 30 }, 12, Font::Mono, col::dim);
		g.text(fmtTime(t.info.durationMs / 1000.0), { colX[5], ry, 56, 30 }, 12, Font::Mono, col::dim, Align::Right);
		g.line(a.x, ry + 29.5f, a.right(), ry + 29.5f, 0x202020, 1);
		float top = ry < listArea.y ? listArea.y : ry;
		float bot = ry + 30 > listArea.bottom() ? listArea.bottom() : ry + 30;
		if (bot > top) hot({ a.x, top, a.w, bot - top }, rowHit, i);

		// heart: always shown when liked, on hover otherwise
		if (inLibrary(t.path))
		{
			bool fav = inPlaylist(0, t.path);
			Rect hb{ colX[5] - 36, ry + 3, 24, 24 };
			if (fav || rowHov)
			{
				if (hovered(Hit::RowHeart, i)) g.fill(hb, col::ctlHover, 3);
				g.icon(icons::heart, { hb.cx() - 7, hb.cy() - 7, 14, 14 }, fav ? col::white : col::soft, fav, 1.6f);
			}
			rowHeartPaths[i] = t.path;
			if (bot > top) hot({ hb.x, top > hb.y ? top : hb.y, hb.w, (bot < hb.bottom() ? bot : hb.bottom()) - (top > hb.y ? top : hb.y) }, Hit::RowHeart, i);
		}
	}
	g.popClip();
}

ID2D1Bitmap* App::coverOf(const std::wstring& path)
{
	if (coversGen != g.generation()) { covers.clear(); coversGen = g.generation(); }
	auto it = covers.find(path);
	if (it != covers.end())
	{
		it->second.lastUsed = frameNo;
		return it->second.bmp.Get();
	}
	UINT px = (UINT)(320 * scale);
	CachedCover& c = covers[path];
	c.bmp = g.loadImage(readTrackInfo(fsPath(path)).coverData, px);
	c.lastUsed = frameNo;
	return c.bmp.Get();
}

void App::trimCovers()
{
	// Keep the most recently shown covers; the rest are decoded again if they come back on screen.
	const size_t keep = 120;
	if (covers.size() <= keep + 30) return;
	std::vector<std::pair<unsigned long long, std::wstring>> byAge;
	for (const auto& [path, c] : covers) byAge.push_back({ c.lastUsed, path });
	std::sort(byAge.begin(), byAge.end());
	for (size_t i = 0; i + keep < byAge.size(); ++i) covers.erase(byAge[i].second);
}

float App::drawTiles(const std::vector<Tile>& tiles, float x, float y, float w, Hit hit, bool round)
{
	if (tiles.empty()) return 0;
	float minW = round ? 110.0f : 140.0f, gap = round ? 20.0f : 8.0f;
	int cols = (int)((w + gap) / (minW + gap));
	if (cols < 1) cols = 1;
	float tw = (w - (cols - 1) * gap) / cols;
	float pad = round ? 0.0f : 8.0f;
	float th = pad + (tw - 2 * pad) + 10 + 18 + (round ? 0 : 18) + pad;
	for (int i = 0; i < (int)tiles.size(); ++i)
	{
		const Tile& t = tiles[i];
		Rect r{ x + (i % cols) * (tw + gap), y + (i / cols) * (th + (round ? 0 : gap)), tw, th };
		if (r.bottom() < pageArea.y || r.y > pageArea.bottom()) continue;
		bool hov = hovered(hit, i) || ((hit == Hit::AlbumTile || hit == Hit::ArtistAlbum || hit == Hit::AddedTile || hit == Hit::SearchAlbum) && hovered(Hit::TilePlay, i));
		float ha = anim(hit, i, hov);
		if (ha > 0.01f && !round) g.fill(r, col::hover, 4, ha);
		Rect img{ r.x + pad, r.y + pad, tw - 2 * pad, tw - 2 * pad };
		if (round)
		{
			if (ID2D1Bitmap* bmp = t.coverPath.empty() ? nullptr : coverOf(t.coverPath))
			{
				g.image(bmp, img, img.w / 2);
				if (hov) g.circle(img.cx(), img.cy(), img.w / 2, 0xFFFFFF, true, 1, 0.08f);
			}
			else
			{
				g.circle(img.cx(), img.cy(), img.w / 2, hov ? 0x353535 : 0x2E2E2E);
				std::wstring initial = t.title.empty() ? L"?" : t.title.substr(0, 1);
				g.text(initial, img, img.w * 0.32f, Font::UiMedium, col::dim, Align::Center);
			}
		}
		else
		{
			if (t.heart || t.sparkle)
			{
				g.fill(img, hov ? 0x353535 : 0x2E2E2E, 2);
				float hs = img.w * 0.3f;
				g.icon(t.heart ? icons::heart : icons::sparkle, { img.cx() - hs / 2, img.cy() - hs / 2, hs, hs }, col::text, t.heart, 1.4f);
			}
			else if (t.mosaic.size() > 1)
			{
				g.fill(img, 0x2E2E2E, 2);
				float c = img.w / 2;
				for (int k = 0; k < (int)t.mosaic.size() && k < 4; ++k)
					if (ID2D1Bitmap* b = coverOf(t.mosaic[k])) g.image(b, { img.x + (k % 2) * c, img.y + (k / 2) * c, c, c }, 0);
			}
			else if (ID2D1Bitmap* bmp = coverOf(t.coverPath)) g.image(bmp, img, 2);
			else
			{
				g.fill(img, col::tile, 2);
				if (t.coverPath.empty())
				{
					float ns = img.w * 0.3f;
					g.icon(icons::note, { img.cx() - ns / 2, img.cy() - ns / 2, ns, ns }, col::muted, false, 1.4f);
				}
			}
			g.stroke(img, 0x313131, 2, 1, 0.6f);
			if (hov && (hit == Hit::AlbumTile || hit == Hit::ArtistAlbum || hit == Hit::AddedTile || hit == Hit::SearchAlbum))
			{
				Rect pb{ img.right() - 44, img.bottom() - 44, 36, 36 };
				bool ph = hovered(Hit::TilePlay, i);
				g.circle(pb.cx(), pb.cy(), 18, ph ? 0xFFFFFF : col::text);
				g.icon(icons::play, { pb.cx() - 7, pb.cy() - 7, 14, 14 }, 0x131313, true);
			}
			else if (t.playing)
			{
				Rect badge{ img.right() - 28, img.bottom() - 28, 20, 20 };
				g.fill(badge, col::panel, 2);
				g.icon(icons::play, { badge.cx() - 6, badge.cy() - 6, 12, 12 }, col::white, true);
			}
		}
		hotVisible(r, hit, i);
		if (hov && (hit == Hit::AlbumTile || hit == Hit::ArtistAlbum || hit == Hit::AddedTile || hit == Hit::SearchAlbum))
			hotVisible({ img.right() - 44, img.bottom() - 44, 36, 36 }, Hit::TilePlay, i);
		float ty = img.bottom() + 10;
		g.text(t.title, { r.x + pad, ty, tw - 2 * pad, 18 }, 13, Font::UiMedium, t.playing ? col::white : col::text, round ? Align::Center : Align::Left);
		if (!round) g.text(t.sub, { r.x + pad, ty + 18, tw - 2 * pad, 18 }, 13, Font::Ui, col::dim);
	}
	int rows = ((int)tiles.size() + cols - 1) / cols;
	return rows * th + (rows - 1) * (round ? 0 : gap);
}

static void sectionTitle(Gfx& g, const wchar_t* s, float x, float y, float w)
{
	g.text(s, { x + 6, y, w - 12, 22 }, 14, Font::UiMedium, col::text);
}

void App::drawHome(const Rect& a)
{
	listArea = {};
	pageArea = a;
	if (libTracks.empty() && recentPlayed.empty())
	{
		pageMaxScroll = 0;
		drawEmpty(a, L"Your library is empty",
			L"Import songs to copy them into your LossCat folder, or drop files on this window to just play them.", false);
		Rect b{ a.cx() - 210, a.y + 78 + 36 + 21 * 2 + 14, g.textWidth(L"Import files", 13, Font::UiMedium) + 28, 32 };
		g.fill(b, hovered(Hit::ImportFiles) ? 0xFFFFFF : col::text, 3);
		g.text(L"Import files", b, 13, Font::UiMedium, 0x131313, Align::Center);
		hot(b, Hit::ImportFiles);
		return;
	}

	const std::wstring* playing = current >= 0 ? &queue[current].path : nullptr;
	float y = a.y - pageScroll;
	auto songTile = [&](const Track& t) {
		Tile tl{ t.title, t.artist, t.path, playing && *playing == t.path };
		return tl;
	};

	if (!recentPlayed.empty())
	{
		sectionTitle(g, L"Recently played", a.x, y, a.w);
		y += 34;
		std::vector<Tile> tiles;
		// One tile per entry, in order, so a tile's index is its place in recentPlayed.
		// The song may have left the queue since; the library still knows it.
		for (const auto& p : recentPlayed)
		{
			auto same = [&](const Track& t) { return t.path == p; };
			auto it = std::find_if(queue.begin(), queue.end(), same);
			if (it != queue.end()) { tiles.push_back(songTile(*it)); continue; }
			auto lt = std::find_if(libTracks.begin(), libTracks.end(), same);
			if (lt != libTracks.end()) { tiles.push_back(songTile(*lt)); continue; }
			std::wstring name = p.substr(p.find_last_of(L"\\/") + 1);
			tiles.push_back({ name, L"", p, playing && *playing == p });
		}
		y += drawTiles(tiles, a.x, y, a.w, Hit::RecentTile, false) + 30;
	}
	if (!recentlyAdded.empty())
	{
		sectionTitle(g, L"Recently added", a.x, y, a.w);
		y += 34;
		std::vector<Tile> tiles;
		for (int k : recentlyAdded)
		{
			const Group& al = albums[k];
			bool on = false;
			for (int i : al.tracks) if (playing && *playing == libTracks[i].path) on = true;
			tiles.push_back({ al.name, al.artist, libTracks[al.tracks.front()].path, on });
		}
		y += drawTiles(tiles, a.x, y, a.w, Hit::AddedTile, false) + 30;
	}
	if (!artists.empty())
	{
		sectionTitle(g, L"Artists", a.x, y, a.w);
		y += 38;
		std::vector<Tile> tiles;
		for (size_t i = 0; i < artists.size() && i < 14; ++i) tiles.push_back({ artists[i].name, L"", libTracks[artists[i].tracks.front()].path });
		y += drawTiles(tiles, a.x + 6, y, a.w - 12, Hit::ArtistTile, true) + 30;
	}
	pageMaxScroll = (y + pageScroll) - a.bottom();
	if (pageMaxScroll < 0) pageMaxScroll = 0;
}

void App::drawLibrary(const Rect& a)
{
	listArea = {};
	float y = a.y;
	g.text(L"Library", { a.x + 6, y, 120, 30 }, 22, Font::UiMedium, col::text);
	const wchar_t* tabs[] = { L"Albums", L"Artists", L"Songs", L"Playlists" };
	const Hit tabHits[] = { Hit::LibTabAlbums, Hit::LibTabArtists, Hit::LibTabSongs, Hit::LibTabPlaylists };
	float tx = a.x + 6 + g.textWidth(L"Library", 22, Font::UiMedium) + 28;
	for (int i = 0; i < 4; ++i)
	{
		float tw = g.textWidth(tabs[i], 13, Font::UiMedium);
		Rect r{ tx, y + 2, tw, 28 };
		bool on = libTab == i;
		g.text(tabs[i], r, 13, Font::UiMedium, on || hovered(tabHits[i]) ? col::text : col::muted);
		if (on) g.fill({ tx, r.bottom() - 2, tw, 2 }, col::text);
		hot(r, tabHits[i]);
		tx += tw + 18;
	}
	// Import is a quiet text button; rescanning lives in Settings (and F5)
	float bx = a.right() - 6;
	{
		bool hov = hovered(Hit::ImportFiles);
		float bw = 10 + 14 + 6 + g.textWidth(L"Import", 13, Font::Ui) + 12;
		Rect b{ bx - bw, y, bw, 30 };
		if (hov) g.fill(b, col::hover, 15);
		uint32_t c = hov ? col::text : col::soft;
		g.icon(icons::plus, { b.x + 10, b.cy() - 7, 14, 14 }, c, false, 1.8f);
		g.text(L"Import", { b.x + 30, b.y, bw - 34, 30 }, 13, Font::Ui, c);
		hot(b, Hit::ImportFiles);
		bx -= bw + 8;
	}
	if (importing)
	{
		float cw = g.textWidth(L"Cancel", 12, Font::Ui);
		Rect cancel{ bx - cw, y + 5, cw, 20 };
		g.text(L"Cancel", cancel, 12, Font::Ui, hovered(Hit::CancelImport) ? col::white : col::soft);
		g.line(cancel.x, cancel.bottom() - 2.5f, cancel.right(), cancel.bottom() - 2.5f, hovered(Hit::CancelImport) ? col::white : 0x5A5A5A, 1);
		hot(cancel.inset(-4), Hit::CancelImport);
		std::wstring progress = importCancel ? L"Stopping after this file\x2026"
			: L"Importing " + std::to_wstring(importDone + 1 > importTotal ? importTotal : (int)importDone + 1) + L" of " + std::to_wstring(importTotal) + L"\x2026";
		g.text(progress, { tx + 12, y, cancel.x - 12 - tx - 12, 30 }, 12, Font::Ui, col::dim, Align::Right);
		float barW = 120, done = importTotal ? (float)importDone / importTotal : 0;
		g.fill({ cancel.x - 12 - barW, y + 27, barW, 2 }, col::track, 1);
		g.fill({ cancel.x - 12 - barW, y + 27, barW * done, 2 }, col::text, 1);
	}
	else if (scanning) g.text(L"Scanning\x2026 " + std::to_wstring(scanCount) + L" files", { tx + 12, y, bx - tx - 20, 30 }, 12, Font::Ui, col::dim, Align::Right);
	else if (!notice.empty()) g.text(notice, { tx + 12, y, bx - tx - 20, 30 }, 12, Font::Ui, col::dim, Align::Right);
	y += 30 + 22;

	Rect body{ a.x, y, a.w, a.bottom() - y };
	pageArea = body;
	if (libTracks.empty())
	{
		pageMaxScroll = 0;
		g.text(scanning ? L"Scanning your folders\x2026" : L"Nothing here yet. Import files, or add a music folder in Settings to use your music where it is.",
			{ a.x + 6, y, a.w - 12, 20 }, 13, Font::Ui, col::dim);
		return;
	}
	if (libTab == 2)
	{
		pageMaxScroll = 0;
		float px = a.x + 6;
		auto smallButton = [&](const wchar_t* label, const char* icon, bool filled, Hit id, bool primary) {
			float bw = 12 + 16 + 8 + g.textWidth(label, 13, Font::UiMedium) + 14;
			Rect b{ px, y, bw, 30 };
			if (primary) g.fill(b, hovered(id) ? 0xFFFFFF : col::text, 15);
			else { if (hovered(id)) g.fill(b, col::hover, 15); g.stroke(b, col::chip, 15); }
			uint32_t fg = primary ? 0x131313 : col::text;
			g.icon(icon, { b.x + 12, b.cy() - 8, 16, 16 }, fg, filled, 1.6f);
			g.text(label, { b.x + 36, b.y, bw - 40, 30 }, 13, Font::UiMedium, fg);
			hot(b, id);
			px += bw + 8;
		};
		smallButton(L"Play all", icons::play, true, Hit::PlayAll, true);
		smallButton(L"Shuffle", icons::shuffle, false, Hit::ShuffleAll, false);
		wchar_t cnt[32];
		swprintf(cnt, 32, libTracks.size() == 1 ? L"%zu song" : L"%zu songs", libTracks.size());
		g.text(cnt, { px + 8, y, 200, 30 }, 12, Font::Ui, col::muted);
		// Sort on the right of this row: shows the current order, opens the list of orders
		{
			const wchar_t* names[] = { L"Default order", L"Title", L"Artist", L"Album", L"Format", L"Length", L"Date added" };
			std::wstring label = std::wstring(L"Sort: ") + names[songSort];
			float sw = g.textWidth(label, 12, Font::Ui) + 40;
			Rect sb{ body.right() - sw, y, sw, 30 };
			float h = anim(Hit::SortMenu, -1, hovered(Hit::SortMenu));
			if (h > 0.01f) g.fill(sb, col::hover, 15, h);
			g.text(label, { sb.x + 12, sb.y, sw - 34, 30 }, 12, Font::Ui, hovered(Hit::SortMenu) ? col::text : col::dim);
			g.icon("M6 9l6 6 6-6", { sb.right() - 24, sb.cy() - 7, 14, 14 }, col::dim, false, 1.8f);
			hot(sb, Hit::SortMenu);
			sortButton = sb;
		}
		drawTrackRows({ body.x, y + 30 + 16, body.w, body.h - 46 }, sortedSongs(), Hit::LibraryRow, true);
		return;
	}
	g.pushClip(body);
	float cy = y - pageScroll;
	float used = 0;
	const std::wstring* playing = current >= 0 ? &queue[current].path : nullptr;
	if (libTab == 0)
	{
		std::vector<Tile> tiles;
		for (const Group& al : albums)
		{
			bool on = false;
			for (int i : al.tracks) if (playing && *playing == libTracks[i].path) on = true;
			tiles.push_back({ al.name, al.artist, libTracks[al.tracks.front()].path, on });
		}
		used = drawTiles(tiles, a.x, cy, a.w, Hit::AlbumTile, false);
	}
	else if (libTab == 3)
	{
		std::vector<Tile> tiles;
		const auto& all = pls.all();
		for (int p = 0; p < (int)all.size(); ++p)
		{
			Tile tl;
			tl.title = widen(all[p].name);
			tl.sub = all[p].paths.size() == 1 ? L"1 song" : std::to_wstring(all[p].paths.size()) + L" songs";
			tl.heart = p == 0;
			std::vector<std::wstring> folders;   // one cover per album folder, so the mosaic isn't the same art four times
			for (const std::string& rel : all[p].paths)
			{
				std::wstring abs = absOf(rel);
				std::wstring dir = abs.substr(0, abs.find_last_of(L"\\/"));
				if (std::find(folders.begin(), folders.end(), dir) != folders.end()) continue;
				folders.push_back(dir);
				tl.mosaic.push_back(abs);
				if (tl.mosaic.size() == 4) break;
			}
			if (tl.mosaic.size() == 1) tl.coverPath = tl.mosaic[0];
			tiles.push_back(tl);
			if (p == 0)
			{
				Tile sm;
				sm.title = L"Lossless 24-bit";
				size_t n = smartTracks().size();
				sm.sub = n == 1 ? L"1 song" : std::to_wstring(n) + L" songs";
				sm.sparkle = true;
				tiles.push_back(sm);
			}
		}
		used = drawTiles(tiles, a.x, cy, a.w, Hit::PlaylistTile, false);
	}
	else
	{
		std::vector<Tile> tiles;
		for (const Group& ar : artists) tiles.push_back({ ar.name, L"", libTracks[ar.tracks.front()].path });
		used = drawTiles(tiles, a.x + 6, cy, a.w - 12, Hit::ArtistTile, true);
	}
	g.popClip();
	pageMaxScroll = used - body.h + 24;
	if (pageMaxScroll < 0) pageMaxScroll = 0;
}

void App::drawMosaic(const Rect& r, const std::vector<Track>& list, const char* icon)
{
	// one song per album, first four albums in playlist order
	std::vector<std::wstring> picks, seen;
	for (const Track& t : list)
	{
		std::wstring key = t.album.empty() ? t.path : t.album + L"\x1F" + t.artist;
		if (std::find(seen.begin(), seen.end(), key) != seen.end()) continue;
		seen.push_back(key);
		if (coverOf(t.path)) picks.push_back(t.path);
		if (picks.size() == 4 || seen.size() > 40) break;
	}
	if (picks.empty())
	{
		g.fill(r, 0x232323, 3);
		g.stroke(r, 0x313131, 3, 1, 0.6f);
		g.icon(icon, { r.cx() - 14, r.cy() - 14, 28, 28 }, col::soft, false, 1.4f);
		return;
	}
	if (picks.size() < 4) { g.image(coverOf(picks[0]), r, 3); return; }
	float h = r.w / 2;
	g.pushClip(r);
	for (int k = 0; k < 4; ++k)
		g.image(coverOf(picks[k]), { r.x + (k % 2) * h, r.y + (k / 2) * h, h, h }, 0);
	g.popClip();
}

void App::drawPlaylist(const Rect& a)
{
	listArea = {};
	const auto& all = pls.all();
	if (openPlaylist < 0 || openPlaylist >= (int)all.size()) { page = Page::Home; return; }
	const Playlist& pl = all[openPlaylist];
	const std::vector<Track>& list = playlistTracks(openPlaylist);
	float y = a.y;

	// picture on the left, name / songs / buttons beside it (same header as the smart playlist)
	Rect tile{ a.x + 6, y, 96, 96 };
	drawMosaic(tile, list, openPlaylist == 0 ? icons::heart : icons::note);
	float hx = tile.right() + 24;
	g.text(openPlaylist == 0 ? L"Favourites" : L"Playlist", { hx, y + 2, 300, 18 }, 12, Font::Ui, col::soft);
	y += 20;
	if (editPlaylist == openPlaylist)
		drawEditBox({ hx - 6, y - 2, (a.right() - hx) * 0.6f, 34 }, 22, Font::UiMedium);
	else
		g.text(widen(pl.name), { hx, y, a.right() - hx, 30 }, 22, Font::UiMedium, col::text);
	y += 32;
	int secs = 0;
	for (const Track& t : list) secs += t.info.durationMs / 1000;
	wchar_t sub[64];
	if (list.size() == 1) swprintf(sub, 64, L"1 song, %d min", (secs + 59) / 60);
	else swprintf(sub, 64, L"%zu songs, %d min", list.size(), (secs + 59) / 60);
	g.text(sub, { hx, y, a.right() - hx, 20 }, 13, Font::Ui, col::dim);
	y = tile.bottom() + 20;

	float px = a.x + 6;
	auto button = [&](const wchar_t* label, const char* icon, bool filled, Hit id, bool primary) {
		float bw = (icon ? 12 + 16 + 8 : 14) + g.textWidth(label, 13, Font::UiMedium) + 14;
		Rect b{ px, y, bw, 30 };
		if (primary) g.fill(b, hovered(id) ? 0xFFFFFF : col::text, 15);
		else { if (hovered(id)) g.fill(b, col::hover, 15); g.stroke(b, col::chip, 15); }
		uint32_t fg = primary ? 0x131313 : col::text;
		if (icon) g.icon(icon, { b.x + 12, b.cy() - 8, 16, 16 }, fg, filled, 1.6f);
		float lx = icon ? 36.0f : 14.0f;
		g.text(label, { b.x + lx, b.y, bw - lx - 8, 30 }, 13, Font::UiMedium, fg);
		hot(b, id);
		px += bw + 8;
	};
	if (!list.empty())
	{
		button(L"Play", icons::play, true, Hit::PlaylistPlay, true);
		button(L"Shuffle", icons::shuffle, false, Hit::PlaylistShuffle, false);
	}
	if (openPlaylist > 0)
	{
		button(L"Rename", nullptr, false, Hit::PlaylistRename, false);
		button(L"Delete", nullptr, false, Hit::PlaylistDelete, false);
	}
	if (!notice.empty()) g.text(notice, { px + 8, y, a.right() - px - 8, 30 }, 12, Font::Ui, col::dim);
	y += 30 + 18;

	if (list.empty())
	{
		g.text(openPlaylist == 0 ? L"Nothing here yet. Use the heart on Now playing, or right-click a song."
			: L"Nothing here yet. Right-click a song and choose Add to playlist.",
			{ a.x + 6, y, a.w - 12, 20 }, 13, Font::Ui, col::dim);
		return;
	}
	drawTrackRows({ a.x, y, a.w, a.bottom() - y }, list, Hit::PlaylistRow);
}

namespace
{
	std::wstring songsAndMinutes(size_t songs, int secs)
	{
		std::wstring s = songs == 1 ? L"1 song" : std::to_wstring(songs) + L" songs";
		int min = (secs + 59) / 60;
		if (min >= 60) return s + L", " + std::to_wstring(min / 60) + L" h " + std::to_wstring(min % 60) + L" min";
		return s + L", " + std::to_wstring(min) + L" min";
	}
}

void App::marquee(const std::wstring& s, const Rect& r, float size, Font f, uint32_t rgb)
{
	float tw = g.textWidth(s, size, f);
	if (tw <= r.w) { g.text(s, r, size, f, rgb); return; }
	marqueeActive = true;
	// pause, slide to the end, pause, slide back
	const double pause = 1800, speed = 0.03;   // px per ms
	float over = tw - r.w + 4;
	double slide = over / speed;
	double e = std::fmod(nowMs() - marqueeStart, 2 * (pause + slide));
	float off;
	if (e < pause) off = 0;
	else if (e < pause + slide) off = (float)((e - pause) * speed);
	else if (e < 2 * pause + slide) off = over;
	else off = over - (float)((e - 2 * pause - slide) * speed);
	g.pushClip(r);
	g.text(s, { r.x - off, r.y, tw + 8, r.h }, size, f, rgb);
	g.popClip();
}

void App::drawAlbum(const Rect& a)
{
	listArea = {};
	int al = findAlbum();
	if (al < 0) { page = Page::Library; return; }
	const Group& grp = albums[al];
	float x = a.x + 6, y = a.y;

	std::wstring backLabel = albumFromArtist && findArtist() >= 0 ? artistName : L"Library";
	float bw = g.textWidth(backLabel, 13, Font::UiMedium) + 40;
	if (bw > 260) bw = 260;
	Rect back{ x, y, bw, 30 };
	if (hovered(Hit::GroupBack)) g.fill(back, col::hover, 15);
	g.stroke(back, col::chip, 15);
	g.icon("M15 6l-6 6 6 6", { back.x + 10, back.cy() - 8, 16, 16 }, col::text, false, 1.8f);
	g.text(backLabel, { back.x + 30, back.y, bw - 38, 30 }, 13, Font::UiMedium, col::text);
	hot(back, Hit::GroupBack);
	y += 30 + 22;

	float cs = 180;
	Rect cv{ x, y, cs, cs };
	const std::wstring& first = libTracks[grp.tracks.front()].path;
	ID2D1Bitmap* bmp = coverOf(first);
	bool ch = hovered(Hit::AlbumCover) && !busyImporting();
	if (bmp) g.image(bmp, cv, 3);
	else
	{
		g.fill(cv, col::tile, 3);
		g.icon(icons::note, { cv.cx() - 27, cv.cy() - 27, 54, 54 }, col::muted, false, 1.4f);
	}
	g.stroke(cv, 0x313131, 3, 1, 0.6f);
	if (ch || (!bmp && !busyImporting()))
	{
		Rect lab{ cv.x, cv.bottom() - 34, cs, 34 };
		if (ch) g.fill(cv, 0x000000, 3, 0.45f);
		g.text(bmp ? L"Change cover" : L"Set cover", lab, 12, Font::UiMedium, ch ? col::white : col::soft, Align::Center);
	}
	if (!busyImporting()) hot(cv, Hit::AlbumCover);

	float tx = cv.right() + 24, tw = a.right() - 6 - tx;
	float ty = cv.bottom() - 30 - 18 - 20 - 24 - 34;
	g.text(L"Album", { tx, ty, tw, 18 }, 12, Font::UiMedium, col::dim);
	g.text(grp.name, { tx, ty + 18, tw, 34 }, 26, Font::UiMedium, col::text);
	float aw = g.textWidth(grp.artist, 14, Font::UiMedium);
	if (aw > tw) aw = tw;
	Rect ar{ tx, ty + 18 + 34, aw, 22 };
	bool ah = hovered(Hit::AlbumArtist);
	g.text(grp.artist, ar, 14, Font::UiMedium, ah ? col::white : col::soft);
	if (ah) g.line(ar.x, ar.bottom() - 2.5f, ar.right(), ar.bottom() - 2.5f, col::white, 1);
	hot(ar, Hit::AlbumArtist);
	int secs = 0;
	for (int i : grp.tracks) secs += libTracks[i].info.durationMs / 1000;
	std::wstring sub = songsAndMinutes(grp.tracks.size(), secs);
	const TrackInfo& fi = libTracks[grp.tracks.front()].info;
	if (fi.sampleRate)
		sub += L"  \x00B7  " + codecOf(first) + L" " + (fi.bitsPerSample ? std::to_wstring(fi.bitsPerSample) + L"/" : L"") + fmtRate(fi.sampleRate);
	g.text(sub, { tx, ty + 18 + 34 + 22, tw, 20 }, 13, Font::Ui, col::dim);

	float px = tx, by = cv.bottom() - 30;
	auto button = [&](const wchar_t* label, const char* icon, Hit id, bool primary) {
		float w = 12 + 16 + 8 + g.textWidth(label, 13, Font::UiMedium) + 14;
		Rect b{ px, by, w, 30 };
		if (primary) g.fill(b, hovered(id) ? 0xFFFFFF : col::text, 15);
		else { if (hovered(id)) g.fill(b, col::hover, 15); g.stroke(b, col::chip, 15); }
		uint32_t fg = primary ? 0x131313 : col::text;
		g.icon(icon, { b.x + 12, b.cy() - 8, 16, 16 }, fg, primary, 1.6f);
		g.text(label, { b.x + 36, b.y, w - 40, 30 }, 13, Font::UiMedium, fg);
		hot(b, id);
		px += w + 8;
	};
	button(L"Play", icons::play, Hit::GroupPlay, true);
	button(L"Shuffle", icons::shuffle, Hit::GroupShuffle, false);
	if (!notice.empty()) g.text(notice, { px + 8, by, a.right() - px - 14, 30 }, 12, Font::Ui, col::dim);
	y = cv.bottom() + 28;

	std::vector<Track> list;
	for (int i : grp.tracks) list.push_back(libTracks[i]);
	drawTrackRows({ a.x, y, a.w, a.bottom() - y }, list, Hit::AlbumRow);
}

void App::drawSmart(const Rect& a)
{
	listArea = {};
	std::vector<int> idx = smartTracks();
	float x = a.x + 6, y = a.y;
	std::vector<Track> list;
	for (int i : idx) list.push_back(libTracks[i]);
	Rect tile{ x, y, 96, 96 };
	drawMosaic(tile, list, icons::sparkle);
	float tx = tile.right() + 24;
	g.text(L"Smart playlist", { tx, y + 2, 300, 18 }, 12, Font::Ui, col::soft);
	g.text(L"Lossless 24-bit", { tx, y + 20, a.right() - tx, 32 }, 22, Font::UiMedium, col::text);
	std::wstring count = (idx.size() == 1 ? L"1 song" : std::to_wstring(idx.size()) + L" songs");
	g.text(L"Lossless files at 24-bit or more \x00B7 " + count + L" \x00B7 updates by itself",
		{ tx, y + 52, a.right() - tx, 20 }, 13, Font::Ui, col::dim);
	y += 96 + 28;
	if (idx.empty())
	{
		g.text(L"No 24-bit lossless songs in your library yet. They show up here as soon as you import some.", { x, y, a.w - 12, 20 }, 13, Font::Ui, col::dim);
		return;
	}
	drawTrackRows({ a.x, y, a.w, a.bottom() - y }, list, Hit::SmartRow);
}

void App::drawArtist(const Rect& a)
{
	listArea = {};
	int ai = findArtist();
	if (ai < 0) { page = Page::Library; return; }
	const Group& grp = artists[ai];
	pageArea = a;
	float x = a.x + 6;
	g.pushClip(a);
	float y = a.y - pageScroll;

	float bw = g.textWidth(L"Library", 13, Font::UiMedium) + 40;
	Rect back{ x, y, bw, 30 };
	if (hovered(Hit::GroupBack)) g.fill(back, col::hover, 15);
	g.stroke(back, col::chip, 15);
	g.icon("M15 6l-6 6 6 6", { back.x + 10, back.cy() - 8, 16, 16 }, col::text, false, 1.8f);
	g.text(L"Library", { back.x + 30, back.y, bw - 38, 30 }, 13, Font::UiMedium, col::text);
	hotVisible(back, Hit::GroupBack, -1);
	y += 30 + 22;

	artistAlbums.clear();
	for (int i = 0; i < (int)albums.size(); ++i)
		if (_wcsicmp(albums[i].artist.c_str(), grp.name.c_str()) == 0) artistAlbums.push_back(i);

	float cs = 140;
	g.circle(x + cs / 2, y + cs / 2, cs / 2, 0x2E2E2E);
	std::wstring initial = grp.name.empty() ? L"?" : grp.name.substr(0, 1);
	g.text(initial, { x, y, cs, cs }, cs * 0.32f, Font::UiMedium, col::dim, Align::Center);
	float tx = x + cs + 24, tw = a.right() - 6 - tx;
	float ty = y + cs - 30 - 18 - 20 - 18 - 34;
	g.text(L"Artist", { tx, ty, tw, 18 }, 12, Font::UiMedium, col::dim);
	g.text(grp.name, { tx, ty + 18, tw, 34 }, 26, Font::UiMedium, col::text);
	int secs = 0;
	for (int i : grp.tracks) secs += libTracks[i].info.durationMs / 1000;
	std::wstring sub = (artistAlbums.size() == 1 ? L"1 album, " : std::to_wstring(artistAlbums.size()) + L" albums, ") + songsAndMinutes(grp.tracks.size(), secs);
	g.text(sub, { tx, ty + 18 + 34, tw, 20 }, 13, Font::Ui, col::dim);

	float px = tx, by = y + cs - 30;
	auto button = [&](const wchar_t* label, const char* icon, Hit id, bool primary) {
		float w = 12 + 16 + 8 + g.textWidth(label, 13, Font::UiMedium) + 14;
		Rect b{ px, by, w, 30 };
		if (primary) g.fill(b, hovered(id) ? 0xFFFFFF : col::text, 15);
		else { if (hovered(id)) g.fill(b, col::hover, 15); g.stroke(b, col::chip, 15); }
		uint32_t fg = primary ? 0x131313 : col::text;
		g.icon(icon, { b.x + 12, b.cy() - 8, 16, 16 }, fg, primary, 1.6f);
		g.text(label, { b.x + 36, b.y, w - 40, 30 }, 13, Font::UiMedium, fg);
		hotVisible(b, id, -1);
		px += w + 8;
	};
	button(L"Play", icons::play, Hit::GroupPlay, true);
	button(L"Shuffle", icons::shuffle, Hit::GroupShuffle, false);
	y += cs + 30;

	g.text(L"Albums", { x, y, 200, 24 }, 16, Font::UiMedium, col::text);
	y += 24 + 12;
	const std::wstring* playing = current >= 0 ? &queue[current].path : nullptr;
	std::vector<Tile> tiles;
	for (int k : artistAlbums)
	{
		const Group& al = albums[k];
		bool on = false;
		for (int i : al.tracks) if (playing && *playing == libTracks[i].path) on = true;
		std::wstring s = al.tracks.size() == 1 ? L"1 song" : std::to_wstring(al.tracks.size()) + L" songs";
		tiles.push_back({ al.name, s, libTracks[al.tracks.front()].path, on });
	}
	y += drawTiles(tiles, a.x, y, a.w, Hit::ArtistAlbum, false) + 24;
	g.popClip();
	pageMaxScroll = (y + pageScroll) - a.bottom();
	if (pageMaxScroll < 0) pageMaxScroll = 0;
}

ID2D1Bitmap* App::currentCover()
{
	if (current < 0) return nullptr;
	if (coverFor != current || coverGen != g.generation())
	{
		cover = g.loadImage(queue[current].coverBytes, (UINT)(720 * scale));
		coverFor = current;
		coverGen = g.generation();
	}
	return cover.Get();
}

std::wstring App::signalSource() const
{
	if (current < 0) return L"";
	const TrackInfo& i = queue[current].info;
	std::wstring s = codecOf(queue[current].path);
	if (i.sampleRate)
		s += L" " + (i.bitsPerSample ? std::to_wstring(i.bitsPerSample) + L"/" : L"") + fmtRate(i.sampleRate);
	return s;
}

std::wstring App::signalOutput() const
{
	return fmtRate((int)player.outputSampleRate()) + L" kHz";
}

void App::drawNowPlaying(const Rect& a)
{
	listArea = {};
	if (current < 0)
	{
		drawEmpty(a, L"Nothing playing", L"Drop audio files on this window, or open them, and they play here.", true);
		return;
	}
	const Track& tr = queue[current];
	const TrackInfo& info = tr.info;

	float coverW = (a.w - 36) * 0.4f;
	if (coverW > 340) coverW = 340;
	// the cover ends level with the line under the tabs
	if (npCoverBottom > 0 && npCoverBottom - 4 < coverW) coverW = npCoverBottom - 4;
	if (coverW < 160) coverW = 160;
	Rect cv{ a.x + 6, a.y + 4, coverW, coverW };
	if (ID2D1Bitmap* bmp = currentCover()) g.image(bmp, cv, 2);
	else g.fill(cv, col::tile, 2);
	g.stroke(cv, 0x313131, 2, 1, 0.6f);

	float x = cv.right() + 36, w = a.right() - x;
	float y = a.y + 4;
	State st = player.getState();
	g.text(!error.empty() ? error : st == State::Playing ? L"Now playing" : L"Paused", { x, y, w, 14 }, 12, Font::Ui, col::white);
	y += 24;
	y += g.textBlock(tr.title, { x, y, w, 120 }, 30, Font::UiMedium, col::text, 3, 34.5f) + 8;
	g.text(tr.artist, { x, y, w, 20 }, 15, Font::Ui, col::text);
	y += 22;
	if (!tr.album.empty()) { g.text(tr.album, { x, y, w, 18 }, 14, Font::Ui, col::soft); y += 22; }
	y += 18;

	// format chips
	std::vector<std::wstring> chips;
	chips.push_back(codecOf(tr.path));
	if (info.sampleRate)
	{
		std::wstring dr = info.bitsPerSample ? std::to_wstring(info.bitsPerSample) + L"-bit / " : L"";
		chips.push_back(dr + fmtRate(info.sampleRate) + L" kHz");
	}
	if (info.bitrateKbps)
	{
		wchar_t b[32];
		int k = info.bitrateKbps;
		if (k >= 1000) swprintf(b, 32, L"%d,%03d kbps", k / 1000, k % 1000);
		else swprintf(b, 32, L"%d kbps", k);
		chips.push_back(b);
	}
	if (info.hasTrackGain)
	{
		wchar_t b[32];
		swprintf(b, 32, L"RG %+.2f dB", info.trackGainDb);
		chips.push_back(b);
	}
	float cx = x;
	for (size_t i = 0; i < chips.size(); ++i)
	{
		float cw = g.textWidth(chips[i], 12, Font::Mono) + 16;
		if (cx + cw > a.right()) break;
		Rect c{ cx, y, cw, 22 };
		g.stroke(c, col::chip, 2);
		g.text(chips[i], c, 12, Font::Mono, i == 0 ? col::text : col::soft, Align::Center);
		cx += cw + 6;
	}
	y += 22 + 18;

	// signal path: Source [FLAC 24/96] -> WASAPI shared [48 kHz] -> o Resampled 96 -> 48 kHz
	{
		Rect row{ x, y, w, 42 };
		g.line(x, y + 0.5f, a.right(), y + 0.5f, col::line2, 1);
		g.line(x, y + 41.5f, a.right(), y + 41.5f, col::line2, 1);
		float px = x, cyy = y + 21;
		auto label = [&](const std::wstring& s, uint32_t c) {
			float tw = g.textWidth(s, 13, Font::Mono);
			g.text(s, { px, cyy - 10, tw + 2, 20 }, 13, Font::Mono, c);
			px += tw + 10;
		};
		auto box = [&](const std::wstring& s) {
			float tw = g.textWidth(s, 13, Font::Mono) + 12;
			Rect b{ px, cyy - 11, tw, 22 };
			g.stroke(b, hovered(Hit::SignalPath) ? 0x5A5A5A : col::tile, 2);
			g.text(s, b, 13, Font::Mono, col::text, Align::Center);
			px += tw + 10;
		};
		std::wstring backend = widen(player.backendName());
		label(L"Source", col::muted);
		box(signalSource());
		label(L"→", col::dim);
		label(player.asioActive() ? backend : backend + (player.exclusiveActive() ? L" exclusive" : L" shared"), col::muted);
		box(signalOutput());
		label(L"→", col::dim);
		bool same = info.sampleRate == (int)player.outputSampleRate();
		if (player.bitPerfect(current >= 0 ? queue[current].info.bitsPerSample : 0))
		{
			g.circle(px + 4, cyy, 3.5f, col::white);
			px += 14;
			label(L"Bit-perfect", col::white);
		}
		else if (player.equalizerActive())
		{
			g.circle(px + 4, cyy, 3, col::dim, false, 1);
			px += 14;
			label(L"EQ active, not bit-perfect", col::dim);
		}
		else if ((player.exclusiveActive() || player.asioActive()) && same)
		{
			g.circle(px + 4, cyy, 3, col::dim, false, 1);
			px += 14;
			label(volume < 0.9999f ? L"Volume below 100%, not bit-perfect" : L"Converted, not bit-perfect", col::dim);
		}
		else if (same)
		{
			g.circle(px + 4, cyy, 3.5f, col::white);
			px += 14;
			label(L"No resampling", col::white);
		}
		else
		{
			g.circle(px + 4, cyy, 3, col::dim, false, 1);
			px += 14;
			label(info.sampleRate ? L"Resampled " + fmtRate(info.sampleRate) + L" → " + signalOutput()
				: L"Resampled to " + signalOutput(), col::dim);
		}
		hot(row, Hit::SignalPath);
		y += 42 + 18;
	}

	{
		bool have = inLibrary(tr.path);
		float bx = x;
		auto button = [&](const std::wstring& label, const char* icon, bool filled, bool on, Hit id) {
			float bw = 12 + 16 + 8 + g.textWidth(label, 12, Font::Ui) + 12;
			Rect b{ bx, y, bw, 30 };
			if (on) g.fill(b, 0x272727, 3);
			else if (hovered(id)) g.fill(b, col::hover, 3);
			g.stroke(b, col::chip, 3);
			g.icon(icon, { b.x + 12, b.cy() - 8, 16, 16 }, col::text, filled, 1.6f);
			g.text(label, { b.x + 36, b.y, bw - 44, 30 }, 12, Font::Ui, col::text);
			hot(b, id);
			bx = b.right() + 8;
		};
		if (have)
		{
			bool fav = inPlaylist(0, tr.path);
			button(fav ? L"In Favourites" : L"Favourite", icons::heart, fav, fav, Hit::Favourite);
			button(L"Go to album", icons::disc, false, false, Hit::GoToAlbum);
		}
		else button(L"Add to library", icons::plus, false, false, Hit::AddToLibrary);
		float nx = bx + 4;
		if (have)
		{
			Rect more{ bx, y, 40, 30 };
			if (hovered(Hit::NowMore) || (menuOpen && nowMoreAt.x == more.x && nowMoreAt.y == more.y)) g.fill(more, col::hover, 3);
			g.stroke(more, col::chip, 3);
			for (int d = -1; d <= 1; ++d) g.circle(more.cx() + d * 5.0f, more.cy(), 1.6f, col::text);
			hot(more, Hit::NowMore);
			nowMoreAt = more;
			nx = more.right() + 12;
		}
		if (!notice.empty()) g.text(notice, { nx, y, a.right() - nx, 30 }, 12, Font::Ui, col::dim);
		y += 30 + 24;
	}

	// tabs
	const wchar_t* tabs[] = { L"Up next", L"About album", L"Lyrics" };
	const Hit tabHits[] = { Hit::TabNext, Hit::TabAbout, Hit::TabLyrics };
	float tx = x;
	for (int i = 0; i < 3; ++i)
	{
		float tw = g.textWidth(tabs[i], 14, Font::UiMedium);
		Rect r{ tx, y, tw, 36 };
		bool on = npTab == i;
		g.text(tabs[i], r, 14, Font::UiMedium, on || hovered(tabHits[i]) ? col::text : col::muted);
		if (on) g.fill({ tx, y + 34, tw, 2 }, col::text);
		hot(r, tabHits[i]);
		tx += tw + 22;
	}
	g.line(x, y + 35.5f, a.right(), y + 35.5f, 0xEBEBEB, 1, 0.16f);
	if (std::fabs((y + 36 - a.y) - npCoverBottom) > 0.5f)
	{
		npCoverBottom = y + 36 - a.y;   // the title can wrap differently at the new width; settle next frame
		InvalidateRect(hwnd, nullptr, FALSE);
	}
	y += 36 + 6;

	Rect body{ x, y, w, a.bottom() - y };
	if (npTab == 0)
	{
		bool any = false;
		int upPos = 0;
		for (int i : upcoming())
		{
			if (y + 46 > a.bottom()) break;
			const Track& q = queue[i];
			Rect r{ x - 10, y, w + 10, 46 };
			bool held = qDragging && qDragItem == i;
			bool hov = !qDragging && hovered(Hit::QueueRow, i), hovX = !qDragging && hovered(Hit::QueueRemove, i);
			if (held) g.fill(r, col::selected, 2);
			else if (hov || hovX) g.fill(r, col::hover, 2);
			Rect cvr{ x, y + 3, 40, 40 };
			if (ID2D1Bitmap* b = coverOf(q.path)) g.image(b, cvr, 2);
			else g.fill(cvr, col::tile, 2);
			if (hov || hovX || held)
			{
				g.fill(cvr, 0x000000, 2, 0.55f);
				for (int gy = 0; gy < 3; ++gy)
					for (int gx = 0; gx < 2; ++gx)
						g.circle(cvr.cx() - 3 + gx * 6, cvr.cy() - 6 + gy * 6, 1.4f, col::white);
			}
			g.text(q.title, { x + 52, y + 6, w - 52 - 56, 18 }, 13, Font::UiMedium, held ? col::muted : col::text);
			g.text(q.artist, { x + 52, y + 24, w - 52 - 56, 16 }, 12, Font::Ui, held ? col::muted : col::soft);
			Rect xb{ x + w - 34, y + 9, 28, 28 };
			if (hov || hovX)
			{
				if (hovX) g.fill(xb, col::ctlHover, 14);
				g.icon(icons::close, { xb.cx() - 7, xb.cy() - 7, 14, 14 }, hovX ? col::white : col::soft, false, 1.8f);
			}
			else g.text(fmtTime(q.info.durationMs / 1000.0), { x + w - 54, y, 44, 46 }, 12, Font::Mono, col::soft, Align::Right);
			hot(r, Hit::QueueRow, i);
			hot(xb, Hit::QueueRemove, i);
			qRows.push_back({ i, upPos++, 2, r });
			y += 46;
			any = true;
		}
		if (qDragging && qDragList == 2 && qDropSlot >= 0)
		{
			float ly = -1;
			for (const QRow& r : qRows) if (r.list == 2 && r.up == qDropSlot) ly = r.r.y;
			if (ly < 0) for (const QRow& r : qRows) if (r.list == 2) ly = r.r.bottom();
			if (ly >= 0) { g.fill({ x - 6, ly - 1, w + 2, 2 }, col::text, 1); g.circle(x - 6, ly, 3.5f, col::text); }
		}
		if (!any) g.text(L"Nothing queued. Right-click a song and pick Play next or Add to queue.", { x, y + 4, w, 20 }, 13, Font::Ui, col::muted);
	}
	else if (npTab == 1)
	{
		// the album as the library knows it
		nowAlbum = -1;
		for (int k = 0; k < (int)albums.size() && nowAlbum < 0; ++k)
			for (int t : albums[k].tracks)
				if (libTracks[t].path == tr.path) { nowAlbum = k; break; }
		const Group* al = nowAlbum >= 0 ? &albums[nowAlbum] : nullptr;
		int albumSecs = 0;
		if (al) for (int t : al->tracks) albumSecs += libTracks[t].info.durationMs / 1000;

		struct Row { const wchar_t* k; std::wstring v; bool mono; std::wstring extra; };
		std::vector<Row> rows = {
			{ L"Album", tr.album.empty() ? L"Unknown" : tr.album, false, L"" },
			{ L"Artist", tr.artist, false, L"" },
			{ L"Released", info.year ? std::to_wstring(info.year) : L"Unknown", false, L"" },
		};
		if (al)
		{
			rows.push_back({ L"Tracks", std::to_wstring(al->tracks.size()) + (al->tracks.size() == 1 ? L" track" : L" tracks"), false, L"" });
			rows.push_back({ L"Length", fmtTime(albumSecs), true, L"" });
			unsigned long long albumBytes = 0;
			for (int t : al->tracks) albumBytes += libTracks[t].bytes;
			if (albumBytes) rows.push_back({ L"Size", fmtSize(albumBytes), false, L"" });
		}
		// Plain, like the player bar: "FLAC · 24-bit · 44.1 kHz". No quality badge.
		std::wstring fmtLine = codecOf(tr.path);
		if (info.bitsPerSample) fmtLine += L" \x00B7 " + std::to_wstring(info.bitsPerSample) + L"-bit";
		if (info.sampleRate) fmtLine += L" \x00B7 " + fmtRate(info.sampleRate) + L" kHz";
		rows.push_back({ L"Format", fmtLine, false, L"" });

		// Wide enough: details on the left, the album's songs beside them. Otherwise stacked.
		bool side = al && w >= 480;
		float infoW = side ? w * 0.44f : w;
		float top = y;
		for (const Row& r : rows)
		{
			if (y + 26 > a.bottom()) break;
			g.text(r.k, { x, y, 80, 26 }, 13, Font::Ui, col::muted);
			float vx = x + 78, vw = infoW - 78;
			if (r.extra.empty()) marquee(r.v, { vx, y, vw, 26 }, 13, r.mono ? Font::Mono : Font::Ui, col::text);
			else
			{
				// the quality as a badge beside the format, when there's room for it
				g.text(r.v, { vx, y, vw, 26 }, 13, r.mono ? Font::Mono : Font::Ui, col::text);
				float tw = g.textWidth(r.v, 13, r.mono ? Font::Mono : Font::Ui) + 10;
				float bw = g.textWidth(r.extra, 11, Font::Ui) + 14;
				if (tw + bw <= vw)
				{
					Rect badge{ vx + tw, y + 3, bw, 20 };
					g.stroke(badge, col::chip, 3);
					g.text(r.extra, badge, 11, Font::Ui, col::soft, Align::Center);
				}
			}
			y += 30;
		}

		nowAlbumArea = {};
		if (al)
		{
			float lx = x, lw = w;
			if (side) { lx = x + w * 0.47f; lw = w - w * 0.47f; y = top + 10; }
			else y += 12;
			// the list sits on its own card
			int n = (int)al->tracks.size();
			const float rowH = 30, pad = 6;
			float cardH = std::min(std::min(n, 5) * rowH + pad * 2, a.bottom() - 4 - y);   // five rows at most; the rest scroll
			Rect card{ lx - 8, y, lw + 8, cardH };
			g.fill(card, 0x202020, 6);
			g.stroke(card, 0x2C2C2C, 6);
			lx += 6; lw -= 12;
			y += pad;
			nowAlbumArea = { card.x + 1, y, card.w - 2, cardH - pad * 2 };
			nowAlbumMaxScroll = std::max(0.0f, n * rowH - nowAlbumArea.h);
			int here = 0;
			for (int k = 0; k < n; ++k) if (libTracks[al->tracks[k]].path == tr.path) here = k;
			if (nowAlbumFor != tr.path)   // new song: bring it into view
			{
				nowAlbumFor = tr.path;
				marqueeStart = nowMs();
				float want = here * rowH - rowH;
				nowAlbumScroll = want < 0 ? 0 : want;
			}
			if (nowAlbumScroll > nowAlbumMaxScroll) nowAlbumScroll = nowAlbumMaxScroll;

			g.pushClip(nowAlbumArea);
			for (int k = 0; k < n; ++k)
			{
				float ry = y + k * rowH - nowAlbumScroll;
				if (ry + rowH < nowAlbumArea.y) continue;
				if (ry > nowAlbumArea.bottom()) break;
				const Track& t = libTracks[al->tracks[k]];
				bool on = k == here;
				Rect r{ card.x + 4, ry, card.w - 8, rowH };
				if (hovered(Hit::NowAlbumRow, k)) g.fill(r, 0x2A2A2A, 4);
				int no = t.info.track ? t.info.track : k + 1;
				g.text(std::to_wstring(no), { lx, ry, 24, rowH }, 12, on ? Font::UiSemibold : Font::Mono, on ? col::white : col::muted);
				g.text(t.title, { lx + 30, ry, lw - 30 - 58, rowH }, 13, on ? Font::UiMedium : Font::Ui, on ? col::white : col::text);
				g.text(fmtTime(t.info.durationMs / 1000.0), { lx + lw - 56, ry, 50, rowH }, 12, Font::Mono, on ? col::text : col::soft, Align::Right);
				if (k < n - 1) g.line(lx, ry + rowH - 0.5f, lx + lw, ry + rowH - 0.5f, 0x2A2A2A, 1);
				float t0 = std::max(ry, nowAlbumArea.y), t1 = std::min(ry + rowH, nowAlbumArea.bottom());
				if (t1 > t0) hot({ r.x, t0, r.w, t1 - t0 }, Hit::NowAlbumRow, k);
			}
			g.popClip();
			if (nowAlbumMaxScroll > 0)
			{
				float th = std::max(20.0f, nowAlbumArea.h * nowAlbumArea.h / (n * rowH));
				float ty = nowAlbumArea.y + (nowAlbumArea.h - th) * (nowAlbumScroll / nowAlbumMaxScroll);
				g.fill({ card.right() - 5, ty, 2, th }, 0x4A4A4A, 1);
			}
		}
	}
	else
		drawLyrics(tr, body);
}

namespace
{
	// Plain lines for display: LRC timestamps and [ar:...] style headers are dropped.
	std::vector<std::wstring> lyricDisplayLines(const std::wstring& text)
	{
		std::vector<std::wstring> lines;
		size_t start = 0;
		while (start <= text.size())
		{
			size_t end = text.find(L'\n', start);
			if (end == std::wstring::npos) end = text.size();
			std::wstring line = text.substr(start, end - start);
			if (!line.empty() && line.back() == L'\r') line.pop_back();
			bool header = false;
			while (line.size() > 2 && line[0] == L'[')
			{
				size_t close = line.find(L']');
				if (close == std::wstring::npos) break;
				bool stamp = close > 1 && iswdigit(line[1]);
				if (!stamp && close == line.size() - 1 && line.find(L':') < close) header = true;
				if (!stamp) break;
				line.erase(0, close + 1);
			}
			if (!header) lines.push_back(line);
			start = end + 1;
		}
		while (!lines.empty() && lines.back().find_first_not_of(L" \t") == std::wstring::npos) lines.pop_back();
		return lines;
	}
}

void App::drawLyrics(const Track& tr, const Rect& body)
{
	if (lyricsPath != tr.path)
	{
		lyricsPath = tr.path;
		lyricScroll = 0;
		lyricSelA = lyricSelB = -1;
	}
	const std::wstring& text = lyricsFor(tr);
	lyricLines = lyricDisplayLines(text);
	bool editable = inLibrary(tr.path) && !busyImporting();
	float x = body.x, w = body.w, y = body.y;

	// a row of small text actions
	float lx = x;
	auto action = [&](const std::wstring& label, Hit id) {
		float tw = g.textWidth(label, 12, Font::Ui);
		Rect r{ lx, y + 2, tw, 20 };
		bool hov = hovered(id);
		g.text(label, r, 12, Font::Ui, hov ? col::white : col::soft);
		g.line(r.x, r.bottom() - 2.5f, r.right(), r.bottom() - 2.5f, hov ? col::white : 0x5A5A5A, 1);
		hot(r.inset(-4), id);
		lx += tw + 18;
	};
	if (lyricLines.empty())
	{
		g.text(L"No lyrics in this file.", { x, y, w, 24 }, 14, Font::Ui, col::muted);
		y += 30;
		if (editable)
		{
			action(L"Paste from clipboard", Hit::LyricPaste);
			action(L"Load .lrc or .txt\x2026", Hit::LyricFile);
		}
		else g.text(L"Add the song to your library to save lyrics into it.", { x, y + 2, w, 20 }, 12, Font::Ui, col::dim);
		lyricLines.clear();
		lyricTops.clear();
		lyricArea = {};
		return;
	}
	int a = std::min(lyricSelA, lyricSelB), b = std::max(lyricSelA, lyricSelB);
	action(lyricSelA < 0 ? L"Copy" : a == b ? L"Copy line" : L"Copy " + std::to_wstring(b - a + 1) + L" lines", Hit::LyricCopy);
	action(L"Export\x2026", Hit::LyricExport);
	if (editable)
	{
		action(L"Replace from clipboard", Hit::LyricPaste);
		action(L"Load file\x2026", Hit::LyricFile);
		action(L"Remove", Hit::LyricRemove);
	}
	y += 34;

	lyricArea = { x - 10, y, w + 10, body.bottom() - y };
	lyricTops.clear();
	float cy = 0;
	std::vector<float> heights;
	for (const std::wstring& line : lyricLines)
	{
		lyricTops.push_back(cy);
		float h = line.empty() ? 22.0f : std::max(22.0f, g.textHeight(line, w, 14, Font::Ui, 22));
		heights.push_back(h);
		cy += h;
	}
	lyricMaxScroll = cy - lyricArea.h + 8;
	if (lyricMaxScroll < 0) lyricMaxScroll = 0;
	if (lyricScroll > lyricMaxScroll) lyricScroll = lyricMaxScroll;

	g.pushClip(lyricArea);
	for (int i = 0; i < (int)lyricLines.size(); ++i)
	{
		float ly = lyricArea.y + lyricTops[i] - lyricScroll;
		if (ly + heights[i] < lyricArea.y) continue;
		if (ly > lyricArea.bottom()) break;
		Rect r{ lyricArea.x, ly, lyricArea.w, heights[i] };
		bool sel = lyricSelA >= 0 && i >= a && i <= b;
		if (sel) g.fill(r, col::selected, 2);
		else if (hovered(Hit::LyricLine, i) && !lyricLines[i].empty()) g.fill(r, col::hover, 2, 0.6f);
		if (!lyricLines[i].empty())
			g.textBlock(lyricLines[i], { x, ly, w, heights[i] }, 14, Font::Ui, sel ? col::white : col::soft, 8, 22);
		float top = std::max(ly, lyricArea.y), bot = std::min(ly + heights[i], lyricArea.bottom());
		if (bot > top) hot({ r.x, top, r.w, bot - top }, Hit::LyricLine, i);
	}
	g.popClip();

	if (lyricMaxScroll > 0)
	{
		float trackH = lyricArea.h, thumbH = std::max(24.0f, trackH * lyricArea.h / (cy + 8));
		float ty = lyricArea.y + (trackH - thumbH) * (lyricScroll / lyricMaxScroll);
		bool active = hovered(Hit::LyricScroll) || (dragging && pressHit == Hit::LyricScroll);
		g.fill({ lyricArea.right() + (active ? 4.0f : 6.0f), ty, active ? 6.0f : 3.0f, thumbH }, active ? 0x6A6A6A : 0x4A4A4A, 3);
		lyricThumbH = thumbH;
		hot({ lyricArea.right() + 1, lyricArea.y, 12, trackH }, Hit::LyricScroll);
	}
}

void App::drawSettings(const Rect& a)
{
	float x = a.x + 6, w = a.w - 12;
	if (w > 880) w = 880;
	listArea = {};
	pageArea = a;
	float y = a.y - pageScroll;
	g.text(L"Settings", { x, y, w, 30 }, 22, Font::UiMedium, col::text);
	y += 52;

	auto section = [&](const wchar_t* title) {
		g.text(title, { x, y, w, 18 }, 13, Font::UiMedium, col::dim);
		y += 24;
	};
	auto row = [&](const std::wstring& label, const std::wstring& sub, const std::wstring& value, bool mono, bool last) {
		g.line(x, y + 0.5f, x + w, y + 0.5f, col::line, 1);
		g.text(label, { x, y + (sub.empty() ? 0.0f : 11.0f), w * 0.55f, sub.empty() ? 60.0f : 20.0f }, 14, Font::Ui, col::text);
		if (!sub.empty()) g.text(sub, { x, y + 31, w * 0.55f, 18 }, 12, mono ? Font::Mono : Font::Ui, col::dim);
		g.text(value, { x + w * 0.45f, y, w * 0.55f, 60 }, 13, Font::Ui, col::soft, Align::Right);
		y += 60;
		if (last) { g.line(x, y - 0.5f, x + w, y - 0.5f, col::line, 1); y += 30; }
	};

	section(L"Audio");
	{
		auto rowLine = [&]() { g.line(x, y + 0.5f, x + w, y + 0.5f, col::line, 1); };

		// A dropdown button at the right of a row; widths stay steady between choices.
		auto dropdown = [&](const std::wstring& label, Hit id) {
			float dw = std::min(std::max(g.textWidth(label, 13, Font::Ui) + 50, 130.0f), w * 0.55f);
			Rect db{ x + w - dw, y + 13, dw, 34 };
			float h = anim(id, -1, hovered(id));
			g.fill(db, 0x202020, 6);
			if (h > 0.01f) g.fill(db, col::ctlHover, 6, h);
			g.stroke(db, h > 0.5f ? 0x4A4A4A : col::chip, 6);
			g.text(label, { db.x + 14, db.y, db.w - 44, db.h }, 13, Font::Ui, col::text);
			g.icon("M6 9l6 6 6-6", { db.right() - 28, db.cy() - 8, 16, 16 }, col::soft, false, 1.8f);
			hotVisible(db, id, -1);
			return db;
		};

		// Output: a dropdown with the device
		rowLine();
		g.text(L"Output", { x, y + 11, w * 0.4f, 20 }, 14, Font::Ui, col::text);
		g.text(widen(player.backendName()).empty() ? L"No device open" : widen(player.backendName()),
			{ x, y + 31, w * 0.4f, 18 }, 12, Font::Ui, col::dim);
		std::wstring dev = audioOk && player.deviceName()[0] ? widen(player.deviceName())
			: audioMode == 2 ? (asioDriver.empty() ? L"First ASIO driver" : asioDriver) : L"No device";
		if (audioMode != 2 && outputDevice.empty()) dev = L"System default \x00B7 " + dev;
		deviceButton = dropdown(dev, Hit::DeviceMenu);
		y += 60;

		// Mode: shared, exclusive or ASIO, as a dropdown like Output
		rowLine();
		g.text(L"Mode", { x, y + 11, w * 0.5f, 20 }, 14, Font::Ui, col::text);
		g.text(audioMode == 2 ? L"Straight to the device's own driver"
			: audioMode == 1 ? L"LossCat has the device to itself"
			: L"Windows mixes LossCat with other apps",
			{ x, y + 31, w * 0.6f, 18 }, 12, Font::Ui, col::dim);
		const wchar_t* modes[] = { L"Shared", L"Exclusive", L"ASIO" };
		modeButton = dropdown(modes[audioMode], Hit::ModeMenu);
		y += 60;

		// What the device is getting right now
		rowLine();
		g.text(L"Right now", { x, y + 11, 200, 20 }, 14, Font::Ui, col::text);
		std::wstring status;
		uint32_t statusCol = col::dim;
		bool filled = false;
		if (player.deviceSampleRate() == 0) status = L"Opens when a song plays";
		else if (player.bitPerfect(current >= 0 ? queue[current].info.bitsPerSample : 0)) { status = L"Bit-perfect"; statusCol = col::white; filled = true; }
		else if (player.equalizerActive()) status = L"EQ active, not bit-perfect";
		else if (current >= 0 && queue[current].info.sampleRate && queue[current].info.sampleRate != (int)player.deviceSampleRate())
			status = L"Resampled from " + fmtRate(queue[current].info.sampleRate) + L" kHz";
		else if ((player.exclusiveActive() || player.asioActive()) && volume < 0.9999f) status = L"Not bit-perfect: volume below 100%";
		else if (player.exclusiveActive() || player.asioActive()) status = L"Converted, not bit-perfect";
		else if (current >= 0) status = L"Mixed by Windows";
		std::wstring sub = !player.exclusiveProblem().empty() && audioMode != 0 ? widen(player.exclusiveProblem()) : status;
		if (filled) g.circle(x + 4, y + 40, 3.5f, statusCol);
		else if (player.deviceSampleRate()) g.circle(x + 4, y + 40, 3, col::dim, false, 1);
		g.text(sub, { x + (player.deviceSampleRate() ? 14.0f : 0.0f), y + 31, w * 0.5f, 18 }, 12, Font::Ui, statusCol);
		if (player.deviceSampleRate())
		{
			std::vector<std::wstring> chips;
			chips.push_back(player.asioActive() ? L"ASIO" : player.exclusiveActive() ? L"Exclusive" : L"Shared");
			chips.push_back(fmtRate((int)player.deviceSampleRate()) + L" kHz");
			switch (player.deviceFormat())
			{
			case ma_format_s16: chips.push_back(L"16-bit"); break;
			case ma_format_s24: chips.push_back(L"24-bit"); break;
			case ma_format_s32: chips.push_back(L"32-bit"); break;
			case ma_format_f32: chips.push_back(L"32-bit float"); break;
			default: break;
			}
			// A 24-bit file in a 32-bit slot is still exact: the low 8 bits are just zeros. Say so.
			int fileBits = current >= 0 ? queue[current].info.bitsPerSample : 0;
			if (player.bitPerfect(fileBits) && fileBits && (player.deviceFormat() == ma_format_s32 && fileBits < 32))
				chips.back() = std::to_wstring(fileBits) + L"-bit in 32";
			ma_uint32 chn = player.deviceChannels();
			chips.push_back(chn == 2 ? L"Stereo" : chn == 1 ? L"Mono" : std::to_wstring(chn) + L" channels");
			std::wstring joined;
			for (size_t k = 0; k < chips.size(); ++k) joined += (k ? L" \x00B7 " : L"") + chips[k];
			g.text(joined, { x + w * 0.4f, y, w * 0.6f, 60 }, 13, Font::Ui, col::soft, Align::Right);
		}
		y += 60;
		if (audioMode == 1 && !player.exclusiveProblem().empty() && !player.exclusiveAttempts().empty())
		{
			// what was tried, so a device that won't cooperate can be diagnosed
			float h = g.textBlock(L"Tried: " + widen(player.exclusiveAttempts()), { x, y, w, 0 }, 11, Font::Mono, col::muted, 4, 16);
			y += h + 10;
		}
		g.line(x, y - 0.5f, x + w, y - 0.5f, col::line, 1);
		y += 30;
	}

	section(L"Equalizer");
	{
		// On/off and Reset on the first row, the eleven sliders under it.
		g.line(x, y + 0.5f, x + w, y + 0.5f, col::line, 1);
		g.text(L"Equalizer", { x, y + 11, w * 0.55f, 20 }, 14, Font::Ui, col::text);
		std::wstring sub = eqOn ? L"Stereo EQ in all modes; active EQ isn't bit-perfect"
			: L"Off: music plays untouched";
		g.text(sub, { x, y + 31, w * 0.45f, 18 }, 12, Font::Ui, col::dim);

		Rect sw{ x + w - 44, y + 19, 44, 24 };
		float on = anim(Hit::EqToggle, -1, eqOn);
		g.fill(sw, eqOn ? col::text : 0x333333, 12);
		if (hovered(Hit::EqToggle)) g.stroke(sw, 0x5A5A5A, 12);
		g.circle(sw.x + 12 + on * 20, sw.cy(), 8, eqOn ? 0x131313 : col::soft);
		hot(sw, Hit::EqToggle);

		bool anySet = eqPreamp != 0;
		for (float gv : eqGains) if (gv != 0) anySet = true;
		float leftOf = sw.x - 12;
		if (anySet)
		{
			float rw = g.textWidth(L"Reset", 13, Font::Ui) + 24;
			Rect rb{ sw.x - 12 - rw, y + 15, rw, 32 };
			leftOf = rb.x - 8;
			bool rh = hovered(Hit::EqReset);
			if (rh) g.fill(rb, col::ctlHover, 6);
			g.stroke(rb, rh ? 0x4A4A4A : col::chip, 6);
			g.text(L"Reset", rb, 13, Font::Ui, rh ? col::white : col::soft, Align::Center);
			hot(rb, Hit::EqReset);
		}
		{
			// preset picker: shows the preset the sliders match, or Custom
			int p = eqPresetNow();
			std::wstring label = p >= 0 ? eqPresets[p].name : L"Custom";
			float pw = std::max(g.textWidth(label, 13, Font::Ui) + 50, 140.0f);
			Rect pb{ leftOf - pw, y + 13, pw, 34 };
			float h = anim(Hit::EqPresetMenu, -1, hovered(Hit::EqPresetMenu));
			g.fill(pb, 0x202020, 6);
			if (h > 0.01f) g.fill(pb, col::ctlHover, 6, h);
			g.stroke(pb, h > 0.5f ? 0x4A4A4A : col::chip, 6);
			g.text(label, { pb.x + 14, pb.y, pb.w - 44, pb.h }, 13, Font::Ui, col::text);
			g.icon("M6 9l6 6 6-6", { pb.right() - 28, pb.cy() - 8, 16, 16 }, col::soft, false, 1.8f);
			hot(pb, Hit::EqPresetMenu);
			eqPresetButton = pb;
		}
		y += 60;

		// sliders: Pre | 31 62 125 ... 16k, +12 at the top, -12 at the bottom
		float areaH = 210;
		float colW = std::min(56.0f, (w - 50) / 11.5f);
		float x0 = x + 34;
		float top = y + 30, bottom = y + areaH - 34;
		uint32_t lineCol = 0x262626;
		for (float db : { 12.0f, 0.0f, -12.0f })
		{
			float ly = top + (12 - db) / 24 * (bottom - top);
			g.text(db > 0 ? L"+12" : db < 0 ? L"−12" : L"0", { x, ly - 8, 28, 16 }, 11, Font::Mono, col::muted, Align::Right);
			g.line(x0, ly, x0 + colW * 11.5f, ly, db == 0 ? 0x333333 : lineCol, 1);
		}
		const wchar_t* labels[11] = { L"Pre", L"31", L"62", L"125", L"250", L"500", L"1k", L"2k", L"4k", L"8k", L"16k" };
		for (int i = 0; i < 11; ++i)
		{
			float cx = x0 + colW * (i + 0.5f) + (i > 0 ? colW * 0.5f : 0);   // a gap after the preamp
			float v = i == 0 ? eqPreamp : eqGains[i - 1];
			bool hv = hovered(Hit::EqSlider, i) || (dragging && pressHit == Hit::EqSlider && pressIndex == i);
			Rect track{ cx - 2, top, 4, bottom - top };
			eqTracks[i] = track;
			g.fill(track, 0x2A2A2A, 2);
			float zy = top + (bottom - top) / 2;
			float vy = top + (12 - v) / 24 * (bottom - top);
			uint32_t fillCol = eqOn ? col::text : col::dim;
			if (v != 0) g.fill({ cx - 2, std::min(zy, vy), 4, std::fabs(vy - zy) }, fillCol, 2);
			g.circle(cx, vy, hv ? 8.0f : 7.0f, eqOn ? col::white : col::soft);
			wchar_t num[16];
			if (v == 0) wcscpy_s(num, L"0");
			else swprintf(num, 16, v == std::floor(v) ? L"%+.0f" : L"%+.1f", v);
			g.text(num, { cx - colW / 2, y + 4, colW, 18 }, 11, Font::Mono, hv ? col::text : col::dim, Align::Center);
			g.text(labels[i], { cx - colW / 2, bottom + 12, colW, 18 }, 12, Font::Ui, hv ? col::text : col::dim, Align::Center);
			hot({ cx - colW / 2, top - 12, colW, bottom - top + 24 }, Hit::EqSlider, i);
		}
		g.line(x0 + colW * 1.25f, top - 6, x0 + colW * 1.25f, bottom + 6, lineCol, 1);
		y += areaH;
		g.line(x, y - 0.5f, x + w, y - 0.5f, col::line, 1);
		y += 30;
	}

	section(L"Library");
	{
		// small outlined buttons, right-aligned, drawn right to left
		auto smallButton = [&](float& bx, const wchar_t* label, Hit id, int index) {
			float bw = g.textWidth(label, 13, Font::Ui) + 24;
			Rect b{ bx - bw, y + 14, bw, 32 };
			if (hovered(id, index)) g.fill(b, col::hover, 3);
			g.stroke(b, hovered(id, index) ? 0x5A5A5A : col::chip, 3);
			g.text(label, b, 13, Font::Ui, col::soft, Align::Center);
			hotVisible(b, id, index);
			bx = b.x - 8;
		};
		auto folderRow = [&](const std::wstring& title, const std::wstring& path, const std::wstring& note) {
			g.line(x, y + 0.5f, x + w, y + 0.5f, col::line, 1);
			g.text(title, { x, y + 11, w - 260, 20 }, 14, Font::Ui, col::text);
			std::wstring sub = note.empty() ? path : path + L"  \x00B7  " + note;
			g.text(sub, { x, y + 31, w - 260, 18 }, 12, Font::Ui, col::dim);
		};
		std::wstring note = moving ? L"moving\x2026 " + std::to_wstring(moveDone) + L" of " + std::to_wstring(moveTotal) + L" files" : L"";
		folderRow(L"LossCat folder", libraryDir.empty() ? L"Not found" : libraryDir, note);
		float bx = x + w;
		if (!moving) smallButton(bx, L"Change\x2026", Hit::ChangeLibraryFolder, -1);
		smallButton(bx, L"Open", Hit::OpenLibraryFolder, -1);
		y += 60;
		for (int i = 0; i < (int)musicFolders.size(); ++i)
		{
			const std::wstring& f = musicFolders[i];
			std::wstring name = f.substr(f.find_last_of(L'\\') + 1);
			bool there = GetFileAttributesW(f.c_str()) != INVALID_FILE_ATTRIBUTES;
			folderRow(name.empty() ? f : name, f, there ? L"" : L"not found right now");
			bx = x + w;
			smallButton(bx, L"Remove", Hit::RemoveMusicFolder, i);
			if (there) smallButton(bx, L"Open", Hit::OpenMusicFolder, i);
			y += 60;
		}
		g.line(x, y + 0.5f, x + w, y + 0.5f, col::line, 1);
		std::wstring status;
		if (scanning) status = L"Scanning\x2026 " + std::to_wstring(scanCount) + L" files looked at";
		else
		{
			wchar_t n[64];
			swprintf(n, 64, libTracks.size() == 1 ? L"%zu song in your library" : L"%zu songs in your library", libTracks.size());
			status = n;
			unsigned long long bytes = 0, secs = 0;
			for (const Track& t : libTracks) { bytes += t.bytes; secs += t.info.durationMs / 1000; }
			if (bytes) status += L" \x00B7 " + fmtSize(bytes);
			if (secs >= 3600) status += L" \x00B7 " + std::to_wstring(secs / 3600) + L" h " + std::to_wstring(secs / 60 % 60) + L" min";
			else if (secs) status += L" \x00B7 " + std::to_wstring(secs / 60) + L" min";
		}
		g.text(L"Your library", { x, y + 11, w - 200, 20 }, 14, Font::Ui, col::text);
		g.text(status, { x, y + 31, w - 200, 18 }, 12, Font::Ui, col::dim);
		bx = x + w;
		smallButton(bx, L"Add folder\x2026", Hit::AddMusicFolder, -1);
		if (!scanning) smallButton(bx, L"Rescan", Hit::Rescan, -1);
		y += 60;
		if (!hiddenFiles.empty())
		{
			g.line(x, y + 0.5f, x + w, y + 0.5f, col::line, 1);
			std::wstring h = hiddenFiles.size() == 1 ? L"1 hidden song" : std::to_wstring(hiddenFiles.size()) + L" hidden songs";
			g.text(h, { x, y + 11, w - 200, 20 }, 14, Font::Ui, col::text);
			g.text(L"Removed from LossCat but still in your folders", { x, y + 31, w - 200, 18 }, 12, Font::Ui, col::dim);
			bx = x + w;
			smallButton(bx, L"Show again", Hit::ShowHidden, -1);
			y += 60;
		}
		g.line(x, y - 0.5f, x + w, y - 0.5f, col::line, 1);
		y += 30;
	}

	links.clear();
	auto link = [&](const wchar_t* label, const wchar_t* url, float lx, float ly) {
		float tw = g.textWidth(label, 13, Font::Ui);
		Rect r{ lx - tw, ly, tw, 20 };
		int i = (int)links.size();
		links.push_back(url);
		bool hov = hovered(Hit::Link, i);
		g.text(label, r, 13, Font::Ui, hov ? col::white : col::soft, Align::Right);
		g.line(r.x, r.bottom() - 2.5f, r.right(), r.bottom() - 2.5f, hov ? col::white : 0x5A5A5A, 1);
		hotVisible(r.inset(-4), Hit::Link, i);
	};

	section(L"About");
	g.line(x, y + 0.5f, x + w, y + 0.5f, col::line, 1);
	{
		Rect logo{ x, y + 18, 24, 24 };
		g.icon(icons::cat, logo, col::text, true);
		g.icon(icons::catTail, logo, col::text, false, 1.8f);
		g.text(L"LossCat", { x + 36, y + 11, 300, 20 }, 14, Font::UiMedium, col::text);
		g.text(L"Version 0.2.3, \u00A9 2026 SegCat", { x + 36, y + 31, 400, 18 }, 12, Font::Ui, col::dim);
		link(L"GPLv3 License", L"license:201|LossCat", x + w, y + 20);
		y += 60;
	}
	g.line(x, y - 0.5f, x + w, y - 0.5f, col::line, 1);
	y += 30;

	section(L"Credits");
	struct Credit { const wchar_t* name; const wchar_t* what; const wchar_t* license; const wchar_t* url; const wchar_t* text; };
	const Credit credits[] = {
		{ L"miniaudio", L"Audio decoding and playback, by David Reid", L"Public domain or MIT No Attribution", L"https://github.com/mackron/miniaudio", L"license:202|miniaudio" },
		{ L"TagLib", L"Tags, audio properties and cover art", L"LGPL 2.1 or MPL 1.1", L"https://github.com/taglib/taglib", L"license:203|TagLib" },
		{ L"zlib", L"Compression, used by TagLib", L"zlib License", L"https://zlib.net", L"license:204|zlib" },
		{ L"UTF8-CPP", L"Text encoding, used by TagLib", L"Boost Software License 1.0", L"https://github.com/nemtrif/utfcpp", L"license:205|UTF8-CPP" },
		{ L"ASIO SDK", L"ASIO output, by Steinberg. ASIO is a trademark of Steinberg Media Technologies GmbH", L"GPLv3", L"https://www.steinberg.net/developers/", L"license:206|ASIO SDK" },
	};
	for (const Credit& c : credits)
	{
		g.line(x, y + 0.5f, x + w, y + 0.5f, col::line, 1);
		g.text(c.name, { x, y + 11, w * 0.6f, 20 }, 14, Font::Ui, col::text);
		g.text(c.what, { x, y + 31, w * 0.6f, 18 }, 12, Font::Ui, col::dim);
		g.text(c.license, { x + w * 0.4f, y + 11, w * 0.6f, 20 }, 13, Font::Ui, col::dim, Align::Right);
		float lw = g.textWidth(L"License", 13, Font::Ui);
		link(L"License", c.text, x + w, y + 32);
		link(L"Source", c.url, x + w - lw - 16, y + 32);
		y += 64;
	}
	g.line(x, y - 0.5f, x + w, y - 0.5f, col::line, 1);
	pageMaxScroll = (y + pageScroll) - a.bottom() + 24;
	if (pageMaxScroll < 0) pageMaxScroll = 0;
}

void App::drawLicense(const Rect& a)
{
	listArea = {};
	pageArea = a;
	float x = a.x + 6, w = a.w - 12;
	if (w > 880) w = 880;
	float y = a.y - pageScroll;

	float bw = g.textWidth(L"Settings", 13, Font::UiMedium) + 40;
	Rect back{ x, y, bw, 30 };
	if (hovered(Hit::Back)) g.fill(back, col::hover, 15);
	g.stroke(back, col::chip, 15);
	g.icon("M15 6l-6 6 6 6", { back.x + 10, back.cy() - 8, 16, 16 }, col::text, false, 1.8f);
	g.text(L"Settings", { back.x + 30, back.y, bw - 34, 30 }, 13, Font::UiMedium, col::text);
	hotVisible(back, Hit::Back, -1);
	y += 30 + 20;
	g.text(licenseTitle + L" license", { x, y, w, 30 }, 22, Font::UiMedium, col::text);
	y += 46;
	float h = g.textBlock(licenseText.empty() ? L"License text not found." : licenseText,
		{ x, y, w, 0 }, 12, Font::Mono, col::soft, 4000, 18);
	y += h + 24;
	pageMaxScroll = (y + pageScroll) - a.bottom();
	if (pageMaxScroll < 0) pageMaxScroll = 0;
}

// ---------------------------------------------------------------- queue drawer

void App::drawQueuePanel()
{
	// Same kind of panel as the sidebar: it opens as a third column and the main panel makes room.
	const Rect& q = queuePanel;
	if (q.w < 1) return;
	g.fill(q, col::side, 8);
	g.pushClip(q);

	const float fullW = 300;          // lay out at full width so nothing reflows while it slides
	float x = q.x + 12, w = fullW - 24;
	float y = q.y + 22;

	// Title on the left; the gear and window dots sit on the right of this row (drawn later).
	g.text(L"Queue", { x + 9, y, w - 140, 32 }, 14, Font::UiSemibold, col::text);
	hot({ q.x, q.y + 20, q.w, q.h - 20 }, Hit::Panel);   // below the drag strip, the panel isn't a drag area
	y += 32 + 18;

	// Everything under the title scrolls.
	queueList = { q.x, y, q.w, q.bottom() - y - 8 };
	if (queueScroll > queueMaxScroll) queueScroll = queueMaxScroll;
	float top = y;
	y -= queueScroll;
	g.pushClip(queueList);
	auto visible = [&](const Rect& r) {
		Rect v{ r.x, std::max(r.y, queueList.y), r.w, std::min(r.bottom(), queueList.bottom()) - std::max(r.y, queueList.y) };
		return v;
	};

	auto header = [&](const wchar_t* s) {
		g.text(s, { x + 12, y, w - 24, 24 }, 12, Font::Ui, col::muted);
		y += 30;
	};
	int upPos = 0;
	auto row = [&](int i, bool isCurrent) {
		const Track& t = queue[i];
		Rect r{ x, y, w, 48 };
		bool onScreen = r.bottom() > queueList.y && r.y < queueList.bottom();
		bool held = qDragging && qDragItem == i;
		bool hov = !qDragging && hovered(Hit::QueueRow, i);
		bool hovX = !qDragging && hovered(Hit::QueueRemove, i);
		if (onScreen)
		{
			if (isCurrent || held) g.fill(r, col::selected, 3);
			else if (hov || hovX) g.fill(r, col::hover, 3);
			Rect cvr{ x + 6, y + 6, 36, 36 };
			if (ID2D1Bitmap* b = coverOf(t.path)) g.image(b, cvr, 2);
			else g.fill(cvr, col::tile, 2);
			if (!isCurrent && (hov || hovX || held))
			{
				// grip over the cover: this row can be dragged
				g.fill(cvr, 0x000000, 2, 0.55f);
				for (int gy = 0; gy < 3; ++gy)
					for (int gx = 0; gx < 2; ++gx)
						g.circle(cvr.cx() - 3 + gx * 6, cvr.cy() - 6 + gy * 6, 1.4f, col::white);
			}
			float tx = x + 6 + 36 + 12, tw = w - (tx - x) - 52;
			uint32_t tc = held ? col::muted : isCurrent ? col::white : col::text;
			g.text(t.title, { tx, y + 7, tw, 18 }, 13, isCurrent ? Font::UiMedium : Font::Ui, tc);
			g.text(t.artist, { tx, y + 25, tw, 16 }, 12, Font::Ui, held ? col::muted : col::dim);
			if (!isCurrent && (hov || hovX))
			{
				Rect xb{ x + w - 38, y + 10, 28, 28 };
				if (hovX) g.fill(xb, col::ctlHover, 14);
				g.icon(icons::close, { xb.cx() - 7, xb.cy() - 7, 14, 14 }, hovX ? col::white : col::soft, false, 1.8f);
			}
			else g.text(fmtTime(t.info.durationMs / 1000.0), { x + w - 52, y, 42, 48 }, 12, Font::Mono, col::dim, Align::Right);
			hot(visible(r), Hit::QueueRow, i);
			if (!isCurrent) hot(visible({ x + w - 38, y + 10, 28, 28 }), Hit::QueueRemove, i);
		}
		if (!isCurrent) qRows.push_back({ i, upPos++, 1, r });
		y += 48 + 2;
	};

	if (current < 0)
	{
		g.text(L"The queue is empty.", { x + 12, y, w - 24, 20 }, 13, Font::Ui, col::muted);
		g.text(L"Right-click a song and pick Play next or Add to queue.", { x + 12, y + 22, w - 24, 40 }, 12, Font::Ui, col::dim);
	}
	else
	{
		header(L"Now playing");
		row(current, true);
		y += 16;
		std::vector<int> up = upcoming();
		{
			// "Up next" with Clear on the right
			g.text(L"Up next", { x + 12, y, w - 24, 24 }, 12, Font::Ui, col::muted);
			if (!up.empty())
			{
				float cw = g.textWidth(L"Clear", 12, Font::Ui) + 16;
				Rect cb{ x + w - cw - 4, y - 2, cw, 26 };
				bool ch = hovered(Hit::QueueClear);
				if (ch) g.fill(cb, col::ctlHover, 13);
				g.text(L"Clear", cb, 12, Font::Ui, ch ? col::white : col::dim, Align::Center);
				if (cb.bottom() > queueList.y) hot(visible(cb), Hit::QueueClear, -1);
			}
			y += 30;
		}
		if (up.empty())
			g.text(L"Nothing queued", { x + 12, y, w - 24, 20 }, 13, Font::Ui, col::muted);
		for (int i : up) row(i, false);
		// where a dragged song will land
		if (qDragging && qDragList == 1 && qDropSlot >= 0)
		{
			float ly = -1;
			for (const QRow& r : qRows) if (r.list == 1 && r.up == qDropSlot) ly = r.r.y - 1;
			if (ly < 0 && !qRows.empty()) ly = qRows.back().r.bottom() + 1;
			if (ly >= 0) { g.fill({ x + 4, ly - 1, w - 8, 2 }, col::text, 1); g.circle(x + 4, ly, 3.5f, col::text); }
		}
	}
	queueMaxScroll = std::max(0.0f, (y + queueScroll) - top + 12 - queueList.h);
	g.popClip();
	g.popClip();
}


void App::drawToast()
{
	if (toastText.empty()) return;
	float tw = std::min(g.textWidth(toastText, 13, Font::Ui), W * 0.5f);
	float uw = g.textWidth(L"Undo", 13, Font::UiMedium) + 24;
	float w = 18 + tw + 16 + uw + 8;
	Rect r{ (W - w) / 2, playerBar.y - 48, w, 40 };
	g.fill({ r.x, r.y + 2, r.w, r.h }, 0x000000, 8, 0.35f);
	g.fill(r, 0x262626, 8);
	g.stroke(r, 0x333333, 8);
	g.text(toastText, { r.x + 18, r.y, tw + 2, r.h }, 13, Font::Ui, col::text);
	Rect ub{ r.right() - uw - 6, r.y + 6, uw, 28 };
	bool uh = hovered(Hit::QueueUndo);
	g.fill(ub, uh ? 0x3A3A3A : 0x303030, 14);
	g.text(L"Undo", ub, 13, Font::UiMedium, col::white, Align::Center);
	hot(r, Hit::Panel);
	hot(ub, Hit::QueueUndo);
}

void App::drawDragGhost()
{
	if (!qDragging || qDragItem < 0 || qDragItem >= (int)queue.size()) return;
	const Track& t = queue[qDragItem];
	float tw = std::min(g.textWidth(t.title, 13, Font::UiMedium), 220.0f);
	Rect r{ mouseX + 14, mouseY - 16, tw + 54, 32 };
	g.fill({ r.x, r.y + 2, r.w, r.h }, 0x000000, 6, 0.35f);
	g.fill(r, 0x2A2A2A, 6);
	g.stroke(r, 0x3A3A3A, 6);
	if (ID2D1Bitmap* b = coverOf(t.path)) g.image(b, { r.x + 5, r.y + 5, 22, 22 }, 2);
	else g.fill({ r.x + 5, r.y + 5, 22, 22 }, col::tile, 2);
	g.text(t.title, { r.x + 36, r.y, tw + 2, r.h }, 13, Font::UiMedium, col::text);
}

void App::drawMenu()
{
	auto itemH = [](const MenuEntry& m) { return m.look == Look::Separator ? 9.0f : m.look == Look::Heading ? 28.0f : m.look == Look::Action ? 32.0f + 9.0f : 32.0f; };
	float w = 0, h = 8;
	for (const MenuEntry& m : menu)
	{
		float tw = g.textWidth(m.label, 13, m.look == Look::Action ? Font::UiMedium : Font::Ui) + (m.look == Look::Action ? 26 : 0);
		if (tw > w) w = tw;
		h += itemH(m);
	}
	w += 24 + 8 + 24;
	if (w < 200) w = 200;
	float x = menuLeft || menuX + w > W - 8 ? menuX - w : menuX;
	float y = menuUp || menuY + h > H - 8 ? menuY - h : menuY;
	Rect r{ x, y, w, h };
	g.fill({ r.x, r.y + 2, r.w, r.h }, 0x000000, 6, 0.35f);
	g.fill(r, 0x222222, 6);
	g.stroke(r, 0x333333, 6);
	float iy = r.y + 4;
	for (int i = 0; i < (int)menu.size(); ++i)
	{
		const MenuEntry& m = menu[i];
		if (m.look == Look::Separator)
		{
			g.line(r.x + 8, iy + 4.5f, r.right() - 8, iy + 4.5f, 0x333333, 1);
			iy += 9;
			continue;
		}
		if (m.look == Look::Heading)
		{
			g.text(m.label, { r.x + 16, iy + 4, r.w - 32, 24 }, 12, Font::Ui, col::muted);
			iy += 28;
			continue;
		}
		Rect it{ r.x + 4, iy, r.w - 8, 32 };
		if (hovered(Hit::MenuItem, i)) g.fill(it, m.danger ? 0x3A2422 : 0x2E2E2E, 4);
		if (m.look == Look::Action)
		{
			g.icon(icons::plus, { it.x + 12, it.cy() - 8, 16, 16 }, col::white, false, 1.8f);
			g.text(m.label, { it.x + 38, it.y, it.w - 50, it.h }, 13, Font::UiMedium, col::white);
			hot(it, Hit::MenuItem, i);
			g.line(r.x + 1, iy + 32 + 4.5f, r.right() - 1, iy + 32 + 4.5f, 0x333333, 1);
			iy += 32 + 9;
			continue;
		}
		g.text(m.label, { it.x + 12, it.y, it.w - 40, it.h }, 13, Font::Ui, m.danger ? 0xE5584F : col::text);
		if (m.checked) g.icon(icons::check, { it.right() - 26, it.cy() - 8, 16, 16 }, col::white, false, 1.8f);
		hot(it, Hit::MenuItem, i);
		iy += 32;
	}
}

void App::drawConfirm()
{
	hot({ 0, 0, W, H }, Hit::Panel);
	g.fill({ 0, 0, W, H }, 0x000000, 0, 0.5f);
	float w = 420, h = 170;
	Rect d{ W / 2 - w / 2, H / 2 - h / 2 - 40, w, h };
	g.fill(d, 0x1E1E1E, 8);
	g.stroke(d, 0x333333, 8);
	g.text(confirmTitle, { d.x + 24, d.y + 22, w - 48, 22 }, 15, Font::UiMedium, col::text);
	bool deleting = pendingDeletePlaylist > 0 || pendingRemoveLyrics;
	int imported = 0;
	for (const auto& p : pendingRemove) if (isImported(p)) ++imported;
	bool onlyHide = !deleting && imported == 0, mixed = !deleting && imported > 0 && imported < (int)pendingRemove.size();
	g.textBlock(!confirmBody.empty() ? confirmBody
		: pendingRemoveLyrics ? L"They're deleted from the file's tags. The song itself isn't changed."
		: deleting ? L"The songs stay in your library. Only the playlist goes."
		: onlyHide ? L"The files stay in your folder; LossCat just stops showing them. You can show them again in Settings."
		: mixed ? L"Copies in the LossCat folder go to the Recycle Bin. Songs from your own folders are only hidden."
		: L"They come out of your LossCat library. You can restore them from the Recycle Bin.",
		{ d.x + 24, d.y + 52, w - 48, 60 }, 13, Font::Ui, col::dim, 3, 20);
	float bh = 32, by = d.bottom() - 20 - bh;
	bool moveFolder = !pendingLibraryDir.empty();
	const wchar_t* okLabel = moveFolder ? L"Move" : pendingRemoveLyrics ? L"Remove lyrics" : deleting ? L"Delete playlist" : onlyHide ? L"Hide" : L"Move to Recycle Bin";
	float okW = g.textWidth(okLabel, 13, Font::UiMedium) + 28;
	Rect ok{ d.right() - 20 - okW, by, okW, bh };
	if (moveFolder) g.fill(ok, hovered(Hit::ConfirmOk) ? 0xFFFFFF : col::text, 3);   // not destructive, so not red
	else g.fill(ok, hovered(Hit::ConfirmOk) ? 0xD9483F : 0xC0403A, 3);
	g.text(okLabel, ok, 13, Font::UiMedium, moveFolder ? 0x131313 : col::white, Align::Center);
	hot(ok, Hit::ConfirmOk);
	float cW = g.textWidth(L"Cancel", 13, Font::UiMedium) + 28;
	Rect cancel{ ok.x - 8 - cW, by, cW, bh };
	if (hovered(Hit::ConfirmCancel)) g.fill(cancel, col::hover, 3);
	g.stroke(cancel, col::chip, 3);
	g.text(L"Cancel", cancel, 13, Font::UiMedium, col::soft, Align::Center);
	hot(cancel, Hit::ConfirmCancel);
}

// ---------------------------------------------------------------- player bar

void App::drawPlayerBar()
{
	// A floating island. Left: the cover filling the island's end, title, artist, format.
	// Middle: previous / play / next, then the seek bar in the same line.
	// Right: shuffle, repeat | queue, output device, volume.
	const Rect& b = playerBar;
	Rect card{ 8, b.y + 8, W - 16, b.h - 16 };
	g.fill(card, col::panel, 10);
	hot(card, Hit::Panel);
	float cy = card.cy();
	bool showRight = W >= 900;
	bool hasTrack = current >= 0;
	State st = player.getState();

	Rect cv{ card.x + 1, card.y + 1, card.h - 2, card.h - 2 };
	if (ID2D1Bitmap* bmp = currentCover()) g.image(bmp, cv, 9, true);
	else
	{
		uint32_t c = hasTrack ? col::tile : 0x222222;   // same shape as a cover: left corners round
		g.fill(cv, c, 9);
		g.fill({ cv.cx(), cv.y, cv.w / 2, cv.h }, c, 0);
	}
	g.stroke(card, col::line, 10);
	hot(cv, Hit::Cover);
	float leftEnd = card.x + (W >= 1100 ? 300.0f : W >= 900 ? 250.0f : 210.0f);
	float tx = cv.right() + 14, tw = leftEnd - tx;
	if (hasTrack)
	{
		const Track& t = queue[current];
		bool canFav = inLibrary(t.path);
		float titleW = g.textWidth(t.title, 15, Font::UiMedium);
		float maxTitle = tw - (canFav ? 30 : 0);
		if (titleW > maxTitle) titleW = maxTitle;
		g.text(t.title, { tx, cy - 28, titleW + 2, 20 }, 15, Font::UiMedium, col::text);
		if (canFav)
		{
			bool fav = inPlaylist(0, t.path);
			Rect heart{ tx + titleW + 6, cy - 30, 24, 24 };
			if (hovered(Hit::BarFavourite)) g.fill(heart, col::ctlHover, 12);
			g.icon(icons::heart, { heart.cx() - 7, heart.cy() - 7, 14, 14 }, fav ? col::text : col::soft, fav, 1.6f);
			hot(heart, Hit::BarFavourite);
		}
		g.text(t.artist, { tx, cy - 8, tw, 16 }, 12, Font::Ui, col::dim);

		// the signal path in one line: the file's format, then what the device gets
		// Quiet plain text: "FLAC · 24-bit · 44.1 kHz", the rate showing "96 → 48 kHz" when resampled,
		// and a small dot when the device gets the file untouched.
		float py = cy + 13;
		const TrackInfo& fi = t.info;
		std::wstring line = codecOf(t.path);
		if (fi.bitsPerSample) line += L" \x00B7 " + std::to_wstring(fi.bitsPerSample) + L"-bit";
		int outRate = (int)player.outputSampleRate();
		if (fi.sampleRate)
			line += L" \x00B7 " + (outRate && outRate != fi.sampleRate ? fmtRate(fi.sampleRate) + L" \x2192 " + fmtRate(outRate) : fmtRate(fi.sampleRate)) + L" kHz";
		bool perfect = player.bitPerfect(fi.bitsPerSample);
		bool sh = hovered(Hit::SignalPath);
		float lw = g.textWidth(line, 11, Font::Ui);
		g.text(line, { tx, py, tw, 15 }, 11, Font::Ui, sh ? col::soft : col::dim);
		float used = lw;
		if (perfect && lw + 16 < tw)
		{
			g.circle(tx + lw + 8, py + 8, 2.5f, col::soft);
			used += 12;
		}
		hot({ tx, py, std::min(tw, used), 15 }, Hit::SignalPath);
	}
	else
	{
		g.text(error.empty() ? L"Nothing playing" : error, { tx, cy - 18, tw, 18 }, 14, Font::UiMedium, col::muted);
		g.text(L"Drop files on the window to start", { tx, cy + 2, tw, 16 }, 12, Font::Ui, col::muted);
	}


	// ---- right cluster, laid out from the right edge inward
	float x = card.right() - 16;
	auto ctlAt = [&](float rx, Hit id, const char* icon, bool filled, bool active) {
		Rect r{ rx, cy - 18, 36, 36 };
		float h = anim(id, -1, hovered(id)), p = anim(id, -2, pressed(id));
		float on = anim(id, -3, active);
		if (on > 0.01f) g.fill(r, col::ctlHover, 18, on);
		if (h > 0.01f) g.fill(r, on > 0.5f ? 0x333333 : col::ctlHover, 18, h * (on > 0.5f ? 0.6f : 1.0f));
		uint32_t c = active ? col::white : hovered(id) ? col::text : col::soft;
		float sz = 16 * (1 - 0.12f * p);
		g.icon(icon, { r.cx() - sz / 2, r.cy() - sz / 2, sz, sz }, c, filled, 1.6f);
		hot(r, id);
		return r;
	};
	if (showRight)
	{
		volumeBar = { x - 80, cy - 2, 80, 4 };
		g.fill(volumeBar, col::track, 2);
		g.fill({ volumeBar.x, volumeBar.y, volumeBar.w * volume, 4 }, col::text, 2);
		if (hovered(Hit::Volume) || (dragging && pressHit == Hit::Volume))
			g.circle(volumeBar.x + volumeBar.w * volume, cy, 6, col::white);
		hot({ volumeBar.x - 6, cy - 12, volumeBar.w + 12, 24 }, Hit::Volume);
		x = volumeBar.x - 8;
		g.icon(volume <= 0.001f ? icons::mute : icons::volume, { x - 18, cy - 9, 18, 18 }, col::soft, false, 1.6f);
		x -= 18 + 8;

		x -= 36;
		barDeviceButton = ctlAt(x, Hit::BarDevice, icons::speaker, false, false);
		x -= 2 + 36;
		ctlAt(x, Hit::QueueButton, icons::queue, false, queueOpen);
		x -= 9;
		g.line(x + 0.5f, cy - 12, x + 0.5f, cy + 12, col::line, 1);
		x -= 8;
	}
	x -= 36;
	{
		Rect r = ctlAt(x, Hit::Repeat, icons::repeat, false, repeat != 0);
		if (repeat == 2) g.text(L"1", { r.x + 23, r.y + 4, 8, 10 }, 9, Font::Mono, col::white);
	}
	x -= 2 + 36;
	ctlAt(x, Hit::Shuffle, icons::shuffle, false, shuffle);
	float rightStart = x - 24;

	// ---- middle: previous, play, next, then time | seek bar | time
	float mx = leftEnd + 24;
	ctlAt(mx, Hit::Prev, icons::prev, true, false);
	mx += 36 + 6;
	{
		Rect r{ mx, cy - 22, 44, 44 };
		float h = anim(Hit::Play, -1, hovered(Hit::Play)), p = anim(Hit::Play, -2, pressed(Hit::Play));
		float rad = 22 * (1 + 0.05f * h - 0.08f * p);
		g.circle(r.cx(), r.cy(), rad, col::text);
		if (h > 0.01f) g.circle(r.cx(), r.cy(), rad, col::white, true, 1, h);
		float isz = 17 * (1 - 0.08f * p);
		g.icon(st == State::Playing ? icons::pause : icons::play, { r.cx() - isz / 2, r.cy() - isz / 2, isz, isz }, 0x131313, true);
		hot(r, Hit::Play);
		mx += 44 + 6;
	}
	ctlAt(mx, Hit::Next, icons::next, true, false);
	mx += 36 + 16;

	double dur = hasTrack ? player.duration() : 0;
	double pos = dragging && pressHit == Hit::Seek ? dragSeconds : hasTrack ? player.position() : 0;
	std::wstring a1 = fmtTime(pos), a2 = fmtTime(dur);
	float t1 = g.textWidth(a1, 11, Font::Mono), t2 = g.textWidth(a2, 11, Font::Mono);
	float seekW = rightStart - mx - t1 - t2 - 24;
	if (seekW > 60)
	{
		g.text(a1, { mx, cy - 9, t1 + 2, 18 }, 11, Font::Mono, col::dim);
		seekBar = { mx + t1 + 12, cy - 2, seekW, 4 };
		g.text(a2, { seekBar.right() + 12, cy - 9, t2 + 2, 18 }, 11, Font::Mono, col::dim);
		g.fill(seekBar, col::track, 2);
		float f = clamp01(dur > 0 ? (float)(pos / dur) : 0);
		if (f > 0) g.fill({ seekBar.x, seekBar.y, seekBar.w * f, 4 }, col::text, 2);
		if (hasTrack && (hovered(Hit::Seek) || (dragging && pressHit == Hit::Seek)))
			g.circle(seekBar.x + seekBar.w * f, cy, 6, col::white);
		hot({ seekBar.x - 6, cy - 12, seekBar.w + 12, 24 }, Hit::Seek);
	}
	else seekBar = {};
}
