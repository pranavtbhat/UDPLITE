#ifndef UDPLITE_H_
#define UDPLITE_H_

#include "packet.h"

#define UDPLITE_HEADER_SIZE 16
#define PAYLOAD_DATA_SIZE 10

struct udplite_payload{
	unsigned short size;
	unsigned short checksum;
	unsigned char *data;
};

struct udp_payload{
	unsigned short size;
	unsigned short checksum;
	unsigned char *data;
};


struct hdr_udplite {

	//Common to both udplite and udp
	unsigned char ver;                //0 for udplite and 1 for udp
	unsigned short header_checksum;
	unsigned char header[16];          //8 byte udp header + 4 byte dest address + 4 byte source address 

	//only for udplite
	unsigned short nunits;
	struct udplite_payload *udplite_data;

	//only for udp
	struct udp_payload *udp_data;

	// necessary for access
	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_udplite* access(const Packet* p) {
		return (hdr_udplite*) p->access(offset_);
	}
};

#endif