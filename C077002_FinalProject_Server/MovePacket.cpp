#include "MovePacket.h"

MovePacket::MovePacket(HEADER header, char id, short length, short x, short y, short z, short endMarker)
	: Packet(header, id, length, endMarker), x(x), y(y), z(z)
{ }

MovePacket::~MovePacket() { }

void MovePacket::Serialize(std::vector<char>& buffer) {
	short offset = 0;
	buffer.clear();
	buffer.resize(length);

	memcpy(buffer.data(), reinterpret_cast<const char*>(&header), sizeof(header));
	offset += sizeof(header);
	memcpy(buffer.data() + offset, reinterpret_cast<const char*>(&length), sizeof(length));
	offset += sizeof(length);
	memcpy(buffer.data() + offset, reinterpret_cast<const char*>(&id), sizeof(id));
	offset += sizeof(id);
	memcpy(buffer.data() + offset, reinterpret_cast<const char*>(&x), sizeof(x));
	offset += sizeof(x);
	memcpy(buffer.data() + offset, reinterpret_cast<const char*>(&y), sizeof(y));
	offset += sizeof(y);
	memcpy(buffer.data() + offset, reinterpret_cast<const char*>(&z), sizeof(z));
	offset += sizeof(z);
	memcpy(buffer.data() + offset, buf.data(), buf.length());
	offset += buf.length();
	memcpy(buffer.data() + offset, reinterpret_cast<const char*>(&endMarker), sizeof(endMarker));
	offset += sizeof(endMarker);
}

void MovePacket::DeSerialize(std::vector<char>& buffer) {
	short offset = 0;

	memcpy(&header, buffer.data(), sizeof(header));
	offset += sizeof(header);
	memcpy(&length, buffer.data() + offset, sizeof(length));
	offset += sizeof(length);
	memcpy(&id, buffer.data() + offset, sizeof(id));
	offset += sizeof(id);
	memcpy(&x, buffer.data() + offset, sizeof(x));
	offset += sizeof(x);
	memcpy(&y, buffer.data() + offset, sizeof(y));
	offset += sizeof(y);
	memcpy(&z, buffer.data() + offset, sizeof(z));
	offset += sizeof(z);
	while (offset < buffer.size()) {
		if (memcmp(buffer.data() + offset, &endMarker, sizeof(endMarker)) == 0) {
			offset += sizeof(endMarker);
			break;
		}
		buf.push_back(buffer[offset]);
		offset += 1;
	}
}

void MovePacket::SetLength() {
	length = sizeof(header) + sizeof(length) + sizeof(id) + sizeof(x) + sizeof(y) + sizeof(z) + buf.size() + sizeof(endMarker);
}

void MovePacket::PrintSize() const {
	std::cout << "header: " << sizeof(header) << std::endl;
	std::cout << "length: " << sizeof(length) << std::endl;
	std::cout << "x: " << sizeof(x) << std::endl;
	std::cout << "x: " << sizeof(y) << std::endl;
	std::cout << "x: " << sizeof(z) << std::endl;
	std::cout << "buf: " << buf.size() << std::endl;
	std::cout << "endMarker: " << sizeof(endMarker) << std::endl;
}

void MovePacket::PrintItem() const {
	std::cout << "length: " << length << std::endl;
	std::cout << "x: " << x << std::endl;
	std::cout << "y: " << y << std::endl;
	std::cout << "z: " << z << std::endl;
	std::cout << "buf: " << buf << std::endl;
	std::cout << "endMarker: " << endMarker << std::endl;
}

void MovePacket::SetPos(short x, short y, short z) {
	this->x = x;
	this->y = y;
	this->z = z;
}

short MovePacket::GetPosX() const {
	return x;
}
short MovePacket::GetPosY() const {
	return y;
}
short MovePacket::GetPosZ() const {
	return z;
}
