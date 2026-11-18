#include "gfx.h"
#include <cmath>
#include <cstdlib>
#include <cctype>

#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")
#pragma comment(lib, "windowscodecs")

static D2D1_COLOR_F color(uint32_t rgb, float alpha)
{
	return D2D1::ColorF(((rgb >> 16) & 0xFF) / 255.0f, ((rgb >> 8) & 0xFF) / 255.0f, (rgb & 0xFF) / 255.0f, alpha);
}

static D2D1_RECT_F rectF(const Rect& r)
{
	return D2D1::RectF(r.x, r.y, r.x + r.w, r.y + r.h);
}

std::wstring widen(const std::string& s)
{
	if (s.empty()) return {};
	int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
	std::wstring w(n, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), n);
	return w;
}

std::string narrow(const std::wstring& w)
{
	if (w.empty()) return {};
	int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
	std::string s(n, '\0');
	WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), s.data(), n, nullptr, nullptr);
	return s;
}

static bool hasFamily(IDWriteFactory* dw, const wchar_t* name)
{
	ComPtr<IDWriteFontCollection> fonts;
	if (FAILED(dw->GetSystemFontCollection(&fonts))) return false;
	UINT32 index = 0;
	BOOL exists = FALSE;
	fonts->FindFamilyName(name, &index, &exists);
	return exists != FALSE;
}

bool Gfx::init(HWND h)
{
	hwnd = h;
	if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, factory.GetAddressOf()))) return false;
	if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
		reinterpret_cast<IUnknown**>(dwrite.GetAddressOf())))) return false;
	CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wic));

	D2D1_STROKE_STYLE_PROPERTIES sp = D2D1::StrokeStyleProperties(
		D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_LINE_JOIN_ROUND);
	factory->CreateStrokeStyle(sp, nullptr, 0, &roundStroke);

	uiFamily = hasFamily(dwrite.Get(), L"Segoe UI Variable Text") ? L"Segoe UI Variable Text" : L"Segoe UI";
	monoFamily = hasFamily(dwrite.Get(), L"Cascadia Mono") ? L"Cascadia Mono" : L"Consolas";

	dpi = (float)GetDpiForWindow(hwnd);
	return createTarget();
}

bool Gfx::createTarget()
{
	RECT rc;
	GetClientRect(hwnd, &rc);
	D2D1_SIZE_U size = D2D1::SizeU((UINT32)(rc.right - rc.left), (UINT32)(rc.bottom - rc.top));
	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties();
	props.dpiX = props.dpiY = dpi;
	if (FAILED(factory->CreateHwndRenderTarget(props, D2D1::HwndRenderTargetProperties(hwnd, size), &target)))
		return false;
	target->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
	target->CreateSolidColorBrush(color(0xFFFFFF, 1), &solid);
	++gen;
	return true;
}

void Gfx::setDpi(float d)
{
	dpi = d;
	if (target) target->SetDpi(dpi, dpi);
}

void Gfx::resize(UINT w, UINT h)
{
	if (target) target->Resize(D2D1::SizeU(w, h));
}

bool Gfx::begin()
{
	if (!target && !createTarget()) return false;
	target->BeginDraw();
	target->SetTransform(D2D1::Matrix3x2F::Identity());
	return true;
}

void Gfx::end()
{
	if (target->EndDraw() == (HRESULT)D2DERR_RECREATE_TARGET)
	{
		solid.Reset();
		target.Reset();
	}
}

ID2D1SolidColorBrush* Gfx::brush(uint32_t rgb, float alpha)
{
	solid->SetColor(color(rgb, alpha));
	return solid.Get();
}

void Gfx::clear(uint32_t rgb)
{
	target->Clear(color(rgb, 1));
}

void Gfx::fill(const Rect& r, uint32_t rgb, float radius, float alpha)
{
	if (radius > 0)
		target->FillRoundedRectangle(D2D1::RoundedRect(rectF(r), radius, radius), brush(rgb, alpha));
	else
		target->FillRectangle(rectF(r), brush(rgb, alpha));
}

void Gfx::stroke(const Rect& r, uint32_t rgb, float radius, float width, float alpha)
{
	// Inset by half the stroke so the line sits inside r, like a CSS border.
	Rect in = r.inset(width * 0.5f);
	if (radius > 0)
		target->DrawRoundedRectangle(D2D1::RoundedRect(rectF(in), radius, radius), brush(rgb, alpha), width);
	else
		target->DrawRectangle(rectF(in), brush(rgb, alpha), width);
}

