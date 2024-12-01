#pragma once

#include "Packet.h"
class ChatPacket : public Packet {
private:

public:
	ChatPacket(HEADER header, char id, short length, short endMarker);
	~ChatPacket();

	void Serialize(std::vector<char>& buffer) override;
	void DeSerialize(std::vector<char>& buffer) override;
	void SetLength() override;
	void PrintSize() const override;
	void PrintItem() const override;
};
