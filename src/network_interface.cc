#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
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
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  // Your code here.
  auto next_hop_ip = next_hop.ipv4_numeric();
  auto arp_iter = arp_table_.find( next_hop_ip );
  if ( arp_iter == arp_table_.end() ) {
    if ( pending_arp_res_.find( next_hop_ip ) == pending_arp_res_.end() ) {
      ARPMessage arp_msg = { .opcode = ARPMessage::OPCODE_REQUEST,
                             .sender_ethernet_address = ethernet_address_,
                             .sender_ip_address = ip_address_.ipv4_numeric(),
                             .target_ethernet_address = {},
                             .target_ip_address = next_hop_ip };
      EthernetFrame frame;
      frame.header = { ETHERNET_BROADCAST, ethernet_address_, EthernetHeader::TYPE_ARP };
      frame.payload = serialize( arp_msg );
      transmit( frame );
      pending_arp_res_[next_hop_ip] = res_ttl_;
    }
    pending_ip_datagrams_.push_back( { next_hop, dgram } );
  } else {
    EthernetFrame frame;
    frame.header = { arp_iter->second.addr, ethernet_address_, EthernetHeader::TYPE_IPv4 };
    frame.payload = serialize( dgram );
    transmit( frame );
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // Your code here.
  if ( frame.header.dst != ETHERNET_BROADCAST && frame.header.dst != ethernet_address_ )
    return;
  switch ( frame.header.type ) {
    case EthernetHeader::TYPE_IPv4: {
      IPv4Datagram dgram;
      if ( !parse( dgram, frame.payload ) )
        return;
      datagrams_received_.push( std::move( dgram ) );
      break;
    }
    case EthernetHeader::TYPE_ARP: {
      ARPMessage arp_msg;
      if ( !parse( arp_msg, frame.payload ) )
        return;
      if ( arp_msg.opcode == ARPMessage::OPCODE_REQUEST
           && arp_msg.target_ip_address == ip_address_.ipv4_numeric() ) {
        transmit( EthernetFrame {
          .header = { arp_msg.sender_ethernet_address, ethernet_address_, EthernetHeader::TYPE_ARP },
          .payload = serialize( ARPMessage {
            .opcode = ARPMessage::OPCODE_REPLY,
            .sender_ethernet_address = ethernet_address_,
            .sender_ip_address = ip_address_.ipv4_numeric(),
            .target_ethernet_address = arp_msg.sender_ethernet_address,
            .target_ip_address = arp_msg.sender_ip_address,
          } ) } );
      }
      auto src = arp_msg.sender_ethernet_address;
      auto src_ip = arp_msg.sender_ip_address;
      arp_table_[arp_msg.sender_ip_address] = { src, init_ttl_ };
      for ( auto iter = pending_ip_datagrams_.begin(); iter != pending_ip_datagrams_.end(); ) {
        if ( iter->first.ipv4_numeric() == src_ip ) {
          transmit( EthernetFrame { .header = { src, ethernet_address_, EthernetHeader::TYPE_IPv4 },
                                    .payload = serialize( iter->second ) } );
          iter = pending_ip_datagrams_.erase( iter );
        }
      }
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  for ( auto it = arp_table_.begin(); it != arp_table_.end(); ) {
    if ( it->second.ttl <= ms_since_last_tick ) {
      it = arp_table_.erase( it );
    } else {
      it->second.ttl -= ms_since_last_tick;
      it = std::next( it );
    }
  }
  for ( auto it = pending_arp_res_.begin(); it != pending_arp_res_.end(); ) {
    if ( it->second <= ms_since_last_tick ) {
      for ( auto iter = pending_ip_datagrams_.begin(); iter != pending_ip_datagrams_.end(); )
        if ( it->first == iter->first.ipv4_numeric() )
          iter = pending_ip_datagrams_.erase( iter );
      it = pending_arp_res_.erase( it );
    } else {
      it->second -= ms_since_last_tick;
      it = std::next( it );
    }
  }
}