void Gfx::line(float x1, float y1, float x2, float y2, uint32_t rgb, float width, float alpha)
{
	target->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), brush(rgb, alpha), width, roundStroke.Get());
}

void Gfx::circle(float cx, float cy, float r, uint32_t rgb, bool filled, float width, float alpha)
{
	D2D1_ELLIPSE e = D2D1::Ellipse(D2D1::Point2F(cx, cy), r, r);
	if (filled) target->FillEllipse(e, brush(rgb, alpha));
	else target->DrawEllipse(e, brush(rgb, alpha), width);
}

IDWriteTextFormat* Gfx::format(float size, Font f)
{
	uint64_t key = ((uint64_t)f << 32) | (uint32_t)(size * 100);
	auto it = formats.find(key);
	if (it != formats.end()) return it->second.Get();

	DWRITE_FONT_WEIGHT w = DWRITE_FONT_WEIGHT_NORMAL;
	if (f == Font::UiMedium) w = DWRITE_FONT_WEIGHT_MEDIUM;
	if (f == Font::UiSemibold) w = DWRITE_FONT_WEIGHT_SEMI_BOLD;
	const std::wstring& family = f == Font::Mono ? monoFamily : uiFamily;

	ComPtr<IDWriteTextFormat> fmt;
	dwrite->CreateTextFormat(family.c_str(), nullptr, w, DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL, size, L"en-us", &fmt);
	formats[key] = fmt;
	return fmt.Get();
}

void Gfx::text(const std::wstring& s, const Rect& r, float size, Font f, uint32_t rgb, Align a, float alpha)
{
	if (s.empty() || r.w <= 0) return;
	ComPtr<IDWriteTextLayout> layout;
	if (FAILED(dwrite->CreateTextLayout(s.c_str(), (UINT32)s.size(), format(size, f), r.w, r.h, &layout))) return;
	layout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
	layout->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	layout->SetTextAlignment(a == Align::Left ? DWRITE_TEXT_ALIGNMENT_LEADING
		: a == Align::Center ? DWRITE_TEXT_ALIGNMENT_CENTER : DWRITE_TEXT_ALIGNMENT_TRAILING);
	DWRITE_TRIMMING trim = { DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0 };
	ComPtr<IDWriteInlineObject> ellipsis;
	dwrite->CreateEllipsisTrimmingSign(layout.Get(), &ellipsis);
	layout->SetTrimming(&trim, ellipsis.Get());
	target->DrawTextLayout(D2D1::Point2F(r.x, r.y), layout.Get(), brush(rgb, alpha), D2D1_DRAW_TEXT_OPTIONS_CLIP);
}

float Gfx::textBlock(const std::wstring& s, const Rect& r, float size, Font f, uint32_t rgb, int maxLines, float lineHeight)
{
	if (s.empty() || r.w <= 0) return 0;
	float maxH = maxLines * lineHeight;
	ComPtr<IDWriteTextLayout> layout;
	if (FAILED(dwrite->CreateTextLayout(s.c_str(), (UINT32)s.size(), format(size, f), r.w, maxH, &layout))) return 0;
	layout->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, lineHeight, lineHeight * 0.8f);
	DWRITE_TRIMMING trim = { DWRITE_TRIMMING_GRANULARITY_WORD, 0, 0 };
	ComPtr<IDWriteInlineObject> ellipsis;
	dwrite->CreateEllipsisTrimmingSign(layout.Get(), &ellipsis);
	layout->SetTrimming(&trim, ellipsis.Get());
	target->DrawTextLayout(D2D1::Point2F(r.x, r.y), layout.Get(), brush(rgb, 1), D2D1_DRAW_TEXT_OPTIONS_CLIP);
	DWRITE_TEXT_METRICS m{};
	layout->GetMetrics(&m);
	return m.height < maxH ? m.height : maxH;
}

float Gfx::textWidth(const std::wstring& s, float size, Font f)
{
	if (s.empty()) return 0;
	ComPtr<IDWriteTextLayout> layout;
	if (FAILED(dwrite->CreateTextLayout(s.c_str(), (UINT32)s.size(), format(size, f), 10000, 100, &layout))) return 0;
	DWRITE_TEXT_METRICS m{};
	layout->GetMetrics(&m);
	return m.widthIncludingTrailingWhitespace;
}

