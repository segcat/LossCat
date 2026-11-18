#pragma once
// Thin Direct2D / DirectWrite / WIC layer. Everything is in DIPs (1 DIP = 1 CSS px
// on the design canvas); the render target's DPI does the scaling.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

using Microsoft::WRL::ComPtr;

struct Rect
{
	float x = 0, y = 0, w = 0, h = 0;
	float right() const { return x + w; }
	float bottom() const { return y + h; }
	float cx() const { return x + w * 0.5f; }
	float cy() const { return y + h * 0.5f; }
	bool contains(float px, float py) const { return px >= x && px < x + w && py >= y && py < y + h; }
	Rect inset(float d) const { return { x + d, y + d, w - 2 * d, h - 2 * d }; }
};

enum class Font { Ui, UiMedium, UiSemibold, Mono };
enum class Align { Left, Center, Right };

class Gfx
{
public:
	bool init(HWND hwnd);
	void setDpi(float dpi);
	void resize(UINT pixelW, UINT pixelH);

	bool begin();   // false if there is nothing to draw on
	void end();     // recreates the target on D2DERR_RECREATE_TARGET
	unsigned generation() const { return gen; } // bumps when GPU resources are recreated

	void clear(uint32_t rgb);
	void fill(const Rect& r, uint32_t rgb, float radius = 0, float alpha = 1);
	void stroke(const Rect& r, uint32_t rgb, float radius = 0, float width = 1, float alpha = 1);
	void line(float x1, float y1, float x2, float y2, uint32_t rgb, float width = 1, float alpha = 1);
	void circle(float cx, float cy, float r, uint32_t rgb, bool filled = true, float width = 1, float alpha = 1);

	// Single line, vertically centred in r, trimmed with an ellipsis.
	void text(const std::wstring& s, const Rect& r, float size, Font f, uint32_t rgb,
		Align a = Align::Left, float alpha = 1);
	// Wrapped, top-aligned, at most maxLines lines; returns the height used.
	float textBlock(const std::wstring& s, const Rect& r, float size, Font f, uint32_t rgb,
		int maxLines, float lineHeight);
	float textWidth(const std::wstring& s, float size, Font f);

	// Draw an SVG path (24x24 viewBox) scaled into box. `path` must be a string literal
	// (geometries are cached by pointer).
	void icon(const char* path, const Rect& box, uint32_t rgb, bool filled, float strokeWidth = 1.5f, float alpha = 1);

	ComPtr<ID2D1Bitmap> loadImage(const std::vector<unsigned char>& bytes, UINT maxPixels = 0);   // 0 = full size
	void image(ID2D1Bitmap* bmp, const Rect& r, float radius);

	void pushClip(const Rect& r);
	void popClip();

private:
	bool createTarget();
	IDWriteTextFormat* format(float size, Font f);
	ID2D1PathGeometry* geometry(const char* path);
	ID2D1SolidColorBrush* brush(uint32_t rgb, float alpha);

	HWND hwnd = nullptr;
	float dpi = 96.0f;
	unsigned gen = 0;
	ComPtr<ID2D1Factory> factory;
	ComPtr<IDWriteFactory> dwrite;
	ComPtr<IWICImagingFactory> wic;
	ComPtr<ID2D1HwndRenderTarget> target;
	ComPtr<ID2D1SolidColorBrush> solid;
	ComPtr<ID2D1StrokeStyle> roundStroke;
	std::wstring uiFamily, monoFamily;
	std::unordered_map<uint64_t, ComPtr<IDWriteTextFormat>> formats;
	std::unordered_map<const char*, ComPtr<ID2D1PathGeometry>> geometries;
};

std::wstring widen(const std::string& utf8);
std::string narrow(const std::wstring& w);
