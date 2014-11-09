/**
	Ns-2 Udplite Agent.
	udplite.cc

	Based on udp.cc developed by the Xerox Corporation. 
	
	Udplite is a variant of the standard User Datagram Protocol that divides the payload data into segments of fixed size. Each segment maintains its own
	checksum field. When the datagram is received by the network layer, it evaluates the checksum on individual segments. Only segments failing the checksum 
	discarded, the entire packet need not be discarded because of a handful of errors. Depending on the segment size used, udplite can show significantly 
	better performance in error prone networks. 


	The udplite agent provides significantly better bit error resilience than the standard udp agent.
	Refer udplitepacket.h for the packet format used. 

	The tcl parameters for the agent are:
		1. packetSize_ : size of the udplite payload.
		2. pkts_sent_  : number of packets sent by the agent.
		3. pkts_recv_  : a real number depicting the summation of the fraction of uncorrupted data in each packet.
		4. udp_mode_   : Udplite provides backwards compatibility with udp. Set this field to 1 to use udp-mode.
		5. ratio_      : Size of the udplite segment Eg. If this field is 30, then there is one checksum field 
						 for every 30 bytes of payload data. Increasing the value of ratio_ saves on redundant packet data, but reduces error resilience.

	
	@author Pranav Bhat and Rohit Varkey
	@version 1.0
*/




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


/**
	TCL BINDING 
*/
UdpLiteAgent::UdpLiteAgent() : Agent(PT_UDPLite), seqno_(-1)
{
	bind("packetSize_", &size_);
	bind("pkts_recv_", &pkts_recv_);
	bind("pkts_sent_", &pkts_sent_);
	bind("udp_mode_", &udp_mode_);
	bind("ratio_", &ratio_);
}

/**
	TCL BINDING 
*/
UdpLiteAgent::UdpLiteAgent(packet_t type) : Agent(type)
{
	bind("packetSize_", &size_);
	bind("pkts_recv_", &pkts_recv_);
	bind("pkts_sent_", &pkts_sent_);
	bind("udp_mode_", &udp_mode_);
	bind("ratio_", &ratio_);
}

/**
	Function to create and sent udp packets.
	Seperate headers are used for udplite and udp.

	Udplite can be used like a udp agent. It retains the ability to send and receive AppData. It uses a separate packet header to allocated data and checksums
	to simulate udp and udplite performance in error prone networks.

	Malloc is used instead of calloc to facilitate checksum computation on random data.
*/
void UdpLiteAgent::sendmsg(int nbytes, AppData* data, const char* flags)
{
	Packet *p;
	int n,i;
	int x;

	assert (size_ > 0);
	
	//If default is not provided use PAYLOAD_DATA_SIZE
	if(ratio_<= 0){
		ratio_ = PAYLOAD_DATA_SIZE;
		printf("ratio wasn't set\n");
	}

	//Ensure that ratio_ is even
	if(ratio_%2 != 0 ){
		printf("ratio was odd\n");
		ratio_++;
	}

	//n is the number of fully filled packets required
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

		//Set IP datagram headers
		p = allocpkt();
		hdr_cmn::access(p)->size() = size_;
		hdr_rtp* rh = hdr_rtp::access(p);
		rh->flags() = 0; 
		rh->seqno() = ++seqno_;
		hdr_cmn::access(p)->timestamp() = (u_int32_t)(SAMPLERATE*local_time);
		// add "beginning of talkspurt" labels (tcl/ex/test-rcvr.tcl)
		if (flags && (0 ==strcmp(flags, "NEW_BURST")))
			rh->flags() |= RTP_M;
		//set application data
		p->setdata(data);

		//Now process the udplite headers

		if(udp_mode_ == 0){

				//Udplite packet
				hdr_udplite *pch = hdr_udplite::access(p);

				pch->ver = 0; 																						//set version to 0 for udplite
				pch->nunits = ceil((float)size_/ratio_); 															//Compute the number of udplite_segments
				pch->udplite_data = (struct udplite_payload*)malloc(sizeof(struct udplite_payload)*pch->nunits);	//Allocate memory to all udplite segments

				//compute header checksum
				pch->header_checksum = compute_checksum(pch->header,UDPLITE_HEADER_SIZE);

				//set data and compute checksums for individual udplite_payloads
				for(i=0;i<hdr_udplite::access(p)->nunits;i++){
					udplite_payload *unit = &pch->udplite_data[i];

					unit->size = ratio_;
					unit->data = (unsigned char *)malloc(sizeof(unsigned char)*ratio_);								//Fill up segment with data
					unit->checksum = compute_checksum(unit->data, ratio_);											//Compute segment checksum
				}
		}
		else if(udp_mode_ == 1){
			//Build UDP packet header
			hdr_udplite *pch = hdr_udplite::access(p);

			pch->ver = 1; 																							//This is not a udplite packet
			pch->udp_data = (struct udp_payload*)malloc(sizeof(struct udp_payload));								//Allocate memory to udp payload
			(pch->udp_data)->data = (unsigned char *)malloc(sizeof(unsigned char)*size_);							//Fill up payload with data
			(pch->udp_data)->size = size_;

			//compute header checksum
			pch->header_checksum = compute_checksum(pch->header,UDPLITE_HEADER_SIZE);								

			//compute udp_payload checksum
			(pch->udp_data)->checksum = compute_checksum((pch->udp_data)->data , size_ );
		}

		//Increment number of packets sent
		pkts_sent_++;
		//Send packet to connector
		target_->recv(p);
	}

	//Process the left over data
	n = nbytes % size_;

	if (n > 0) {
		//Process IP headers
		p = allocpkt();
		hdr_cmn::access(p)->size() = n;
		hdr_rtp* rh = hdr_rtp::access(p);
		rh->flags() = 0;
		rh->seqno() = ++seqno_;
		hdr_cmn::access(p)->timestamp() = (u_int32_t)(SAMPLERATE*local_time);
		// add "beginning of talkspurt" labels (tcl/ex/test-rcvr.tcl)
		if (flags && (0 == strcmp(flags, "NEW_BURST")))
			rh->flags() |= RTP_M;
		//Set AppData
		p->setdata(data);

		if(udp_mode_ == 0){
			//Build UDPLite packet header
				hdr_udplite *pch = hdr_udplite::access(p);

				pch->ver = 0; 																								//This is a udplite packet
				pch->nunits = ceil((float)n/ratio_);																		//Compute the number of udplite_segments
				pch->udplite_data = (struct udplite_payload *)malloc(sizeof(struct udplite_payload)*pch->nunits);			//Allocate memory to all udplite segments

				//compute header checksum
				pch->header_checksum = compute_checksum(pch->header,UDPLITE_HEADER_SIZE);

				//set data and compute checksums for individual udplite_payloads
				for(i=0;i<hdr_udplite::access(p)->nunits;i++){
					udplite_payload *unit = &pch->udplite_data[i];

					unit->size = ratio_;
					unit->data = (unsigned char *)malloc(sizeof(unsigned char)*ratio_);										//Fill up segment with data
					unit->checksum = compute_checksum(unit->data, ratio_);													//Compute segment checksum
				}
		}
		else if(udp_mode_ == 1){
			//Build UDP packet header
			hdr_udplite *pch = hdr_udplite::access(p);

			pch->ver = 1; //This is not a udplite packet
			pch->udp_data = (struct udp_payload*)malloc(sizeof(struct udp_payload));										//Allocate memory to udp payload
			(pch->udp_data)->data = (unsigned char *)malloc(sizeof(unsigned char)*n);										//Fill up payload with data
			(pch->udp_data)->size = n;

			//compute header checksum
			pch->header_checksum = compute_checksum(pch->header,UDPLITE_HEADER_SIZE);

			//compute udp_payload checksum
			(pch->udp_data)->checksum = compute_checksum((pch->udp_data)->data , n );

		}
		//Increment number of packets sent
		pkts_sent_++;
		//Send packet to connector
		target_->recv(p);
	}
	idle();
}