// ---- SVG path data -> D2D geometry (M L H V C S Q A Z, absolute and relative) ----

namespace
{
	struct PathParser
	{
		const char* p;
		void skip() { while (*p && (std::isspace((unsigned char)*p) || *p == ',')) ++p; }
		bool number(float& out)
		{
			skip();
			if (!*p || !(std::isdigit((unsigned char)*p) || *p == '-' || *p == '+' || *p == '.')) return false;
			char* end = nullptr;
			out = std::strtof(p, &end);
			if (end == p) return false;
			p = end;
			return true;
		}
		bool flag(float& out)
		{
			// arc flags may be written without separators ("011")
			skip();
			if (*p == '0' || *p == '1') { out = (float)(*p - '0'); ++p; return true; }
			return false;
		}
	};
}


// SVG elliptical arc as cubic Beziers (endpoint -> centre parameterisation, SVG spec F.6.5).
// Done by hand rather than with AddArc so the result is identical everywhere.
static void addSvgArc(ID2D1GeometrySink* sink, float x0, float y0, float rx, float ry, float rotDeg,
	bool large, bool sweep, float x, float y)
{
	if (rx == 0 || ry == 0 || (x0 == x && y0 == y)) { sink->AddLine(D2D1::Point2F(x, y)); return; }
	const double pi = 3.14159265358979323846;
	double phi = rotDeg * pi / 180.0, c = std::cos(phi), s = std::sin(phi);
	double dx = (x0 - x) / 2.0, dy = (y0 - y) / 2.0;
	double x1p = c * dx + s * dy, y1p = -s * dx + c * dy;
	double ax = std::fabs(rx), ay = std::fabs(ry);
	double lam = (x1p * x1p) / (ax * ax) + (y1p * y1p) / (ay * ay);
	if (lam > 1) { ax *= std::sqrt(lam); ay *= std::sqrt(lam); }
	double num = ax * ax * ay * ay - ax * ax * y1p * y1p - ay * ay * x1p * x1p;
	double den = ax * ax * y1p * y1p + ay * ay * x1p * x1p;
	double k = std::sqrt(num > 0 ? num / den : 0) * (large == sweep ? -1 : 1);
	double cxp = k * ax * y1p / ay, cyp = -k * ay * x1p / ax;
	double cx = c * cxp - s * cyp + (x0 + x) / 2.0, cy = s * cxp + c * cyp + (y0 + y) / 2.0;
	auto angle = [](double ux, double uy, double vx, double vy) {
		double a = std::atan2(ux * vy - uy * vx, ux * vx + uy * vy);
		return a;
	};
	double t1 = angle(1, 0, (x1p - cxp) / ax, (y1p - cyp) / ay);
	double dt = angle((x1p - cxp) / ax, (y1p - cyp) / ay, (-x1p - cxp) / ax, (-y1p - cyp) / ay);
	if (!sweep && dt > 0) dt -= 2 * pi;
	if (sweep && dt < 0) dt += 2 * pi;
	int segs = (int)std::ceil(std::fabs(dt) / (pi / 2));
	double step = dt / segs;
	double kappa = 4.0 / 3.0 * std::tan(step / 4);
	auto pt = [&](double t, double ox, double oy) {
		double ex = ax * std::cos(t) + ox, ey = ay * std::sin(t) + oy;
		return D2D1::Point2F((float)(c * ex - s * ey + cx), (float)(s * ex + c * ey + cy));
	};
	double t = t1;
	for (int i = 0; i < segs; ++i)
	{
		double ta = t, tb = t + step;
		D2D1_POINT_2F p1 = pt(ta, -kappa * ax * std::sin(ta), kappa * ay * std::cos(ta));
		D2D1_POINT_2F p2 = pt(tb, kappa * ax * std::sin(tb), -kappa * ay * std::cos(tb));
		D2D1_POINT_2F p3 = pt(tb, 0, 0);
		sink->AddBezier(D2D1::BezierSegment(p1, p2, p3));
		t = tb;
	}
}

