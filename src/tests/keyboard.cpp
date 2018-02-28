#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <termios.h>
#include <stdio.h>

int main ()
{
	struct termios OldModes;
	struct termios NewModes;
	int fd { fileno ( stdin ) };
	char c;

	setbuf ( stdin, ( char* ) 0 );
	tcgetattr (fileno (stdin), &OldModes);
	NewModes = OldModes;
	NewModes.c_lflag &= ~ ( ICANON );
	NewModes.c_lflag &= ~ ( ECHO | ECHOE | ECHOK );
	NewModes.c_lflag |= ECHONL;
	NewModes.c_cc[VMIN]  = 0;
	NewModes.c_cc[VTIME] = 1;
	tcsetattr ( fileno ( stdin ), TCSANOW, &NewModes );
	std::cout << "> ";
	while (c != 'q' )
	{
		if ( read ( fd, &c, 1 ) )
		{
			std::cout
				<< std::hex << int(c)
				<< std::dec << "'" << c << "'"
				<< std::endl;
			std::cout << "> ";
		}
	}
	tcsetattr ( fileno ( stdin ), TCSANOW, &OldModes );
	return 0;
}