/**
	Function to receive udplite packets. Decides how much of the packet is useful and computes pkts_recv_ accordingly.
	Drops the packet if every segment is corrupted. Otherwise sends the entire AppData to application layer.

	In udp mode, packet is dropped if even part of it is corrupted.

*/
void UdpLiteAgent::recv(Packet* pkt, Handler*)
{

	if(udp_mode_ == 0){
		//It's a udplite packet
		//Find out how much of the packet is actually usable
		int i;
		hdr_udplite *pch = hdr_udplite::access(pkt);
		int count = pch->nunits;

		for(i=0;i<pch->nunits;i++){
			if(pch->udplite_data[i].checksum != compute_checksum(pch->udplite_data[i].data, pch->udplite_data[i].size )){
				count--;
			}
		}

		if(count == 0){
			drop(pkt);
			return;
		}
		else{
			pkts_recv_ += ((float)count/pch->nunits);
		}	

		
	}
	else if(udp_mode_ == 1){
		//Its a udp packet
		//Find out if the packet is useful or not 
		hdr_udplite *pch = hdr_udplite::access(pkt);
		
		if((pch->udp_data)->checksum == compute_checksum((pch->udp_data)->data , (pch->udp_data)->size ))
			pkts_recv_ +=1;
		else{
			drop(pkt);
			return;
		}
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

unsigned short UdpLiteAgent::compute_checksum(unsigned char array[],short length)
{
	//The checksum is first calculated as a 32 bit number
	//Then it is condensed to a 16 bit field

	unsigned long checksum=0;
	unsigned short word; 
	int i;


	//First make 16 bit words out of every pair of adjacent bytes and add them up
	for(i=0;i<length;i=i+2){

		if(i+1 == length) //If length is odd, add padding
			word = ((array[i]<<8) & 0xFF00) + 0x00FF;
		else
			word = ((array[i]<<8) & 0xFF00) + (array[i+1]&0x00FF);
		checksum+=word;
	}

	//condense checksum to 16 bits
	while(checksum >> 16){
		checksum = (checksum & 0xFFFF) + (checksum >> 16);
	}

	//Find one's complement of checksum
	checksum = ~checksum;
	return (unsigned short)checksum;
}