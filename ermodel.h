#ifndef ns_ermodel_h
#define ns_ermodel_h

#include "connector.h"
#include "timer-handler.h"
#include "ranvar.h"
#include "packet.h"
#include "basetrace.h"

class ErModel : public Connector {
public:
	ErModel();
	virtual void recv(Packet*, Handler*);
	virtual void corrupt(Packet*);
	
protected:
	virtual int command(int argc, const char*const* argv);
	double rate_;		// uniform error rate in pkt or byte
};

#endif 

