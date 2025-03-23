#include "reassembler.hh"
#include "debug.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
	
	// 判断字段是否合法，并截取

	if (data == "") {
		if (is_last_substring) {
			end_index = first_index;
		}
		check_and_close();
		return;
	}

	uint64_t last_index = data.size() + first_index - 1;

	if ( last_index < frag_base() || first_index >= end_base() ) {
		return;
	}
	else {
		if (first_index < frag_base()) {
			data = data.substr(frag_base() - first_index);
			first_index = frag_base();
		}
		if (last_index >= end_base()) {
			last_index = end_base() - 1;
			data = data.substr(0, last_index - first_index + 1);
			is_last_substring = false;
		}
	}

	// 更新缓存

	uint64_t offset = first_index - frag_base();

	while (rbuf.size() < offset + data.size()) {
		rbuf.push_back('\0');
		rbuf_bit.push_back(false);
	}

	for (size_t i = 0; i < data.size(); i ++) {
		rbuf[offset + i] = data[i];
		rbuf_bit[offset + i] = true;
		
	}


	if (is_last_substring) {
		end_index = last_index;
	}


	// 检查能否写入，写入 end_index 时 close

	if (rbuf_bit.size() && rbuf_bit.front() == true) {
		string push_data = "";
		while (rbuf_bit.size() && rbuf_bit.front() == true) {
			push_data.push_back(rbuf.front());

			rbuf.pop_front();
			rbuf_bit.pop_front();
		}

		output_.writer().push(push_data);
	}
	
	check_and_close();	


  // debug( "unimplemented insert({}, {}, {}) called", first_index, data, is_last_substring );
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
	uint64_t count = 0;
	for (const auto& x : rbuf_bit) {
		count += (x == true);
	}
	return count;
}
