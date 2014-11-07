#include "udplite.h"
#include "udplitepacket.h"
#include "rtp.h"
#include "random.h"
#include "address.h"
#include "ip.h"


static class UdpLiteAgentClass : public TclClass {
public:
	UdpLiteAgentClass() : TclClass("Agent/UDPLite") {}
	TclObject* create(int, const char*const*) {
		return (new UdpLiteAgent());
	}
} class_udplite_agent;

UdpLiteAgent::UdpLiteAgent() : Agent(PT_UDPLite), seqno_(-1)
{
	bind("packetSize_", &size_);
	bind("pkts_recv_", &pkts_recv_);
	bind("pkts_sent_", &pkts_sent_);
	bind("udp_mode_", &udp_mode_);
}

UdpLiteAgent::UdpLiteAgent(packet_t type) : Agent(type)
{
	bind("packetSize_", &size_);
	bind("pkts_recv_", &pkts_recv_);
	bind("pkts_sent_", &pkts_sent_);
	bind("udp_mode_", &udp_mode_);
}

// put in timestamp and sequence number, even though UDP doesn't usually 
// have one.
void UdpLiteAgent::sendmsg(int nbytes, AppData* data, const char* flags)
{
	Packet *p;
	int n,i;

	assert (size_ > 0);

	n = nbytes / size_;
	if (nbytes == -1) {
		printf("Error:  sendmsg() for UDP should not be -1\n");
		return;
	}	

	// If they are sending data, then it must fit within a single packet.
	if (data && nbytes > size_) {
		printf("Error: data greater than maximum UDP packet size\n");
		return;
	}

	double local_time = Scheduler::instance().clock();
	while (n-- > 0) {
		p = allocpkt();
		hdr_cmn::access(p)->size() = size_;
		hdr_rtp* rh = hdr_rtp::access(p);
		rh->flags() = 0; 
		rh->seqno() = ++seqno_;
		hdr_cmn::access(p)->timestamp() = 
		    (u_int32_t)(SAMPLERATE*local_time);
		// add "beginning of talkspurt" labels (tcl/ex/test-rcvr.tcl)
		if (flags && (0 ==strcmp(flags, "NEW_BURST")))
			rh->flags() |= RTP_M;
		p->setdata(data);

		if(udp_mode_ == 0){
			//Build UDPLite packet header
				hdr_udplite *pch = hdr_udplite::access(p);

				pch->ver = 0; //This is a udplite packet
				pch->nunits = ceil((float)size_/PAYLOAD_DATA_SIZE);
				pch->udplite_data = (struct udplite_payload*)malloc(sizeof(struct udplite_payload)*pch->nunits);

				//compute header checksum
				pch->header_checksum = compute_checksum(pch->header,UDPLITE_HEADER_SIZE);

				//compute individual udplite_payload checksums
				for(i=0;i<hdr_udplite::access(p)->nunits;i++){
					udplite_payload *unit = &pch->udplite_data[i];
					unit->checksum = compute_checksum(unit->data, PAYLOAD_DATA_SIZE);
				}
		}
		else if(udp_mode_ == 1){
			//Build UDP packet header
			hdr_udplite *pch = hdr_udplite::access(p);

			pch->ver = 1; //This is not a udplite packet
			pch->udp_data = (struct udp_payload*)malloc(sizeof(struct udp_payload));
			(pch->udp_data)->data = (unsigned char *)malloc(sizeof(unsigned char)*size_);
			(pch->udp_data)->size = size_;

			//compute header checksum
			pch->header_checksum = compute_checksum(pch->header,UDPLITE_HEADER_SIZE);

			//compute udp_payload checksum
			(pch->udp_data)->checksum = compute_checksum((pch->udp_data)->data , (pch->udp_data)->size );

			
		}

		pkts_sent_++;
		target_->recv(p);
	}
	n = nbytes % size_;
	if (n > 0) {
		p = allocpkt();
		hdr_cmn::access(p)->size() = n;
		hdr_rtp* rh = hdr_rtp::access(p);
		rh->flags() = 0;
		rh->seqno() = ++seqno_;
		hdr_cmn::access(p)->timestamp() = 
		    (u_int32_t)(SAMPLERATE*local_time);
		// add "beginning of talkspurt" labels (tcl/ex/test-rcvr.tcl)
		if (flags && (0 == strcmp(flags, "NEW_BURST")))
			rh->flags() |= RTP_M;
		p->setdata(data);

		if(udp_mode_ == 0){
			//Build UDPLite packet header
				hdr_udplite *pch = hdr_udplite::access(p);

				pch->ver = 0; //This is a udplite packet
				pch->nunits = ceil((float)n/PAYLOAD_DATA_SIZE);
				pch->udplite_data = (struct udplite_payload*)malloc(sizeof(struct udplite_payload)*pch->nunits);

				//compute header checksum
				pch->header_checksum = compute_checksum(pch->header,UDPLITE_HEADER_SIZE);

				//compute individual udplite_payload checksums
				for(i=0;i<hdr_udplite::access(p)->nunits;i++){
					udplite_payload *unit = &pch->udplite_data[i];
					unit->checksum = compute_checksum(unit->data, PAYLOAD_DATA_SIZE);
				}
		}
		else if(udp_mode_ == 1){
			//Build UDP packet header
			hdr_udplite *pch = hdr_udplite::access(p);

			pch->ver = 1; //This is not a udplite packet
			pch->udp_data = (struct udp_payload*)malloc(sizeof(struct udp_payload));
			(pch->udp_data)->data = (unsigned char *)malloc(sizeof(unsigned char)*n);
			(pch->udp_data)->size = n;

			//compute header checksum
			pch->header_checksum = compute_checksum(pch->header,UDPLITE_HEADER_SIZE);

			//compute udp_payload checksum
			(pch->udp_data)->checksum = compute_checksum((pch->udp_data)->data , (pch->udp_data)->size );
		}
		
		pkts_sent_++;
		target_->recv(p);
	}
	idle();
}
void UdpLiteAgent::recv(Packet* pkt, Handler*)
{

	if(udp_mode_ == 0){
		//Find out how much of the packet is actually usable
		int i;
		hdr_udplite *pch = hdr_udplite::access(pkt);
		int count = pch->nunits;

		for(i=0;i<pch->nunits;i++){
			if(pch->udplite_data[i].checksum != compute_checksum(pch->udplite_data[i].data,PAYLOAD_DATA_SIZE)){
				count--;
			}
		}	

		pkts_recv_ += ((float)count/pch->nunits);
	}
	else if(udp_mode_ == 1){
		//Find out if the packet is useful or not 
		hdr_udplite *pch = hdr_udplite::access(pkt);

		if((pch->udp_data)->checksum == compute_checksum((pch->udp_data)->data , (pch->udp_data)->size ))
			pkts_recv_ +=1;
	}

	//Standard udp implementation once the headers are sorted out
	if (app_ ) {
		// If an application is attached, pass the data to the app
		hdr_cmn* h = hdr_cmn::access(pkt);
		app_->process_data(h->size(), pkt->userdata());
	} else if (pkt->userdata() && pkt->userdata()->type() == PACKET_DATA) {
		// otherwise if it's just PacketData, pass it to Tcl
		//
		// Note that a Tcl procedure Agent/Udp recv {from data}
		// needs to be defined.  For example,
		//
		// Agent/Udp instproc recv {from data} {puts data}

		PacketData* data = (PacketData*)pkt->userdata();

		hdr_ip* iph = hdr_ip::access(pkt);
                Tcl& tcl = Tcl::instance();
		tcl.evalf("%s process_data %d {%s}", name(),
		          iph->src_.addr_ >> Address::instance().NodeShift_[1],
			  data->data());
	}
	Packet::free(pkt);
}


int UdpLiteAgent::command(int argc, const char*const* argv)
{
	if (argc == 4) {
		if (strcmp(argv[1], "send") == 0) {
			PacketData* data = new PacketData(1 + strlen(argv[3]));
			strcpy((char*)data->data(), argv[3]);
			sendmsg(atoi(argv[2]), data);
			return (TCL_OK);
		}
	} else if (argc == 5) {
		if (strcmp(argv[1], "sendmsg") == 0) {
			PacketData* data = new PacketData(1 + strlen(argv[3]));
			strcpy((char*)data->data(), argv[3]);
			sendmsg(atoi(argv[2]), data, argv[4]);
			return (TCL_OK);
		}
	}
	return (Agent::command(argc, argv));
}

unsigned char UdpLiteAgent::compute_checksum(unsigned char array[],short length)
{
	unsigned char checksum=0;
	short i;

	for(i=0;i<length;i++){
		checksum+=array[i];
	}

	return checksum;
}