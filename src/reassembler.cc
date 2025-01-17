#include "reassembler.hh"
#include <cassert>
#include <cstdint>
#include <limits>
#include <utility>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.
  auto first_unassembled = output_.writer().bytes_pushed();
  auto first_unaccept = first_unassembled + _capacity;
  if ( first_index >= first_unaccept || first_index + data.size() < first_unassembled )
    return;
  auto begin_idx = std::max( first_index, first_unassembled );
  auto max_data_idx = first_index + data.size();
  auto end_idx = std::min( first_unaccept, max_data_idx );
  // push into reassembler
  for ( auto i = begin_idx; i < end_idx; ++i ) {
    if ( !_bytes_flag[i - first_unassembled] ) {
      _bytes_flag[i - first_unassembled] = true;
      _bytes_buffer[i - first_unassembled] = data[i - first_index];
      _unassembled_bytes += 1;
    }
  }
  std::string pending_str = "";
  while ( _bytes_flag.front() ) {
    pending_str += std::move( _bytes_buffer.front() );
    _bytes_buffer.pop_front();
    _bytes_flag.pop_front();
    _bytes_buffer.push_back( 0 );
    _bytes_flag.push_back( false );
  }
  if ( !pending_str.empty() ) {
    output_.writer().push( pending_str );
    _unassembled_bytes -= pending_str.size();
  }
  if ( is_last_substring )
    _eof_idx = max_data_idx;
  if ( _eof_idx == output_.writer().bytes_pushed() && _eof_idx != std::numeric_limits<uint64_t>::max() )
    output_.writer().close();
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return _unassembled_bytes;
}
