#include <iostream>


#include "arp_message.hh"
#include "debug.hh"
#include "ethernet_frame.hh"
#include "exception.hh"
#include "helpers.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
	if ( ip2mac_.count(next_hop) == 0 ) { // 表中无映射

		datagrams_waiting_arp_[next_hop].push(dgram);

		EthernetFrame msg_arp;

		/* */

		if (arp_sent_.count(next_hop) == 0) {
			transmit(msg_arp);
			
			arp_sent_[next_hop] = 5000; // 5000 ms
		}
	}
	else { // 有映射
		EthernetFrame msg_ipv4;

		/* 构造 msg */

		transmit(msg_ipv4);
	}
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( EthernetFrame frame )
{
  debug( "unimplemented recv_frame called" );
  (void)frame;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
	for (auto &[ip, tick_time] : arp_sent_) {
		tick_time -= ms_since_last_tick;
		if (tick_time < 0) {
			arp_sent_.erase(ip);
		}
	}
}
