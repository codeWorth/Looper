#pragma once

#include "JuceHeader.h"
#include <vector>
#include <stdexcept>

class LoopSyncer : 
	public juce::InterprocessConnectionServer,
	public juce::InterprocessConnection 
{
public:

	class MessageListener {
	public:
		virtual void startRecordLoop(int loopIndex) = 0;
		virtual void stopRecordLoop() = 0;
		virtual void setLoopVolume(int loopIndex, float volume) = 0;
	};

	LoopSyncer(MessageListener* listener) : listener(listener) {
		if (!connectToSocket(SERVER_ADDRESS, SERVER_PORT, 50)) {
			startServer();
		}
	}

	~LoopSyncer() {
		if (isServer && clients.size() > 0) {
			BecomeHostMessage message;
			clients[0].get()->sendMessage(juce::MemoryBlock(&message, sizeof(BecomeHostMessage)));
		}

		stop();
		disconnect();
	}

	void connectionMade() override {}

	void connectionLost() override {
		if (shouldBecomeServer) {
			shouldBecomeServer = false;
			startServer();
		} else {
			connectToSocket(SERVER_ADDRESS, SERVER_PORT, 100);
		}
	}

	void messageReceived(const juce::MemoryBlock& message) {
		MessageType type = (MessageType) message[0];

		switch (type) {
		case LoopSyncer::BECOME_HOST:
			shouldBecomeServer = true;
			break;

		case LoopSyncer::START_RECORDING:
			listener->startRecordLoop(((StartRecordMessage*)message.getData())->loopIndex);
			break;

		case LoopSyncer::STOP_RECORDING:
			listener->stopRecordLoop();
			break;

		case LoopSyncer::SET_VOLUME:
			listener->setLoopVolume(
				((SetVolumeMessage*)message.getData())->loopIndex,
				((SetVolumeMessage*)message.getData())->volume
			);
			break;

		default:
			throw std::invalid_argument("Unknown message type received!");
		}
	}

	void startRecordLoop(int loopIndex) {
		StartRecordMessage message(loopIndex);
		broadcastMessage(juce::MemoryBlock(&message, sizeof(StartRecordMessage)));
	}

	void stopRecordLoop() {
		StopRecordMessage message;
		broadcastMessage(juce::MemoryBlock(&message, sizeof(StopRecordMessage)));
	}

	void setLoopVolume(int loopIndex, float volume) {
		SetVolumeMessage message(loopIndex, volume);
		broadcastMessage(juce::MemoryBlock(&message, sizeof(SetVolumeMessage)));
	}

	bool getIsServer() const {
		return isServer;
	}

protected:
	struct ServerConnection : juce::InterprocessConnection {
		~ServerConnection() {
			disconnect();
		}

		void connectionMade() override {}

		void connectionLost() override {}

		void messageReceived(const juce::MemoryBlock& message) {
			throw std::invalid_argument("A message was sent to looper server, which is invalid.");
		}
	};

	juce::InterprocessConnection* createConnectionObject() override {
		clients.push_back(std::make_unique<ServerConnection>());
		return clients.back().get();
	}

private:
	const juce::String SERVER_ADDRESS = "localhost";
	const int SERVER_PORT = 4444;

	bool isServer = false;
	bool shouldBecomeServer = false;

	std::vector<std::unique_ptr<juce::InterprocessConnection>> clients;
	MessageListener* listener;

	void startServer() {
		isServer = true;
		beginWaitingForSocket(SERVER_PORT, SERVER_ADDRESS);
	}

	void broadcastMessage(const juce::MemoryBlock& message) {
		if (!isServer) return;

		for (auto& client : clients) {
			client.get()->sendMessage(message);
		}
	}

	enum MessageType : char {
		BECOME_HOST,
		START_RECORDING,
		STOP_RECORDING,
		SET_VOLUME
	};

	struct BecomeHostMessage {
		MessageType type = BECOME_HOST;
	};

	struct StartRecordMessage {
		MessageType type = START_RECORDING;
		uint8_t loopIndex;

		StartRecordMessage(uint8_t loopIndex) : loopIndex(loopIndex) {};
	};

	struct StopRecordMessage {
		MessageType type = STOP_RECORDING;
	};

	struct SetVolumeMessage {
		MessageType type = SET_VOLUME;
		uint8_t loopIndex;
		float volume;

		SetVolumeMessage(uint8_t loopIndex, float volume) : loopIndex(loopIndex), volume(volume) {};
	};
};
