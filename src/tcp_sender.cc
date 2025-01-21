#include "tcp_sender.hh"
#include "tcp_config.hh"
#include <cstdint>
#include <unistd.h>

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return bytes_in_flight_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return consecutive_retrans_cnt_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // Your code here.
  while ( bytes_in_flight_ < ( window_size_ == 0 ? 1 : window_size_ ) ) {
    if ( FIN_ )
      break;
    auto msg = make_empty_message();
    if ( !SYN_ ) {
      msg.SYN = true;
      SYN_ = true;
    }
    auto remaining_wnd = ( window_size_ == 0 ? 1 : window_size_ ) - bytes_in_flight_;
    auto max_len = std::min( input_.reader().bytes_buffered(), TCPConfig::MAX_PAYLOAD_SIZE );
    auto payload_len = std::min( max_len, remaining_wnd-msg.sequence_length() );    
    read( input_.reader(), payload_len, msg.payload );
    if ( !FIN_ && input_.reader().is_finished() && remaining_wnd > msg.sequence_length() ) {
      FIN_ = true;
      msg.FIN = true;
    }
    if ( msg.sequence_length() == 0 )
      break;
    transmit( msg );
    if ( !timer_en_ ) {
      timer_en_ = true;
      timer_ = 0;
    }
    abs_seqno_ += msg.sequence_length();
    bytes_in_flight_ = abs_seqno_ - ack_;
    msg_in_flight_.emplace( msg );
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // Your code here.
  return { Wrap32::wrap( abs_seqno_, isn_ ), false, {}, false, input_.has_error() };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  window_size_ = msg.window_size;
  if ( msg.RST ) {
    input_.set_error();
    return;
  }
  if ( msg.ackno.has_value() ) {
    auto recv_ack = msg.ackno.value().unwrap( isn_, ack_ );
    if ( recv_ack > abs_seqno_ )
      return;
    while ( !msg_in_flight_.empty() ) {
      auto m = msg_in_flight_.front();
      if ( ack_ + m.sequence_length() > recv_ack )
        break;
      ack_ += m.sequence_length();
      bytes_in_flight_ = abs_seqno_ - ack_;
      consecutive_retrans_cnt_ = 0;
      cur_RTO_ms_ = initial_RTO_ms_;
      timer_ = 0;
      msg_in_flight_.pop();
      if ( msg_in_flight_.empty() )
        timer_en_ = false;
    }
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
  if ( timer_en_ )
    timer_ += ms_since_last_tick;
  if ( timer_ >= cur_RTO_ms_ ) {
    while ( !msg_in_flight_.empty() ) {
      auto msg = msg_in_flight_.front();
      auto msg_seq = msg.seqno.unwrap( isn_, ack_ );
      if ( msg_seq + msg.sequence_length() > ack_ ) {
        transmit( msg );
        if ( window_size_ != 0 ) {
          consecutive_retrans_cnt_ += 1;
          cur_RTO_ms_ <<= 1;
        }
        timer_ = 0;
        break;
      } else {
        msg_in_flight_.pop();
      }
    }
  }
}
