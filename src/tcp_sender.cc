#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"


using namespace std;

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::sequence_numbers_in_flight() const
{
	return flight_count;
}

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::consecutive_retransmissions() const
{
  debug( "unimplemented consecutive_retransmissions() called" );
  return {};
}

void TCPSender::push( const TransmitFunction& transmit )
{

	TCPSenderMessage message;

	// 检查 payload 长度
	uint64_t segment_len = window_size - flight_count;
	if (segment_len > TCPConfig::MAX_PAYLOAD_SIZE) {
		segment_len = TCPConfig::MAX_PAYLOAD_SIZE;
	}

	// 头序号
	message.seqno = head_seqno();

	// SYN
	if (SYN == false && reader().bytes_popped() == 0) {
		message.SYN = true;
		SYN = true;
	}

	// FIN
	if (FIN == false && flight_count == 0 && reader().is_finished() == true) {
		message.FIN = true;
		FIN = true;
	}

	// payload
	uint64_t payload_len = segment_len - message.SYN - message.FIN;

	read(reader(), payload_len, message.payload);

	// RST
	if (writer().has_error() == true) {
		message.RST = true;
		RST = true;
	}

	// message 非空
	if (message.sequence_length() > 0) {

		transmit(message);
	
		// 将 message 存进数据结构，用于超时重传
		
		lost_messages.push(message);

		flight_count += message.sequence_length();

	}
}

TCPSenderMessage TCPSender::make_empty_message() const
{
	TCPSenderMessage message;

	message.seqno = head_seqno();		
	
	return message;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
	window_size = msg.window_size;

	// 检查 ackno，移除已经收到的包
	while (lost_messages.empty() == false && !(msg.ackno == lost_messages.front().seqno) ) {

		flight_count -= lost_messages.front().sequence_length();
		lost_messages.pop();

		reTimer.reset_RTO();

	}

	// 检查是否仍有未收到的包，更新计时器
	if (lost_messages.empty() == true) {
		reTimer.stop();	
	}
	else {
		reTimer.start();
	}
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  debug( "unimplemented tick({}, ...) called", ms_since_last_tick );
  (void)transmit;
}
