#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"

#include <iostream>

using namespace std;

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::sequence_numbers_in_flight() const
{
	return flight_count;
}

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::consecutive_retransmissions() const
{
	return resend_count;
}

void TCPSender::push( const TransmitFunction& transmit )
{

	// 窗口分为两段 : [0, flight_count - 1], [flight_count, window_size - 1]

	const uint64_t &MAX = TCPConfig::MAX_PAYLOAD_SIZE;

	for (uint64_t i = flight_count; i < window_size && FIN == false; /* i 的更新放在循环最后 */) {

		TCPSenderMessage message;
		
		// 头序号
		message.seqno = head_seqno();
	
		// SYN
		if (SYN == false && reader().bytes_popped() == 0) {
			message.SYN = true;
			SYN = true;
		}
	
		// assume FIN followed
		if (FIN == false && writer().is_closed() == true &&
			   	i + message.SYN + min(reader().bytes_buffered(), MAX) < window_size) {
			message.FIN = true;
			FIN = true;
		}
	
		// payload
		uint64_t payload_len = min( min(
				   					MAX,
				   					reader().bytes_buffered() ) ,
			   						window_size - i - message.SYN - message.FIN );
	
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
	
			if (reTimer.running() == false) {
				reTimer.start();
			}

		}
		// 无信息就退出
		else {
			break;
		}

		// 更新 i
		i += message.sequence_length();
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
	bool received_new_data = false;

	uint64_t msg_abs_seqno = abs_seqno(msg.ackno.value_or(Wrap32(0)));

	// 检查 ackno，移除已经收到的包
	while ( lost_messages.empty() == false &&
		   	msg_abs_seqno > lost_msg_abs_seqno() && 
		 	msg_abs_seqno <= lost_msg_abs_seqno() + flight_count ) {

		// 不是整块，先忽略（待定）
		if (msg_abs_seqno < lost_msg_abs_seqno() + lost_messages.front().sequence_length() ) 
			return;

		flight_count -= lost_messages.front().sequence_length();
		lost_messages.pop();

		received_new_data = true;

	}

	// 更新窗口大小
	window_size = msg.window_size;

	// 检查是否仍有未收到的包，更新计时器
	if (received_new_data == true) {
		reTimer.reset_RTO();
		resend_count = 0;
		reTimer.start();
	}

	if (lost_messages.empty() == true) {
		reTimer.stop();	
	}
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{

	reTimer.update(ms_since_last_tick);
	
	if (reTimer.overtime() == true) {
		transmit(lost_messages.front());

		if (window_size > 0) {
			
			resend_count ++;

			reTimer.double_RTO();
		}
		

		reTimer.start();
	}
	
}
