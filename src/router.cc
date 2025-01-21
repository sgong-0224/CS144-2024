#include "router.hh"

#include <iostream>
#include <limits>

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

  // Your code here.
  for(auto& entry:route_table_){
    if(entry.route_prefix==route_prefix && entry.prefix_length==prefix_length){
        entry.next_hop = next_hop;
        entry.interface_num = interface_num;
        return;
    }
  }
  route_table_.push_back(route_info{route_prefix,interface_num,next_hop,prefix_length});
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  // Your code here.
  auto match = []( uint32_t ip, uint32_t route_prefix, uint8_t prefix_length ){
    if ( prefix_length == 0 )
        return true;
    uint32_t mask = 0xffffffff << ( 32 - prefix_length );
    return ( ip & mask ) == ( route_prefix & mask );
  };
  for ( auto& interface_ptr : _interfaces ) {
    auto& dgrams = interface_ptr->datagrams_received();
    while ( !dgrams.empty() ) {
      auto& dgram = dgrams.front();
      if ( dgram.header.ttl <= 1 ) {
        dgrams.pop();
        continue;
      }
      dgram.header.ttl -= 1;
      dgram.header.compute_checksum();
      auto dst_ip = dgram.header.dst;
      int match_length = -1;
      uint32_t next_hop = 0;
      size_t interface_num = 0;
      for ( auto const& entry : route_table_ ) {
        if ( match( dst_ip, entry.route_prefix, entry.prefix_length ) 
        && entry.prefix_length >= match_length ) {
          match_length = entry.prefix_length;
          interface_num = entry.interface_num;
          next_hop = entry.next_hop.has_value()? entry.next_hop.value().ipv4_numeric() : dst_ip;
        }
      }
      if ( match_length != -1 )
        interface(interface_num)->send_datagram( dgram, Address::from_ipv4_numeric( next_hop ) );
      dgrams.pop();
    }
  }
}
