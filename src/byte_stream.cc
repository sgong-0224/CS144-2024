#include "byte_stream.hh"
#include <iterator>
#include <string_view>

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  // Your code here.
  return write_end_;
}

void Writer::push( string data )
{
  // Your code here.
  auto push_len = std::min( data.size(), available_capacity() );
  if ( push_len == data.size() )
    data_.append( std::move( data ) );
  else
    data_.append( data.data(), push_len );
  written_bytes_ += push_len;
  return;
}

void Writer::close()
{
  // Your code here.
  write_end_ = true;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - data_.size();
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return written_bytes_;
}

bool Reader::is_finished() const
{
  // Your code here.
  return write_end_ && data_.empty();
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return read_bytes_;
}

string_view Reader::peek() const
{
  // Your code here.
  return std::string_view( data_.data(), data_.size() );
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  if ( len > data_.size() ) {
    set_error();
    return;
  }
  data_.erase( 0, len );
  read_bytes_ += len;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return data_.size();
}
