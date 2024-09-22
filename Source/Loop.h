#pragma once

#include <cassert>
#include <cstring>
#include <algorithm>

constexpr int FADE_SAMPLES = 200;

template<typename T>
class Loop {
public:
	Loop<T>() : data(nullptr), size(0), samplesPerBeat(0), beatsPerLoop(0) {
		preLoop = new T[FADE_SAMPLES];
	}

	~Loop<T>() {
		delete[] data;
		delete[] preLoop;
	}

	T& operator[](size_t x) {
		assert(x >= 0 && x < size);
		return data[x];
	}

	T operator[](size_t x) const {
		assert(x >= 0 && x < size);
		return data[x];
	}
	
	/*
	* Based on the current position of the playhead, copy the corresponding region of the loop into a buffer.
	* @param dest Pointer of the buffer to copy into.
	* @param currentSample Location of the playhead in samples.
	* @param bufferSize Number of values to copy.
	*/
	void readBuffer(T* dest, size_t currentSample, int bufferSize) const {
		assert(bufferSize <= size);
		size_t loopSample = getLoopSample(currentSample, bufferSize);
		
		// copy to the end of loop into buffer
		size_t samplesToEnd = size - loopSample;
		if (samplesToEnd > bufferSize) samplesToEnd = bufferSize;
		readAll(dest, loopSample, samplesToEnd);

		// copy remaining from start of loop if needed
		if (bufferSize > samplesToEnd) {
			readAll(dest + samplesToEnd, 0, bufferSize - samplesToEnd);
		}

		auto fadeStart = size - FADE_SAMPLES;
		if (loopSample + bufferSize <= fadeStart) return;

		size_t start = std::max(fadeStart, loopSample);
		int count = std::min(bufferSize, (int)(size - start));

		for (size_t i = start; i < start + count; i++) {
			size_t fadeIndex = i - fadeStart;
			size_t destIndex = i - loopSample;
			float fadePercent = ((float)fadeIndex) / FADE_SAMPLES;
			dest[destIndex] = dest[destIndex] * (1 - fadePercent) + preLoop[fadeIndex] * fadePercent;
		}
	}

	void fill(T value) {
		std::fill_n(data, size, value);
		std::fill_n(preLoop, FADE_SAMPLES, value);
	}

	size_t getSize() const {
		return size;
	}

	/*
	* Set the length of the loop and fill it with the given value.
	* @param samplesPerBeat Number of samples per beat.
	* @param beatsPerLoop Number of beats per loop.
	* @param value The value to fill the loop with.
	*/
	void setLength(double samplesPerBeat, int beatsPerLoop, T value) {
		this->samplesPerBeat = samplesPerBeat;
		this->beatsPerLoop = beatsPerLoop;
		size = ceil(samplesPerBeat * beatsPerLoop);
		
		delete[] data;
		data = new T[size];

		fill(value);
	}

protected:
	/*
	* Copy the given elements to the loop.
	* @param elements Elements to copy.
	* @param index Location within the loop to copy to.
	* @param count Number of values to copy.
	*/
	void copyAll(const T* elements, size_t index, size_t count) {
		assert(index + count <= getSize());
		assert(index >= 0);

		std::memcpy(data + index, elements, sizeof(T) * count);
	}

	/*
	* Copies the end of the the loop into the pre loop buffer for crossfade purposes.
	*/
	void copyPreLoop() {
		auto fadeStart = size - FADE_SAMPLES;
		std::memcpy(preLoop, data + fadeStart, sizeof(T) * FADE_SAMPLES);
	}

	size_t getLoopSample(size_t currentSample, int bufferSize) const {
		double currentBeat = ((double)currentSample) / samplesPerBeat;
		double loopsFromStart = floor(currentBeat / beatsPerLoop);
		size_t loopStartInSamples = floor(loopsFromStart * beatsPerLoop * samplesPerBeat);
		return currentSample - loopStartInSamples;	// time within loop in sample units. 
													// If currentSample is at the start of the loop, this value will be 0.
	}

	void swapData(Loop<T>& other) {
		assert(size == other.size);
		std::swap(data, other.data);
		std::swap(preLoop, other.preLoop);
	}

private:
	T* data;
	T* preLoop;
	size_t size;
	double samplesPerBeat;
	int beatsPerLoop;

	/*
	* Copy a region of the loop into the given destination.
	* @param dest Pointer to copy into.
	* @param index Location within the loop to copy from.
	* @param count Number of values to copy.
	*/
	void readAll(T* dest, size_t index, size_t count) const {
		assert(index + count <= size);
		assert(index >= 0);

		std::memcpy(dest, data + index, sizeof(T) * count);
	}

};