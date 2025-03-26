#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <functional>

class RetransmissionTimer {

public:
	
	RetransmissionTimer(uint64_t RTO_ms) : RTO_ms_(RTO_ms), initial_RTO_ms_(RTO_ms) {}

	// 检查超时
	bool overtime() { return is_started && timer >= RTO_ms_; }

	// 检查启动状态
	bool running() { return is_started; }

	// 启动、关闭、更新时间
	void start() { timer = 0; is_started = true; }
	void stop() { timer = 0; is_started = false; }
	void update( uint64_t ms_since_last_tick ) { timer += ms_since_last_tick; }

	// RTO 修改
	void reset_RTO() { set_RTO_ms_(initial_RTO_ms_); }
	void double_RTO() { set_RTO_ms_(RTO_ms_ * 2); }

private:
	
	uint64_t RTO_ms_;	
	uint64_t initial_RTO_ms_;
	uint64_t timer { 0 };
	bool is_started { false };

	void set_RTO_ms_( uint64_t RTO_ms ) { RTO_ms_ = RTO_ms; }
};



class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ), isn_( isn ), initial_RTO_ms_( initial_RTO_ms ), reTimer( initial_RTO_ms_ )
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // For testing: how many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // For testing: how many consecutive retransmissions have happened?
  const Writer& writer() const { return input_.writer(); }
  const Reader& reader() const { return input_.reader(); }
  Writer& writer() { return input_.writer(); }

  Wrap32 head_seqno() const { return isn_ + reader().bytes_popped() + SYN + FIN; }

  uint64_t abs_seqno( Wrap32 seqno ) { return seqno.unwrap(isn_, reader().bytes_popped()); }
  uint64_t lost_msg_abs_seqno() { return lost_messages.size() > 0 ? abs_seqno( lost_messages.front().seqno ) : 0; }

private:
  Reader& reader() { return input_.reader(); }

  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;

  	uint16_t window_size { 1 };
	bool SYN { false };
	bool FIN { false };
	bool RST { false };
	
  	uint64_t resend_count { 0 };
	RetransmissionTimer reTimer;

	uint64_t flight_count { 0 };

	std::queue<TCPSenderMessage> lost_messages { };
};

