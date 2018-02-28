#include <iostream>
#include <fglib/ipaddr.hxx>

int
main
(
	int argc,
	char* argv[]
)
{
	fgmp::ipaddr a;

	if ( argc < 2 )
	{
		std::cout << std::endl;
		std::cout << argv[0] << " host/addr" << std::endl;
		std::cout << std::endl;
		return 0;
	}

	a.assign ( argv[1] );
	std::cout << "got addr: " << a << std::endl;
	std::cout << "  type: " << a.type() << std::endl;
	std::cout << "  is mapped v4: " << a.is_mapped_v4() << std::endl;
	std::cout << "  is loopback: " << a.is_loopback() << std::endl;
	std::cout << "  is unspecified: " << a.is_unspecified() << std::endl;
	std::cout << "  is multicast: " << a.is_multicast() << std::endl;
	std::cout << "  is link local: " << a.is_link_local() << std::endl;
	std::cout << "  is site local: " << a.is_site_local() << std::endl;
	return 0;
}
