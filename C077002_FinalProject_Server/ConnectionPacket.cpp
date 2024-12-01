#include "ConnectionPacket.h"

ConnectionPacket::ConnectionPacket(HEADER header, char id, short length, short endMarker)
	: Packet(header, id, length, endMarker)
{ }

ConnectionPacket::~ConnectionPacket() {

}

void ConnectionPacket::Serialize(std::vector<char>& buffer) {
	short offset = 0;
	buffer.clear();
	buffer.resize(length);

	memcpy(buffer.data(), reinterpret_cast<const char*>(&header), sizeof(header));
	offset += sizeof(header);
	memcpy(buffer.data() + offset, reinterpret_cast<const char*>(&length), sizeof(length));
	offset += sizeof(length);
	memcpy(buffer.data() + offset, reinterpret_cast<const char*>(&id), sizeof(id));
	offset += sizeof(id);
	memcpy(buffer.data() + offset, buf.data(), buf.length());
	offset += buf.length();
	memcpy(buffer.data() + offset, reinterpret_cast<const char*>(&endMarker), sizeof(endMarker));
	offset += sizeof(endMarker);
}

void ConnectionPacket::DeSerialize(std::vector<char>& buffer) {
	short offset = 0;

	memcpy(&header, buffer.data(), sizeof(header));
	offset += sizeof(header);
	memcpy(&length, buffer.data() + offset, sizeof(length));
	offset += sizeof(length);
	memcpy(&id, buffer.data() + offset, sizeof(id));
	offset += sizeof(id);
	while (offset < buffer.size()) {
		if (memcmp(buffer.data() + offset, &endMarker, sizeof(endMarker)) == 0) {
			offset += sizeof(endMarker);
			break;
		}
		buf.push_back(buffer[offset]);
		offset += 1;
	}
}

void ConnectionPacket::SetLength() {
	length = sizeof(header) + sizeof(length) + sizeof(id) + buf.size() + sizeof(endMarker);
}

void ConnectionPacket::PrintSize() const {
	std::cout << "header: " << sizeof(header) << std::endl;
	std::cout << "length: " << sizeof(length) << std::endl;
	std::cout << "id: " << sizeof(id) << std::endl;
	std::cout << "buf: " << buf.size() << std::endl;
	std::cout << "endMarker: " << sizeof(endMarker) << std::endl;
}

void ConnectionPacket::PrintItem() const {
	std::cout << "length: " << length << std::endl;
	std::cout << "id: " << id << std::endl;
	std::cout << "buf: " << buf << std::endl;
	std::cout << "endMarker: " << endMarker << std::endl;
}

