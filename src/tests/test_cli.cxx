#include <iostream>
#include <string>
#include <libcli/cli_client.hxx>
#include <libcli/cli_line.hxx>

using namespace libcli;

int
main
(
        int argc,
        char* argv[]
)
{
        cli_client      cli (0);
        line_editor     ed ( cli );
        ed.set_prompt ("Username:");
        int c;
        std::string input;
        bool clear = true;

        while ( input != "quit" )
        {
                c = ed.read_line ( clear );
                if ( c == '\n' )
                {
                        input = ed.get_line ();
                        std::cout << std::endl << "got input: '" << input << "'" << std::endl;
                        clear = true;
                }
                else
                {
                        std::cout  << std::endl<< "got key: " << c << std::endl;
                        clear = false;
                }
        }

}
