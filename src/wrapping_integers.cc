#include "wrapping_integers.hh"
#include "debug.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  return Wrap32 { static_cast<uint32_t>(n) + zero_point.raw_value_ };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
	uint32_t real_address = raw_value_ - zero_point.raw_value_;
	uint32_t ck_address = static_cast<uint32_t>(checkpoint);
	uint32_t ck_mask = static_cast<uint32_t>(checkpoint >> 32);
	//cout << "TEST: " << ck_mask << static_cast<uint64_t>(real_address) << endl;

	if (ck_address == real_address) return checkpoint;
	else if (ck_address < real_address) {
		if (ck_address - real_address < real_address - ck_address && ck_mask > 0) {
			ck_mask --;
		}
	}	
	else {
		if (real_address - ck_address < ck_address - real_address && ck_mask < UINT32_MAX) {
			ck_mask ++;
		}
	}
	return (static_cast<uint64_t>(ck_mask) << 32) + static_cast<uint64_t>(real_address);
}
