// Copyright 2025 tilr
// Mallet generator, a simple unit impulse ran through a bandpass filter
#pragma once
#include "Filter.h"
// #include "Sampler.h"
// #include "vector"

enum MalletType
{
	kImpulse,
	kReserved1,
	kReserved2,
	kReserved3,
	kReserved4,
	kReserved5,
	kReserved6,
	kReserved7,
	kReserved8,
	kReserved9,
	kReserved10,
	kUserFile,
	kSample1,
	kSample2,
	kSample3,
	kSample4,
	kSample5,
	kSample6,
};

// class Sampler;

class Mallet
{
public:
	// TODO:
	// Mallet(Sampler& sampler) : sampler(sampler) {};
	Mallet() {};
	~Mallet() {};

	void trigger(/**<MalletType type, */float32_t srate, float32_t freq);
	void clear();
	float32_t process();

	// void setFilter(float32_t norm);

	// float32_t srate = 44100.0;

	// impulse mallet fields
	float32_t impulse = 0.0;
	int elapsed = 0;
	float32_t env = 0.0;
	// Filter impulse_filter{};

	// sample mallet fields
	// float32_t playback = INFINITY;
	// float32_t playback_speed = 1.0;
	// bool disable_filter = false;
	// Filter sample_filter{};

private:
	// Sampler& sampler;    // disable samples for the moment
	// MalletType type = kImpulse;
	Filter filter{};
};