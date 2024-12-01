#pragma once
#include"Packet.h"

class MovePacket : public Packet {
private:
	short x;
	short y;
	short z;
public:
	MovePacket(HEADER header, char id, short length, short x, short y, short z, short endMarker);
	~MovePacket();

	void Serialize(std::vector<char>& buffer) override;
	void DeSerialize(std::vector<char>& buffer) override;
	void SetLength() override;
	void PrintSize() const override;
	void PrintItem() const override;
	void SetPos(short x, short y, short z);
	short GetPosX() const;
	short GetPosY() const;
	short GetPosZ() const;
};

