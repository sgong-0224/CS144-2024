#include "tcp_receiver.hh"
#include <sys/socket.h>

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  if(message.RST){
    reassembler_.reader().set_error();
    return;
  }else if(reassembler_.reader().has_error()){
    return;
  }
  if(message.SYN){
    SYN_ = true;
    _init_seq = message.seqno;
    message.seqno = message.seqno + 1;
  }
  if(!SYN_)
    return;
  auto first_seq = message.seqno.unwrap(_init_seq, reassembler_.writer().bytes_pushed());
  if(first_seq==0)
    return;
  reassembler_.insert(first_seq-1, message.payload, message.FIN);
  _next_ack = _init_seq + SYN_ + reassembler_.writer().bytes_pushed() + reassembler_.writer().is_closed();
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  TCPReceiverMessage message;
  message.RST = reassembler_.reader().has_error();
  message.window_size = reassembler_.writer().available_capacity() > 65535 ? 
                        65535 : reassembler_.writer().available_capacity();
  if(SYN_)
    message.ackno = _next_ack;
  return message;
}
