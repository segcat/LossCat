#include "equalizer.h"

void Equalizer::update(int band)
{
	if (band < 0 || band >= Bands) return;
	double gain = gains[band];
	double q = std::sqrt(2.0);
	double freq = std::min(Freqs[band], rate * 0.45);
	for (int c = 0; c < 2; ++c)
	{
		bands[band][c].setPeak(rate, freq, gain, q);
	}

}

void Equalizer::prepare(double sampleRate)
{
	if (sampleRate <= 0) return;
	rate = sampleRate;
	for (int b = 0; b < Bands; ++b)
	{
		update(b);
	}
	for (int b = 0; b < Bands; ++b)
	{
		for (int c = 0; c < 2; ++c)
		{
			bands[b][c].reset();
		}
	}
}

void Equalizer::setGain(int band, double dB)
{
	if (band < 0 || band >= Bands) return;
	gains[band] = std::clamp(dB, -12.0, 12.0);
	update(band);
}

void Equalizer::setPreamp(double dB)
{
	preampDb = std::clamp(dB, -12.0, 12.0);
}

void Equalizer::setEnabled(bool on)
{
	enabled = on;
}

bool Equalizer::isActive() const
{
	if (!enabled) return false;
	return preampDb != 0 || std::any_of(gains, gains + Bands, [](double g) { return g != 0; });
}

void Equalizer::process(float* samples, int frames, int channels)
{
	if (!isActive() || frames <= 0 || channels != 2) return;
	float pre = (float)std::pow(10.0, preampDb / 20.0);
	for (int f = 0; f < frames; ++f)
	{
		for (int ch = 0; ch < 2; ++ch)
		{
			float s = samples[f * channels + ch] * pre;
			for (int b = 0; b < Bands; ++b)
				s = bands[b][ch].process(s);
			samples[f * channels + ch] = s;
		}
	}
}