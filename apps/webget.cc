#include "socket.hh"

#include <cstdlib>
#include <iostream>
#include <span>
#include <string>

using namespace std;

void get_URL( const string& host, const string& path )
{
	string endmsg = "\r\n";
	string msg1 = "GET " + path + " HTTP/1.1" + endmsg;
	string msg2 = "HOST: " + host + endmsg;
	string msg3 = "Connection: close" + endmsg;
	string msg4 = endmsg;

	string buf;

	TCPSocket tcpSocket;
	tcpSocket.connect(Address(host, "http"));
	tcpSocket.write(msg1 + msg2 + msg3 + msg4);

	tcpSocket.shutdown(SHUT_WR);

	while (tcpSocket.eof() == false) {
		tcpSocket.read(buf);
		cout << buf;
	}

	tcpSocket.close();

	return;

  //cerr << "Function called: get_URL(" << host << ", " << path << ")\n";
  //cerr << "Warning: get_URL() has not been implemented yet.\n";
}

int main( int argc, char* argv[] )
{
  try {
    if ( argc <= 0 ) {
      abort(); // For sticklers: don't try to access argv[0] if argc <= 0.
    }

    auto args = span( argv, argc );

    // The program takes two command-line arguments: the hostname and "path" part of the URL.
    // Print the usage message unless there are these two arguments (plus the program name
    // itself, so arg count = 3 in total).
    if ( argc != 3 ) {
      cerr << "Usage: " << args.front() << " HOST PATH\n";
      cerr << "\tExample: " << args.front() << " stanford.edu /class/cs144\n";
      return EXIT_FAILURE;
    }

    // Get the command-line arguments.
    const string host { args[1] };
    const string path { args[2] };

    // Call the student-written function.
    get_URL( host, path );
  } catch ( const exception& e ) {
    cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
