#pragma once
#include"Packet.h"

/// <summary>
/// 이동 패킷, 패킷 클래스로 부터 상속받은 멤버들과, 이동 패킷의 고유 멤버 x, y, z를 가진다.
/// </summary>
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

