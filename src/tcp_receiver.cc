#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
	bool last_flag = false;

	if (message.RST == true) {
		RST = true;
		set_error();
	}	

	if (message.SYN == true) {
		SYN = true;
		SYN_index = message.seqno;
		message.seqno = message.seqno + 1;
	}

	if (message.FIN == true) {
		FIN = true;
		FIN_index = message.seqno + message.payload.size();
		last_flag = true;
	}
	
	if (SYN == true) {
		reassembler_.insert( message.seqno.unwrap(SYN_index, writer().bytes_pushed()) - 1, message.payload, last_flag );
	}
	
	if (writer().has_error() == true) {
		RST = true;
	}
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
	TCPReceiverMessage message;
	Wrap32 ackno = Wrap32::wrap(writer().bytes_pushed() + 1, SYN_index); 
	if (ackno == FIN_index) {
		ackno = ackno + 1;
	}
	
	if (SYN == true) {
		message.ackno = ackno;
	}
		message.RST = has_error();
		message.window_size = writer().available_capacity() > 65535 ? 65535 : writer().available_capacity();

	return message;
}
