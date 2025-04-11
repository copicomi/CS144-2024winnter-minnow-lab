#include "router.hh"
#include "debug.hh"

#include <iostream>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
   cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  	route_list[prefix_length][route_prefix] = { next_hop, interface_num };

	return;
}

bool Router::get_route(const InternetDatagram& dgram, Address& out_address, size_t& interface_num) {
	uint32_t dst_addr = dgram.header.dst;
	uint32_t mask = (1u << 31) - 1;
	mask = (mask << 1) | 1;
	for (int pre_len = 32, i = 1; pre_len >= 0; pre_len --, mask -= i, i <<= 1) {
		uint32_t ip_prefix = dst_addr & mask;
		if (route_list[pre_len].count(ip_prefix) > 0) {
			auto pa = route_list[pre_len][ip_prefix];
			interface_num = pa.interface_num;
			if (pa.next_hop.has_value() == true) {
				out_address = pa.next_hop.value();
			}
			else {
				out_address = Address::from_ipv4_numeric(dst_addr);
			}
			return true;
		}

	}	
	return false;
}

void Router::route_datagram(InternetDatagram& dgram) {

	size_t interface_num {};
	Address out_address = Address::from_ipv4_numeric(0);

	uint8_t& ttl = dgram.header.ttl;


	if (ttl > 0 && -- ttl > 0 && get_route(dgram, out_address, interface_num ) == true) {
		
		dgram.header.compute_checksum();
		interface(interface_num)->send_datagram(dgram, out_address);

	}
	
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
	for (size_t index = 0; index < interfaces_.size(); index ++) {

		auto& dgrams_ = interface(index)->datagrams_received();
		
		while (dgrams_.size() > 0) {

			route_datagram(dgrams_.front());

			dgrams_.pop();

		}
	}
}
