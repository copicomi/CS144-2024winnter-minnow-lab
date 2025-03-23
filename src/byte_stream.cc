#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : 
	capacity_( capacity ),
	wr_count(0),
	pop_count(0),
	buf_count(0),
	head_buf_len(0),
	close_(false) {}

void Writer::push( string data )
{

	uint64_t len = data.size();

	if (len > available_capacity()) {
		len = available_capacity();
	}

	if (len == 0) return;

	data.resize(len);
	wr_count += len;
	buf_count += len;
	buf_.emplace(std::move(data));
	if (buf_.size() == 1) {
		head_buf_len = len;
	}
}

void Writer::close()
{
	close_ = true;
}

bool Writer::is_closed() const
{
	return close_;
}

uint64_t Writer::available_capacity() const
{
	return capacity_ - buf_count;
}

uint64_t Writer::bytes_pushed() const
{
	return wr_count;
}

string_view Reader::peek() const
{
	if (buf_count == 0 || buf_.size() == 0) return string_view{};
	const string &front_str = buf_.front();
	return string_view(front_str).substr(front_str.size() - head_buf_len);
}

void Reader::pop( uint64_t len )
{
	if (len > bytes_buffered()) {
		len = bytes_buffered();
	}
	pop_count += len;
	buf_count -= len;
	while (len > 0) {
		if (len >= head_buf_len) {
			buf_.pop();
			len -= head_buf_len;
			head_buf_len = buf_.front().size();
		}
		else {
			head_buf_len -= len;
			len = 0;
		}
	}
}

bool Reader::is_finished() const
{
	return buf_count == 0 && close_;
}

uint64_t Reader::bytes_buffered() const
{
	return buf_count;
}

uint64_t Reader::bytes_popped() const
{
	return pop_count;
}

