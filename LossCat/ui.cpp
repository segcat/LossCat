// Layout and drawing. Numbers come straight from the design canvas (CSS px = DIPs).
#include "app.h"
#include <cmath>
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
	const char* home = "M3 11l9-7 9 7v9a1 1 0 0 1-1 1h-5v-6H9v6H4a1 1 0 0 1-1-1z";
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

// ---------------------------------------------------------------- animation state

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
	return (navStart > 0 && now - navStart < 700) || (queueStart > 0 && now - queueStart < 320);
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
	if (!g.begin()) return;
	hots.clear();
	g.clear(col::base);
	drawSidebar();
	drawMain();
	drawQueuePanel();
	drawWindowControls({ 8, 8, W - 16, H - 104 });   // always in the window's top-right corner
	drawPlayerBar();
	g.end();   // blocks until the next vsync, so this paces animation frames to the display

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
	float y = s.y + 22;

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
	y += 32 + 18;

	// Now playing / Home: these shrink away when collapsed (they move to the main panel).
	struct Item { Hit id; Page p; const char* icon; const wchar_t* label; };
	const Item items[] = {
		{ Hit::NavNow, Page::Now, icons::now, L"Now playing" },
		{ Hit::NavHome, Page::Home, icons::home, L"Home" },
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

	g.popClip();
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
		const Item items[] = { { Hit::NavNow, Page::Now, icons::now }, { Hit::NavHome, Page::Home, icons::home } };
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
	case Page::Now: drawNowPlaying(area); break;
	case Page::Settings: drawSettings(area); break;
	}
	g.popClip();

	g.popClip();
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

// Song table rows (Home): # | title | artist | album | format | time
void App::drawTrackRows(const Rect& a, int first, bool)
{
	float y = a.y;
	float wTitle = a.w * 0.34f, wArtist = a.w * 0.22f, wAlbum = a.w * 0.22f;
	float colX[] = { a.x + 12, a.x + 52, a.x + 52 + wTitle, a.x + 52 + wTitle + wArtist, a.right() - 12 - 56 - 16 - 110, a.right() - 12 - 56 };
	if (wAlbum + colX[3] > colX[4]) wAlbum = colX[4] - colX[3] - 16;
	// header
	g.text(L"#", { colX[0], y, 30, 30 }, 12, Font::Ui, col::muted);
	g.text(L"Title", { colX[1], y, wTitle - 16, 30 }, 12, Font::Ui, col::muted);
	g.text(L"Artist", { colX[2], y, wArtist - 16, 30 }, 12, Font::Ui, col::muted);
	g.text(L"Album", { colX[3], y, wAlbum - 16, 30 }, 12, Font::Ui, col::muted);
	g.text(L"Format", { colX[4], y, 110, 30 }, 12, Font::Ui, col::muted);
	g.text(L"Time", { colX[5], y, 56, 30 }, 12, Font::Ui, col::muted, Align::Right);
	y += 30;
	g.line(a.x, y - 0.5f, a.right(), y - 0.5f, col::line, 1);
	for (int i = first; i < (int)queue.size() && y + 30 <= a.bottom(); ++i)
	{
		const Track& t = queue[i];
		Rect r{ a.x, y, a.w, 30 };
		bool on = i == current;
		if (hovered(Hit::QueueRow, i)) g.fill(r, col::hover);
		uint32_t fg = on ? col::white : col::text;
		wchar_t num[16];
		swprintf(num, 16, L"%d", i + 1);
		g.text(num, { colX[0], y, 30, 30 }, 12, Font::Mono, on ? col::white : col::muted);
		g.text(t.title, { colX[1], y, wTitle - 16, 30 }, 13, on ? Font::UiMedium : Font::Ui, fg);
		g.text(t.artist, { colX[2], y, wArtist - 16, 30 }, 13, Font::Ui, col::soft);
		g.text(t.album, { colX[3], y, wAlbum - 16, 30 }, 13, Font::Ui, col::soft);
		std::wstring fmt = codecOf(t.path);
		if (t.info.sampleRate)
			fmt += L" " + (t.info.bitsPerSample ? std::to_wstring(t.info.bitsPerSample) + L"/" : L"") + fmtRate(t.info.sampleRate);
		g.text(fmt, { colX[4], y, 110, 30 }, 12, Font::Mono, col::dim);
		g.text(fmtTime(t.info.durationMs / 1000.0), { colX[5], y, 56, 30 }, 12, Font::Mono, col::dim, Align::Right);
		g.line(a.x, y + 29.5f, a.right(), y + 29.5f, 0x202020, 1);
		hot(r, Hit::QueueRow, i);
		y += 30;
	}
}

