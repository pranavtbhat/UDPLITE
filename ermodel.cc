#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/queue/errmodel.cc,v 1.84 2010/03/08 05:54:53 tom_henderson Exp $ (UCB)";
#endif


#include "config.h"
#include "udplitepacket.h"
#include <stdio.h>
#include <ctype.h>
#include "packet.h"
#include "flags.h"
#include "mcast_ctrl.h"
#include "ermodel.h"
#include "srm-headers.h"		// to get the hdr_srm structure
#include "classifier.h"
#include <math.h>

static class ErModelClass : public TclClass {
public:
	ErModelClass() : TclClass("ErModel") {}
	TclObject* create(int, const char*const*) {
		return (new ErModel);
	}
} class_ermodel;

ErModel::ErModel()
{
	bind("rate_", &rate_);	
}

int ErModel::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	return Connector::command(argc, argv);
}

void ErModel::recv(Packet* p, Handler* h)
{
	int i;
	int count=0;
	hdr_cmn *ch = hdr_cmn::access(p);
	hdr_udplite *pch = hdr_udplite::access(p);

	corrupt(p);

	if(pch->header_checksum != compute_checksum(pch->header,UDPLITE_HEADER_SIZE)){
		//printf("Header Problem-> %d %d\n",pch->header_checksum,compute_checksum(pch->udplite_header,UDPLITE_HEADER_SIZE));
		drop_->recv(p);
		return;
	}
	

	/*
	for(i=0;i<pch->nunits;i++){
		if(pch->payload[i].checksum != compute_checksum(pch->payload[i].data,PAYLOAD_DATA_SIZE)){
			count++;
		}
	}
	
	if(count>0){
		printf("Data Problem-> %d\n",count);
		drop_->recv(p);
		return;
	}
	*/

	if (target_) {
	       	target_->recv(p, h);
	}
}

void ErModel::corrupt(Packet* p)
{
	double randvar;
	int i;
	int j;
	int k;

	//First handle the header
	hdr_udplite *pch = hdr_udplite::access(p);
	for(i=0;i<UDPLITE_HEADER_SIZE;i++){
		for(j=0;j<8;j++){
			randvar = Random::uniform();
			if(randvar < rate_){
				pch->header[i] ^= 1 << j;
			}
		}  
	}

	//next check if the packet is udplite or udp

	if(pch->ver == 0){
		for(i=0;i < pch->nunits;i++){
			for(j=0;j<PAYLOAD_DATA_SIZE;j++){
				for(k=0;k<8;k++){
					randvar = Random::uniform();
					if(randvar <rate_)
						pch->udplite_data[i].data[j] ^= 1 << k;
				}
			}
		}
	}

	else if(pch->ver == 1){

		for(i=0;i< (pch->udp_data)->size ; i++){
			for(j=0;j<8;j++){
				randvar = Random::uniform();
				if(randvar <rate_)
					(pch->udp_data)->data[i] ^= 1 << j;
			}
		}
	}

}

unsigned char ErModel::compute_checksum(unsigned char array[],short length){
	unsigned char checksum=0;
	short i;

	for(i=0;i<length;i++){
		checksum+=array[i];
	}

	return checksum;
}