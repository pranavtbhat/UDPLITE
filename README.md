This is an implementation of UDPLITE. To test udplite's resilience to high bit error rates, an error model ermodel is also implemented. 

The udplite packet is implemeneted as a packet header( we neglect the application data provision for packets, as this is too difficult to handle) 
   The packet header file is udplitepacket.h
   The packet is implemented in udplitepacket.cc

The udplite protocol is defined in udplite.h and implemented in udplite.cc. The existing udp framework was reused wherever possible. 
The protocol also has a udpmode option to revert to udp.

There are two separate checksums, one for the header and one for the data.

UdpLiteAgent implements the Agent class.

Ermodel implemets the connector class. Ermodel drops the packet if the header checksum fails.
