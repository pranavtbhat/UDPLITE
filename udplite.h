#ifndef ns_udplite_h
#define ns_udplite_h

#include "agent.h"
#include "trafgen.h"
#include "packet.h"

//"rtp timestamp" needs the samplerate
#define SAMPLERATE 8000
#define RTP_M 0x0080 // marker for significant events

class UdpLiteAgent : public Agent {
public:
	UdpLiteAgent();
	UdpLiteAgent(packet_t);
	virtual void sendmsg(int nbytes, const char *flags = 0)
	{
		sendmsg(nbytes, NULL, flags);
	}
	virtual void sendmsg(int nbytes, AppData* data, const char *flags = 0);
	virtual void recv(Packet* pkt, Handler*);
	virtual int command(int argc, const char*const* argv);
	virtual unsigned short compute_checksum(unsigned char[],short length);
	double pkts_recv_;
	int pkts_sent_;
	int udp_mode_;
	int ratio_;
protected:
	int seqno_;
};

#endif