void App::drawHome(const Rect& a)
{
	if (queue.empty())
	{
		drawEmpty(a, L"Add your music", L"Drop audio files on this window, or open them.", true);
		return;
	}
	g.text(L"This session", { a.x + 6, a.y, a.w, 24 }, 13, Font::UiMedium, col::dim);
	drawTrackRows({ a.x, a.y + 36, a.w, a.h - 36 }, 0, false);
}

ID2D1Bitmap* App::currentCover()
{
	if (current < 0) return nullptr;
	if (coverFor != current || coverGen != g.generation())
	{
		cover = g.loadImage(queue[current].coverBytes);
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
	if (current < 0)
	{
		drawEmpty(a, L"Nothing playing", L"Drop audio files on this window, or open them, and they play here.", true);
		return;
	}
	const Track& tr = queue[current];
	const TrackInfo& info = tr.info;

	float coverW = (a.w - 36) * 0.4f;
	if (coverW > 340) coverW = 340;
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
		label(backend + L" shared", col::muted);
		box(signalOutput());
		label(L"→", col::dim);
		bool same = info.sampleRate == (int)player.outputSampleRate();
		if (same)
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
		y += 42 + 24;
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
	y += 36 + 6;

	Rect body{ x, y, w, a.bottom() - y };
	if (npTab == 0)
	{
		bool any = false;
		for (int i = current + 1; i < (int)queue.size() && y + 46 <= a.bottom(); ++i)
		{
			const Track& q = queue[i];
			Rect r{ x - 10, y, w + 10, 46 };
			if (hovered(Hit::QueueRow, i)) g.fill(r, col::hover, 2);
			g.fill({ x, y + 3, 40, 40 }, col::tile, 2);
			g.text(q.title, { x + 52, y + 6, w - 52 - 56, 18 }, 13, Font::UiMedium, col::text);
			g.text(q.artist, { x + 52, y + 24, w - 52 - 56, 16 }, 12, Font::Ui, col::soft);
			g.text(fmtTime(q.info.durationMs / 1000.0), { x + w - 54, y, 44, 46 }, 12, Font::Mono, col::soft, Align::Right);
			hot(r, Hit::QueueRow, i);
			y += 46;
			any = true;
		}
		if (!any) g.text(L"Nothing queued. Drop more files to add them.", { x, y + 4, w, 20 }, 13, Font::Ui, col::muted);
	}
	else if (npTab == 1)
	{
		std::wstring folder = tr.path.substr(0, tr.path.find_last_of(L"\\/"));
		std::wstring fmt = signalSource();
		struct Row { const wchar_t* k; std::wstring v; bool mono; };
		const Row rows[] = {
			{ L"Album", tr.album.empty() ? L"Unknown" : tr.album, false },
			{ L"Artist", tr.artist, false },
			{ L"Released", info.year ? std::to_wstring(info.year) : L"Unknown", false },
			{ L"Track", info.track ? std::to_wstring(info.track) : L"Unknown", false },
			{ L"Length", fmtTime(player.duration()), true },
			{ L"Format", fmt, true },
			{ L"Folder", folder, true },
		};
		for (const Row& r : rows)
		{
			if (y + 26 > a.bottom()) break;
			g.text(r.k, { x, y, 80, 22 }, 13, Font::Ui, col::muted);
			g.text(r.v, { x + 98, y, w - 98, 22 }, 13, r.mono ? Font::Mono : Font::Ui, col::text);
			y += 30;
		}
	}
	else
	{
		if (info.lyrics.empty())
			g.text(L"No lyrics in this file's tags.", { x, y + 4, w, 20 }, 14, Font::Ui, col::muted);
		else
			g.textBlock(widen(info.lyrics), { x, y + 4, w, body.h }, 14, Font::Ui, col::soft, (int)(body.h / 22), 22);
	}
}

void App::drawSettings(const Rect& a)
{
	float x = a.x + 6, w = a.w - 12;
	if (w > 880) w = 880;
	float y = a.y;
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

	wchar_t fmt[96];
	swprintf(fmt, 96, L"%u Hz, %u channels, %hs", player.deviceSampleRate(), player.deviceChannels(), player.deviceFormatName());
	section(L"Audio");
	row(L"Output device", widen(player.backendName()) + L", shared mode", audioOk ? widen(player.deviceName()) : L"No device", false, false);
	row(L"Device format", L"What Windows mixes at; files at other rates are resampled", fmt, false, true);

	section(L"About");
	row(L"LossCat", L"Built on miniaudio and TagLib", L"0.1", false, true);
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

	auto header = [&](const wchar_t* s) {
		g.text(s, { x + 12, y, w - 24, 24 }, 12, Font::Ui, col::muted);
		y += 30;
	};
	auto row = [&](int i, bool isCurrent) {
		const Track& t = queue[i];
		Rect r{ x, y, w, 48 };
		if (isCurrent) g.fill(r, col::selected, 3);
		else if (hovered(Hit::QueueRow, i)) g.fill(r, col::hover, 3);
		g.fill({ x + 6, y + 6, 36, 36 }, col::tile, 2);
		float tx = x + 6 + 36 + 12, tw = w - (tx - x) - 52;
		g.text(t.title, { tx, y + 7, tw, 18 }, 13, isCurrent ? Font::UiMedium : Font::Ui, isCurrent ? col::white : col::text);
		g.text(t.artist, { tx, y + 25, tw, 16 }, 12, Font::Ui, col::dim);
		g.text(fmtTime(t.info.durationMs / 1000.0), { x + w - 52, y, 42, 48 }, 12, Font::Mono, col::dim, Align::Right);
		hot(r, Hit::QueueRow, i);
		y += 48 + 2;
	};

	if (current < 0)
	{
		g.text(L"The queue is empty.", { x + 12, y, w - 24, 20 }, 13, Font::Ui, col::muted);
	}
	else
	{
		header(L"Now playing");
		row(current, true);
		y += 16;
		header(L"Up next");
		if (current + 1 >= (int)queue.size())
			g.text(L"Nothing queued", { x + 12, y, w - 24, 20 }, 13, Font::Ui, col::muted);
		for (int i = current + 1; i < (int)queue.size() && y + 48 <= q.bottom() - 12; ++i) row(i, false);
	}
	g.popClip();
}

// ---------------------------------------------------------------- player bar

void App::drawPlayerBar()
{
	const Rect& b = playerBar;
	float cy = b.cy();
	float left = 26, right = W - 20;
	bool showRight = W >= 900;
	float total = right - left - (showRight ? 56 : 28);
	float unit = total / (showRight ? 3.3f : 2.3f);
	Rect colL{ left, b.y, unit, b.h };
	Rect colC{ colL.right() + 28, b.y, unit * 1.3f, b.h };
	Rect colR{ colC.right() + 28, b.y, unit, b.h };
	bool hasTrack = current >= 0;
	State st = player.getState();

	// left: cover, title, artist, signal path
	Rect cv{ left, cy - 24, 48, 48 };
	if (ID2D1Bitmap* bmp = currentCover()) g.image(bmp, cv, 4);
	else g.fill(cv, hasTrack ? col::tile : 0x1A1A1A, 4);
	g.stroke(cv, col::track, 4, 1);
	hot(cv, Hit::Cover);
	float tx = cv.right() + 14, tw = colL.right() - tx;
	if (hasTrack)
	{
		const Track& t = queue[current];
		g.text(t.title, { tx, cy - 26, tw, 18 }, 14, Font::UiMedium, col::text);
		std::wstring sub = t.album.empty() ? t.artist : t.artist + L", " + t.album;
		g.text(sub, { tx, cy - 7, tw, 16 }, 12, Font::Ui, col::dim);
		// mini signal path
		float px = tx, py = cy + 10;
		auto box = [&](const std::wstring& s) {
			float w = g.textWidth(s, 11, Font::Mono) + 12;
			Rect r{ px, py, w, 18 };
			g.stroke(r, hovered(Hit::SignalPath) ? 0x5A5A5A : col::tile, 2);
			g.text(s, r, 11, Font::Mono, col::text, Align::Center);
			px += w + 6;
		};
		box(signalSource());
		g.text(L"→", { px, py, 12, 18 }, 11, Font::Mono, col::dim);
		px += 18;
		box(signalOutput());
		bool same = t.info.sampleRate == (int)player.outputSampleRate();
		if (same) g.circle(px + 4, py + 9, 3.5f, col::white);
		else g.circle(px + 4, py + 9, 3, col::dim, false, 1);
		g.text(same ? L"No resampling" : L"Resampled", { px + 14, py, 120, 18 }, 11, Font::Mono, same ? col::text : col::dim);
		hot({ tx, py, px + 110 - tx, 18 }, Hit::SignalPath);
	}
	else
	{
		g.text(error.empty() ? L"Nothing playing" : error, { tx, cy - 18, tw, 18 }, 14, Font::UiMedium, col::muted);
		g.text(L"Drop files on the window to start", { tx, cy + 2, tw, 16 }, 12, Font::Ui, col::muted);
	}

	// centre: controls
	float rowY = cy - 14;
	float ctlW = 36 * 4 + 42 + 14 * 4;
	float x = colC.cx() - ctlW / 2;
	auto ctl = [&](Hit id, const char* icon, bool filled, bool active, bool dot) {
		Rect r{ x, rowY - 18, 36, 36 };
		if (hovered(id)) g.fill(r, col::ctlHover, 3);
		uint32_t c = active ? col::white : hovered(id) ? col::text : col::soft;
		g.icon(icon, { r.cx() - 9, r.cy() - 9, 18, 18 }, c, filled, 1.6f);
		if (dot) g.circle(r.cx(), r.bottom() - 5, 2, col::white);
		hot(r, id);
		x += 36 + 14;
	};
	ctl(Hit::Shuffle, icons::shuffle, false, shuffle, shuffle);
	ctl(Hit::Prev, icons::prev, true, false, false);
	{
		Rect r{ x, rowY - 21, 42, 42 };
		g.circle(r.cx(), r.cy(), 21, hovered(Hit::Play) ? col::white : col::text);
		bool playing = st == State::Playing;
		g.icon(playing ? icons::pause : icons::play, { r.cx() - 8, r.cy() - 8, 16, 16 }, 0x131313, true);
		hot(r, Hit::Play);
		x += 42 + 14;
	}
	ctl(Hit::Next, icons::next, true, false, false);
	{
		float rx = x;
		ctl(Hit::Repeat, icons::repeat, false, repeat != 0, repeat != 0);
		if (repeat == 2) g.text(L"1", { rx + 24, rowY - 15, 8, 10 }, 9, Font::Mono, col::white);
	}

	// seek row
	float sy = cy + 24;
	float seekW = colC.w;
	if (seekW > 560) seekW = 560;
	float sx = colC.cx() - seekW / 2;
	double dur = hasTrack ? player.duration() : 0;
	double pos = dragging && pressHit == Hit::Seek ? dragSeconds : hasTrack ? player.position() : 0;
	g.text(fmtTime(pos), { sx, sy - 9, 34, 18 }, 11, Font::Mono, col::dim, Align::Right);
	g.text(fmtTime(dur), { sx + seekW - 34, sy - 9, 34, 18 }, 11, Font::Mono, col::dim);
	seekBar = { sx + 44, sy - 2, seekW - 88, 4 };
	g.fill(seekBar, col::track, 2);
	float f = dur > 0 ? (float)(pos / dur) : 0;
	f = clamp01(f);
	if (f > 0) g.fill({ seekBar.x, seekBar.y, seekBar.w * f, 4 }, col::text, 2);
	if (hasTrack) g.circle(seekBar.x + seekBar.w * f, sy, 6, col::white);
	hot({ seekBar.x - 6, sy - 10, seekBar.w + 12, 20 }, Hit::Seek);

	if (!showRight) return;

	// right: up next button, volume
	float rx = colR.right();
	volumeBar = { rx - 100, cy - 2, 100, 4 };
	g.fill(volumeBar, col::track, 2);
	g.fill({ volumeBar.x, volumeBar.y, volumeBar.w * volume, 4 }, col::text, 2);
	if (hovered(Hit::Volume) || (dragging && pressHit == Hit::Volume))
		g.circle(volumeBar.x + volumeBar.w * volume, cy, 6, col::white);
	g.icon(volume <= 0.001f ? icons::mute : icons::volume, { volumeBar.x - 10 - 18, cy - 9, 18, 18 }, col::soft, false, 1.6f);
	hot({ volumeBar.x - 6, cy - 12, volumeBar.w + 12, 24 }, Hit::Volume);

	float dx = volumeBar.x - 10 - 18 - 16;
	g.fill({ dx, cy - 18, 1, 36 }, 0x2B2B2B);
	if (W >= 1100)
	{
		std::wstring nextTitle = current >= 0 && current + 1 < (int)queue.size() ? queue[current + 1].title : L"Nothing queued";
		float textW = g.textWidth(nextTitle, 13, Font::Ui);
		if (textW > 160) textW = 160;
		if (textW < 50) textW = 50;
		float bw = 8 + textW + 14 + 18 + 8;
		Rect qb{ dx - 16 - bw, cy - 22, bw, 44 };
		if (queueOpen) g.fill(qb, 0x272727, 4);
		else if (hovered(Hit::QueueButton)) g.fill(qb, col::hover, 4);
		g.text(L"Up next", { qb.x + 8, cy - 17, textW, 14 }, 11, Font::Ui, col::dim);
		g.text(nextTitle, { qb.x + 8, cy - 1, textW, 18 }, 13, Font::Ui, col::text);
		g.icon(icons::queue, { qb.right() - 8 - 18, cy - 9, 18, 18 }, queueOpen ? col::white : col::soft, false, 1.6f);
		hot(qb, Hit::QueueButton);
		g.fill({ qb.x - 16, cy - 18, 1, 36 }, 0x2B2B2B);
	}
}
