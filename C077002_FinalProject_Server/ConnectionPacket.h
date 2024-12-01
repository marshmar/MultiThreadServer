#pragma once
#include "Packet.h"

/// <summary>
/// 연결 패킷, 패킷 클래스로 부터 상속받은 멤버를 가진다.
/// </summary>
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

