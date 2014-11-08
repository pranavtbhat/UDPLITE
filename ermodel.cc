#include "config.h"
#include "udplitepacket.h"
#include <stdio.h>
#include <ctype.h>
#include "packet.h"
#include "flags.h"
#include "mcast_ctrl.h"
#include "ermodel.h"
#include "srm-headers.h"	
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
		//Its a udplite packet

		for(i=0;i < pch->nunits; i++){

			for(j=0;j< pch->udplite_data[i].size ;j++){

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
				if(randvar <rate_){
					(pch->udp_data)->data[i] ^= 1 << j;
				}
			}
		}
	}

}

