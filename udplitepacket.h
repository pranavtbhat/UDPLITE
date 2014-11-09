/** 
	Header file for the Udplite Agent.

	@author Pranav Bhat and Rohit Varkey
	@version 1.0
*/

#ifndef UDPLITE_H_
#define UDPLITE_H_

#include "packet.h"

#define UDPLITE_HEADER_SIZE 16
#define PAYLOAD_DATA_SIZE 10


/**
	This structure simulates a segment of the udplite payload. The size field denotes the coverage of the checksum.
	The checksum field is exactly 16 bits in length.
*/
struct udplite_payload{
	unsigned short size;
	unsigned short checksum;
	unsigned char *data;
};

/**
	This structure simulates the udp payload
	The size field denotes the length of udp data.
	The checksum field is exactly 16 bits in length.
*/	
struct udp_payload{
	unsigned short size;
	unsigned short checksum;
	unsigned char *data;
};


/**
	An additional packet header to simulate updlite performance. Instead of using data field in hdr_cmn we decided to introduce hdr_udplite to store
	data and checksums.

	hdr_udplite offers backwards compatibility with udp as well, the ver field must be set to 1.

	The header_checksum field is is common to both udp and udplite. Network implementations of udp use a single checksum field for headers and data, and
	a seperate IP checksum. However due to the way packets are created and transmitted in ns2 (as pointers) without any clear demarcation between the IP 
	protocol and udp protocol, we maintain seperate checksums for the headers and the payloads.
*/
struct hdr_udplite {

	//Common to both udplite and udp
	unsigned char ver;                //0 for udplite and 1 for udp
	unsigned short header_checksum;
	unsigned char header[16];          //8 byte udp header + 4 byte dest address + 4 byte source address 

	//only for udplite
	unsigned short nunits;					//Denotes the number of payload segments
	struct udplite_payload *udplite_data;	//Pointer to the array of segments

	//only for udp
	struct udp_payload *udp_data;			//Pointer to the payload

	// necessary for access
	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_udplite* access(const Packet* p) {
		return (hdr_udplite*) p->access(offset_);
	}
};

#endif