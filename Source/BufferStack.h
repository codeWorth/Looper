#pragma once

#include <cassert>
#include <utility>
#include <algorithm>

template <typename T>
class BufferStack {
public:
	BufferStack(int count) : availableCount(0), count(count), bufferSize(0), buffers(nullptr) {}

	~BufferStack() {
		if (buffers == nullptr) return;

		for (int i = 0; i < count; i++) {
			delete[] buffers[i];
		}
		delete[] buffers;
		delete[] bufferContainers;
	}

	void setupBuffersIfNeeded(int bufferSize, const T& fill) {
		if (bufferSize == this->bufferSize) return;
		this->bufferSize = bufferSize;

		if (buffers == nullptr) { // setup buffers arrays if it's not been intialized
			buffers = new T*[count];
			bufferContainers = new Buffer*[count];
		} else { // free old buffers if they've been created
			for (int i = 0; i < count; i++) {
				delete[] buffers[i];
			}
		}

		for (int i = 0; i < count; i++) {
			buffers[i] = new T[bufferSize];
			std::fill_n(buffers[i], bufferSize, fill);
		}

		availableCount = count;
	}

	class Buffer {
		friend class BufferStack;

	public:
		Buffer(BufferStack& stack) : stack(stack), data(stack.acquire(index, this)) {};
		~Buffer() {
			stack.free(index);
		}

		T* get() {
			return data;
		}

	private:
		T* data;
		int index;
		BufferStack& stack;
	};

	T* get(int index) {
		return buffers[index];
	}

private:
	const int count;
	int availableCount; // first N items of buffers are available
	int bufferSize;
	T** buffers;
	Buffer** bufferContainers;

	T* acquire(int& index, Buffer* bufferContainer) {
		assert(availableCount > 0);

		availableCount--;
		index = availableCount;
		bufferContainers[index] = bufferContainer;
		return buffers[index];
	}

	void free(int index) {
		assert(index >= availableCount);

		if (index != availableCount) {
			std::swap(bufferContainers[availableCount]->index, bufferContainers[index]->index);
			std::swap(bufferContainers[availableCount], bufferContainers[index]);
			std::swap(buffers[availableCount], buffers[index]);
		}

		availableCount++;
	}
};