#include <iostream>
#include <netaddr.hxx>

int
main
(
	int argc,
	char* argv[]
)
{
	fgmp::netaddr a;
	fgmp::netaddr b;

	if ( argc < 3 )
	{
		std::cout << std::endl;
		std::cout << argv[0] << " host/addr service" << std::endl;
		std::cout << std::endl;
		return 0;
	}

	b.assign ( argv[2], 5000 );
	std::cout << "a: " << a << std::endl;
	std::cout << "b: " << b << std::endl;
	std::cout << "a == b: " << (a == b) << std::endl;
	std::cout << "a != b: " << (a != b) << std::endl;
	std::cout << "a < b: " << (a < b) << std::endl;
	std::cout << "a > b: " << (a > b) << std::endl;
	return 0;
}