ID2D1PathGeometry* Gfx::geometry(const char* path)
{
	auto it = geometries.find(path);
	if (it != geometries.end()) return it->second.Get();

	ComPtr<ID2D1PathGeometry> geo;
	ComPtr<ID2D1GeometrySink> sink;
	factory->CreatePathGeometry(&geo);
	geo->Open(&sink);
	sink->SetFillMode(D2D1_FILL_MODE_WINDING);

	PathParser ps{ path };
	float cx = 0, cy = 0, sx = 0, sy = 0;   // current point, subpath start
	float lcx = 0, lcy = 0;                 // last cubic control point (for S)
	bool open = false;
	char cmd = 0, prev = 0;
	auto pt = [](float x, float y) { return D2D1::Point2F(x, y); };
	auto endFigure = [&](bool closed) {
		if (open) sink->EndFigure(closed ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
		open = false;
	};
	auto ensureOpen = [&]() {
		if (!open) { sink->BeginFigure(pt(cx, cy), D2D1_FIGURE_BEGIN_FILLED); open = true; sx = cx; sy = cy; }
	};

	for (;;)
	{
		ps.skip();
		if (!*ps.p) break;
		if (std::isalpha((unsigned char)*ps.p)) cmd = *ps.p++;
		else if (cmd == 'M') cmd = 'L';     // extra pairs after M are line-tos
		else if (cmd == 'm') cmd = 'l';
		bool rel = std::islower((unsigned char)cmd) != 0;
		float ox = rel ? cx : 0, oy = rel ? cy : 0;
		float a, b, c, d, e, f, g;
		switch (std::toupper((unsigned char)cmd))
		{
		case 'M':
			if (!ps.number(a) || !ps.number(b)) goto done;
			endFigure(false);
			cx = ox + a; cy = oy + b;
			sink->BeginFigure(pt(cx, cy), D2D1_FIGURE_BEGIN_FILLED);
			open = true; sx = cx; sy = cy;
			break;
		case 'L':
			if (!ps.number(a) || !ps.number(b)) goto done;
			ensureOpen(); cx = ox + a; cy = oy + b; sink->AddLine(pt(cx, cy));
			break;
		case 'H':
			if (!ps.number(a)) goto done;
			ensureOpen(); cx = ox + a; sink->AddLine(pt(cx, cy));
			break;
		case 'V':
			if (!ps.number(a)) goto done;
			ensureOpen(); cy = oy + a; sink->AddLine(pt(cx, cy));
			break;
		case 'C':
			if (!ps.number(a) || !ps.number(b) || !ps.number(c) || !ps.number(d) || !ps.number(e) || !ps.number(f)) goto done;
			ensureOpen();
			sink->AddBezier(D2D1::BezierSegment(pt(ox + a, oy + b), pt(ox + c, oy + d), pt(ox + e, oy + f)));
			lcx = ox + c; lcy = oy + d; cx = ox + e; cy = oy + f;
			break;
		case 'S':
		{
			if (!ps.number(c) || !ps.number(d) || !ps.number(e) || !ps.number(f)) goto done;
			ensureOpen();
			char pu = (char)std::toupper((unsigned char)prev);
			float x1 = (pu == 'C' || pu == 'S') ? 2 * cx - lcx : cx;
			float y1 = (pu == 'C' || pu == 'S') ? 2 * cy - lcy : cy;
			sink->AddBezier(D2D1::BezierSegment(pt(x1, y1), pt(ox + c, oy + d), pt(ox + e, oy + f)));
			lcx = ox + c; lcy = oy + d; cx = ox + e; cy = oy + f;
			break;
		}
		case 'Q':
			if (!ps.number(a) || !ps.number(b) || !ps.number(c) || !ps.number(d)) goto done;
			ensureOpen();
			sink->AddQuadraticBezier(D2D1::QuadraticBezierSegment(pt(ox + a, oy + b), pt(ox + c, oy + d)));
			cx = ox + c; cy = oy + d;
			break;
		case 'A':
			if (!ps.number(a) || !ps.number(b) || !ps.number(c) || !ps.flag(d) || !ps.flag(e) || !ps.number(f) || !ps.number(g)) goto done;
			ensureOpen();
			addSvgArc(sink.Get(), cx, cy, a, b, c, d != 0, e != 0, ox + f, oy + g);
			cx = ox + f; cy = oy + g;
			break;
		case 'Z':
			endFigure(true);
			cx = sx; cy = sy;
			break;
		default:
			goto done;
		}
		prev = cmd;
	}
done:
	endFigure(false);
	sink->Close();
	geometries[path] = geo;
	return geo.Get();
}

void Gfx::icon(const char* path, const Rect& box, uint32_t rgb, bool filled, float strokeWidth, float alpha)
{
	ID2D1PathGeometry* geo = geometry(path);
	float s = box.w / 24.0f;
	target->SetTransform(D2D1::Matrix3x2F::Scale(s, s) * D2D1::Matrix3x2F::Translation(box.x, box.y));
	if (filled) target->FillGeometry(geo, brush(rgb, alpha));
	else target->DrawGeometry(geo, brush(rgb, alpha), strokeWidth, roundStroke.Get());
	target->SetTransform(D2D1::Matrix3x2F::Identity());
}

ComPtr<ID2D1Bitmap> Gfx::loadImage(const std::vector<unsigned char>& bytes, UINT maxPixels)
{
	ComPtr<ID2D1Bitmap> bmp;
	if (!wic || bytes.empty() || !target) return bmp;
	ComPtr<IWICStream> stream;
	ComPtr<IWICBitmapDecoder> decoder;
	ComPtr<IWICBitmapFrameDecode> frame;
	ComPtr<IWICFormatConverter> conv;
	if (FAILED(wic->CreateStream(&stream))) return bmp;
	if (FAILED(stream->InitializeFromMemory(const_cast<BYTE*>(bytes.data()), (DWORD)bytes.size()))) return bmp;
	if (FAILED(wic->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnLoad, &decoder))) return bmp;
	if (FAILED(decoder->GetFrame(0, &frame))) return bmp;
	// Decode straight to the size it will be shown at: a 3000 px scan is 36 MB decoded, 300 px is 0.4 MB.
	ComPtr<IWICBitmapSource> source = frame;
	UINT w = 0, h = 0;
	frame->GetSize(&w, &h);
	if (maxPixels > 0 && (w > maxPixels || h > maxPixels))
	{
		ComPtr<IWICBitmapScaler> scaler;
		UINT sw = w >= h ? maxPixels : (UINT)((double)w * maxPixels / h);
		UINT sh = h >= w ? maxPixels : (UINT)((double)h * maxPixels / w);
		if (SUCCEEDED(wic->CreateBitmapScaler(&scaler)) &&
			SUCCEEDED(scaler->Initialize(frame.Get(), sw ? sw : 1, sh ? sh : 1, WICBitmapInterpolationModeFant)))
		{
			source = scaler;
		}
	}
	if (FAILED(wic->CreateFormatConverter(&conv))) return bmp;
	if (FAILED(conv->Initialize(source.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone,
		nullptr, 0.0, WICBitmapPaletteTypeCustom))) return bmp;
	target->CreateBitmapFromWicBitmap(conv.Get(), nullptr, &bmp);
	return bmp;
}

