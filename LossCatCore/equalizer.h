#pragma once
#include "biquad.h"
#include <algorithm>
class Equalizer
{
public:
	static constexpr int Bands = 10;
	static constexpr double Freqs[Bands] = { 31, 62, 125, 250, 500, 1000, 2000, 4000, 8000, 16000 };

	void setGain(int band, double dB);   
	void setPreamp(double dB);           
	void setEnabled(bool on);           

	void prepare(double sampleRate);    
	bool isActive() const;               
	void process(float* samples, int frames, int channels);

private:
	void update(int band);              
	Biquad bands[Bands][2];              
	double gains[Bands] = {};            
	double preampDb = 0;
	bool enabled = false;
	double rate = 44100;
};