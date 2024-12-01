#pragma once

#include<vector>
#include <string>
#include <iostream>

// 패킷 헤더 종류
enum class HEADER : char {
	REQ_CON = 'a', ACK_CON, REQ_DIS, ACK_DIS, REQ_MOVE, ACK_MOVE, REQ_CHAT, ACK_CHAT
};



// 패킷 추상클래스
class Packet abstract
{
protected:
	HEADER header;
	short length;
	short endMarker;
	std::string buf;
	char id;
public:

	Packet(HEADER header, char id, short length, short endMarker);
	~Packet();
	virtual void Serialize(std::vector<char>& buffer) = 0;
	virtual void DeSerialize(std::vector<char>& buffer) = 0;


	// Getter & Setter
	short GetLength() const;
	virtual void SetLength();
	void SetHeader(HEADER header);
	HEADER GetHeader() const;
	void SetString(std::string data);
	std::string GetString() const;
	char GetID() const;
	virtual void PrintSize() const;
	virtual void PrintItem() const;
};

