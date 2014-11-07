#include "udplitepacket.h"

static class UDPLiteHeaderClass : public PacketHeaderClass {
public:
	UDPLiteHeaderClass() : PacketHeaderClass("PacketHeader/UDPLite",sizeof(hdr_udplite)) {
		bind_offset(&hdr_udplite::offset_);
	}
} class_udplite_header;

int hdr_udplite::offset_;