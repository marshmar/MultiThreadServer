#include "Packet.h"

Packet::Packet(HEADER header, char id, short length, short endMarker)
	: header(header), id(id), length(length), endMarker(endMarker)
{

}

Packet::~Packet() { }
void Packet::SetLength() { }
void Packet::PrintSize() const { }
void Packet::PrintItem() const { }
short Packet::GetLength() const {
	return length;
}

void Packet::SetHeader(HEADER header) {
	this->header = header;
}
HEADER Packet::GetHeader() const {
	return header;
}

void Packet::SetString(std::string data) {
	buf = data;
}

std::string Packet::GetString() const {
	return buf;
}


char Packet::GetID() const {
	return id;
}





