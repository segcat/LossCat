#pragma once
#include <cmath>

struct Biquad {
	double b0 = 1, b1 = 0, b2 = 0;
	double a1 = 0, a2 = 0;
	double z1 = 0, z2 = 0;
	void reset()
	{
		z1 = z2 = 0;
	}
	void setPeak(double sampleRate, double frequency, double gainDb, double q)
	{
		constexpr double pi = 3.14159265358979323846;
		double a = std::pow(10.0, gainDb / 40.0);
		double w0 = 2.0 * pi * frequency / sampleRate;
		double alpha = std::sin(w0) / (2.0 * q);
		double cosw0 = std::cos(w0);
		double a0 = 1.0 + alpha / a;

		b0 = (1.0 + alpha * a) / a0;
		b1 = (-2.0 * cosw0) / a0;
		b2 = (1.0 - alpha * a) / a0;
		a1 = (-2.0 * cosw0) / a0;
		a2 = (1.0 - alpha / a) / a0;
	}
	float process(float input)
	{
		double output = b0 * input + z1;
		z1 = b1 * input + z2 - a1 * output;
		z2 = b2 * input - a2 * output;
		return (float)output;
	}

};