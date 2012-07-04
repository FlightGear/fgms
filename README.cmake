file: README.cmake - created 20120704

Due to the BIG difficulty of maintaining multiple 
versions of the windows MSVC build files, previously 
in the 'msvc' folder, this folder has been deleted,
and an experimental CMakeLists.txt added instead.

Building:

For those unfamiliar with CMake (http://www.cmake.org/), 
it works best for out-of-source building. That is you 
create a separate directory, say build-fgms, change 
into that directory, and run -

$ cmake /path/to/source/fgms
$ cmake --build . [--config Release|Debug]

If the generator chosen is 'make', then that second 
steps can be replaced with the classic
$ make

Alternatively the first command can be replaced by 
using the cmake-gui - that is -
$ cmake-gui /path/to/source/fgms

In the CMake window, ensure the first line points 
to where the current source of fgms is, and the 
second line points to where to build the binaries,
and that should be somewhere OUT of the fgms source 
tree!

Push [ Configure ], choose which generator to use,
maybe correct, and fix especially any 'red' lines,
and push [ Configure ] perhaps several times, then 
when the list looks clean, push [ Generate ].

Specifically, and ONLY for Windows, you MUST copy 
the fgms source file config.h.msvc to your build 
directory, naming it config.h. This is NOT required 
in any unix system, and may cause problems if done.

Now, if you had chosen say a Visual Studio generator,
you can load fgms.sln into MSVC, and proceed to do 
the build.

Installing:

If you want a system wide install, then this is 
by the classic -
$ make install

Or the cmake way -
$ cmake -DBUILD_TYPE=Release -P cmake_install.cmake

Or in the MSVC IDE, running the 'INSTALL' project 
manually. It is disabled by default.

Normally fgms installs in /usr/sbin in unix systems,
but can be controlled by adding the folowing to 
the cmake command

$ cmake /path/to/fgms -DCMAKE_INSTALL_PREFIX=<anywhere>

Then as the standard INSTALL document states -

- copy the file "fgms_example.conf" to "fgms.conf", edit 
  this file to your needs and copy it to your "sysconfdir"
  That would normally be /usr/etc in a unix system, or 
  to the folder where fgms.exe is in Windows.

Running fgms:

Running $ ./fgms -h will show

syntax: ./fgms options

options are:
-h            print this help screen
-a PORT       listen to PORT for telnet
-c config     read 'config' as configuration file
-p PORT       listen to PORT
-t TTL        Time a client is active while not sending packets
-o OOR        nautical miles two players must be apart to be out of reach
-l LOGFILE    Log to LOGFILE
-v LEVEL      verbosity (loglevel) in range 1 (few) and 5 (much)
-d            do _not_ run as a daemon (stay in foreground)
-D            do run as a daemon

the default is to run as a daemon, which can be overridden in the
config file.

The configuration file can also set all these options, except 
verbosity. See the src/server/fgms_example.conf for details.


Additional Build Options:

Server #2:

The above server will use default file names of fgms.conf,
as the confgiuration file, fgms-exit, fgms-stat, as the 
file interface files, and fg_server.log. This latter log 
file name can also be established in fgms.conf.

But there is a cmake OPTION to generate a second fgms 
server with different defaults - fgms2.conf, fgms-exit2, 
fgms-stat2, and fg_server2.log - adding -
 -DBUILD_SERVER2:BOOL=TRUE
to the cmake command.

fgtracker:

There is also a cmake OPTION to generate the fgtracker 
binaries - fgt_server and fgt_client. This is controls by 
adding the following to the cmake command -
 -DBUILD_TRACKER

But be aware, this fgtracker presently has a dependency 
on PostgreSQL, so you must install this package, configure, 
and get the postgresql server running first.

Also then cmake uses find_package(PostgreSQL REQUIRED). In
this case, so you MUST have FindPostgreSQL.cmake installed, 
and this is not always done by default.

The files that need to be found are the header libpq-fe.h,
and the 'pg' library.

General:

While it is not intended that this CMakeLists.txt should 
immediately replace the current automake - configure.ac and 
Makefile.am - system, it is hope after it has been tested 
fully, that it will become the ONLY build system in the 
fgms source.

Meantime, both will be maintained - also read INSTALL 
file.

Have fun!

Geoff.
reports _at_ geoffair _dot_ info
20120704

# eof
