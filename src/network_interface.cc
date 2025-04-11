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

EthernetFrame NetworkInterface::make_arp_eframe(const uint16_t &opcode,
									   const EthernetAddress &src_ethernet_address,
									   const Address &src_ip_address,
									   const EthernetAddress &dst_ethernet_address,
									   const Address &dst_ip_address) 
{

			EthernetFrame msg_arp;
				// 构造 ARP 请求

				ARPMessage arp_request;
			{

				arp_request.opcode = opcode;

				arp_request.sender_ip_address = src_ip_address.ipv4_numeric();
				arp_request.sender_ethernet_address = src_ethernet_address;

				arp_request.target_ip_address = dst_ip_address.ipv4_numeric();
				arp_request.target_ethernet_address = dst_ethernet_address;

			}

				Serializer serializer;
				arp_request.serialize(serializer);

			if (opcode == ARPMessage::OPCODE_REQUEST) {
				msg_arp.header.dst = ETHERNET_BROADCAST;
			}
			else {
				msg_arp.header.dst = dst_ethernet_address;
			}

			msg_arp.header.type = EthernetHeader::TYPE_ARP;
			msg_arp.header.src = src_ethernet_address;
			msg_arp.payload = serializer.finish();

			return msg_arp;

}

EthernetFrame NetworkInterface::make_ip_eframe( const InternetDatagram &dgram, const EthernetAddress &dst_ethernet_address) {

		Serializer serializer;
		dgram.serialize(serializer);

		EthernetFrame msg_ipv4;

		/* 构造 msg */
		msg_ipv4.header.type = EthernetHeader::TYPE_IPv4;
		msg_ipv4.header.src = ethernet_address_;
		msg_ipv4.header.dst = dst_ethernet_address;
		msg_ipv4.payload = serializer.finish();
		
		return msg_ipv4;
}


//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
	uint32_t next_ip = next_hop.ipv4_numeric();

	if ( ip2mac_.count(next_ip) == 0 ) { // 表中无映射

		datagrams_waiting_arp_[next_ip].push({ dgram, _local_clock });

		if (arp_sent_.count(next_ip) == 0) {

			EthernetFrame eframe = make_arp_eframe(ARPMessage::OPCODE_REQUEST, 
													ethernet_address_,
													ip_address_,
													{  },
													next_hop);

			transmit(eframe);
			
			arp_sent_[next_ip] = _local_clock; // 5000 ms
		}
	}
	else { // 有映射

		EthernetFrame eframe = make_ip_eframe(dgram, ip2mac_[next_ip].ethernet_address);

		transmit(eframe);
	}
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( EthernetFrame frame )
{
	if (frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST ) return;

	if (frame.header.type == EthernetHeader::TYPE_IPv4) { // 收到 IPv4
		
		Parser parser(frame.payload);

		InternetDatagram dgram;
		dgram.parse(parser);

		if (parser.has_error()) return;

		datagrams_received_.push(dgram);
	}
	
	else if (frame.header.type == EthernetHeader::TYPE_ARP) { // 收到 ARP 信息

		Parser parser(frame.payload);

		ARPMessage arp_msg;
		arp_msg.parse(parser);

		if (parser.has_error()) return;

		uint32_t src_ip = arp_msg.sender_ip_address;
		EthernetAddress src_mac = arp_msg.sender_ethernet_address;

		ip2mac_[src_ip] = {src_mac, _local_clock}; // 30000 ms

		if (arp_msg.opcode == ARPMessage::OPCODE_REQUEST
				&& arp_msg.target_ip_address == ip_address_.ipv4_numeric()) { // 发给本机的 ARP 请求

			EthernetFrame eframe = make_arp_eframe(ARPMessage::OPCODE_REPLY,
												   ethernet_address_,
												   ip_address_,
												   src_mac,
												   Address::from_ipv4_numeric(src_ip));

			transmit(eframe);

		}
		
		else if (arp_msg.opcode == ARPMessage::OPCODE_REPLY) {
			if (datagrams_waiting_arp_.count(src_ip) > 0) {

				auto& dgram_queue = datagrams_waiting_arp_[src_ip];

				while (dgram_queue.size() > 0) {
					size_t tick_time = dgram_queue.front().tick_time;

					InternetDatagram dgram = dgram_queue.front().dgram;

					dgram_queue.pop();

					if (tick_time <= _local_clock && tick_time + 5000 <= _local_clock) {
						continue;
					}


					EthernetFrame eframe = make_ip_eframe(dgram, ip2mac_[src_ip].ethernet_address);

					transmit(eframe);


				}

				datagrams_waiting_arp_.erase(src_ip);
			}
		}

	}
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
	_local_clock += ms_since_last_tick;

	for (auto &[ip, tick_time] : arp_sent_) {
		if (tick_time <= _local_clock && tick_time + 5000 <= _local_clock) {
			arp_sent_.erase(ip);
			break;
		}
	}

	for (auto &[ip, pa] : ip2mac_) {
		if (pa.tick_time <= _local_clock && pa.tick_time + 30000 <= _local_clock) {
			ip2mac_.erase(ip);
			break;
		}
	}
}