void Gfx::image(ID2D1Bitmap* bmp, const Rect& r, float radius)
{
	if (!bmp) return;
	// Crop to a centred square-ish region with the same aspect as r ("object-fit: cover").
	D2D1_SIZE_F sz = bmp->GetSize();
	float scale = (r.w / sz.width > r.h / sz.height) ? r.w / sz.width : r.h / sz.height;
	float sw = r.w / scale, sh = r.h / scale;
	D2D1_RECT_F src = D2D1::RectF((sz.width - sw) / 2, (sz.height - sh) / 2, (sz.width + sw) / 2, (sz.height + sh) / 2);

	ComPtr<ID2D1RoundedRectangleGeometry> clip;
	ComPtr<ID2D1Layer> layer;
	if (radius > 0)
	{
		factory->CreateRoundedRectangleGeometry(D2D1::RoundedRect(rectF(r), radius, radius), &clip);
		target->CreateLayer(&layer);
		target->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), clip.Get()), layer.Get());
	}
	target->DrawBitmap(bmp, rectF(r), 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, src);
	if (radius > 0) target->PopLayer();
}

void Gfx::pushClip(const Rect& r)
{
	target->PushAxisAlignedClip(rectF(r), D2D1_ANTIALIAS_MODE_ALIASED);
}

void Gfx::popClip()
{
	target->PopAxisAlignedClip();
}
