#include "wrapping_integers.hh"
#include <cstdint>
#include <limits>

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  return Wrap32(static_cast<uint32_t>(n) + zero_point.raw_value_);
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  uint64_t u32_upper = static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()) + 1;
  uint32_t ckpt_mod  = wrap( checkpoint, zero_point ).raw_value_;
  uint32_t offset    = raw_value_ - ckpt_mod;
  if ( offset<=(u32_upper>>1) || checkpoint+offset<u32_upper ) 
    return checkpoint + offset;
  else
    return checkpoint + offset - u32_upper;
}
