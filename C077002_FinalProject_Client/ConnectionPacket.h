#pragma once
#include "Packet.h"

class ConnectionPacket : public Packet {
private:
public:
	ConnectionPacket(HEADER header, char id, short length, short endMarker);
	~ConnectionPacket();

	void Serialize(std::vector<char>& buffer) override;
	void DeSerialize(std::vector<char>& buffer) override;
	void SetLength() override;
	void PrintSize() const override;
	void PrintItem() const override;
};

