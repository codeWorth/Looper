#pragma once

#include "Constants.h"
#include <utility>
#include <functional>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/map.hpp>

namespace bip = boost::interprocess;

struct LooperUpdate {
	LooperUpdate() {
		std::fill_n(volume, nLoops, 1.f);
	}

	LooperUpdate(const LooperUpdate& u) :
		startRecord(u.startRecord), stopRecord(u.stopRecord), hasUpdate(u.hasUpdate) 
	{
		for (int i = 0; i < nLoops; i++) {
			volume[i] = u.volume[i];
		}
	}

	float volume[nLoops];
	int8_t startRecord = -1;
	bool stopRecord = false;
	bool hasUpdate = false;

	void clear() {
		startRecord = -1;
		stopRecord = false;
		hasUpdate = false;
	}
};
typedef std::pair<void* const, LooperUpdate> SharedMapEntry;
typedef bip::allocator<SharedMapEntry, bip::managed_shared_memory::segment_manager> SharedAllocator;
typedef bip::map<void*, LooperUpdate, std::less<void*>, SharedAllocator> SharedMap;

class LoopSyncer {
public:

	class MessageListener {
	public:
		virtual void startRecordLoop(int loopIndex) = 0;
		virtual void stopRecordLoop() = 0;
		virtual void setLoopVolume(int loopIndex, float volume) = 0;
	};

	LoopSyncer(MessageListener* listener) : listener(listener), shm(bip::open_or_create, MEM_NAME, 8192) {
		using namespace boost::interprocess;

		sharedMap = shm.find_or_construct<SharedMap>("SHARED_MAP")(SharedAllocator(shm.get_segment_manager()));
		
		if (sharedMap->size() > 0) {
			auto existingState = LooperUpdate(sharedMap->begin()->second);
			sharedMap->emplace(this, existingState);
			sharedMap->at(this).hasUpdate = true;
			handleUpdates();
		} else {
			sharedMap->emplace(this, LooperUpdate());
		}
	}

	~LoopSyncer() {
		sharedMap->erase(this);

		if (sharedMap->size() == 0) {
			bip::shared_memory_object::remove(MEM_NAME);
		}
	}

	void handleUpdates() {
		LooperUpdate& entry = sharedMap->at(this);
		if (!entry.hasUpdate) return;

		for (int i = 0; i < nLoops; i++) {
			listener->setLoopVolume(i, entry.volume[i]);
		}

		if (entry.startRecord != -1) {
			listener->startRecordLoop(entry.startRecord);
		}

		if (entry.stopRecord) {
			listener->stopRecordLoop();
		}

		entry.clear();
	}

	void broadcastStartRecord(int loopIndex) {
		for (auto& [key, entry] : *sharedMap) {
			if (key == this) continue;

			entry.startRecord = loopIndex;
			entry.hasUpdate = true;
		}
	}

	void broadcastStopRecord() {
		for (auto& [key, entry] : *sharedMap) {
			if (key == this) continue;

			entry.stopRecord = true;
			entry.hasUpdate = true;
		}
	}

	void broadcastLoopVolume(int loopIndex, float volume) {
		for (auto& [key, entry] : *sharedMap) {
			entry.volume[loopIndex] = volume;
			entry.hasUpdate = key != this;
		}
	}

private:
	const char* MEM_NAME = "LOOPER_MEM";

	MessageListener* listener;
	SharedMap* sharedMap;
	bip::managed_shared_memory shm;
};
