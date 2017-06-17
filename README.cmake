file: README.cmake - 20170617 - 20150307 - 20150216 - created 20120704

This project is now only configured by cmake, and then using 
either the default or the chosen build generation.

Due to the BIG difficulty of maintaining multiple 
versions of the windows MSVC build files, previously 
in the 'msvc' folder, this folder has been deleted,
and a CMakeLists.txt added instead.

Prerequisites:

1. git - http://git-scm.com/
2. cmake - http://www.cmake.org/
3. pthread - http://en.wikipedia.org/wiki/POSIX_Threads

Building:

Note the special additional requirements for 
Windows below.

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
and push [ Configure ] again, perhaps several times, 
then when the list looks clean, push [ Generate ].

Special Requirements for Windows ONLY:
======================================

Further fgms has a requirement of pthreads for 
windows - see http://sourceware.org/pthreads-win32/
During the compile of fgms it must find the 'pthread.h' 
header, and others, 'sched.h' and 'semaphore.h', and 
for the link find 'pthreadVC2.lib', and for the running 
'pthreadVC2.dll'.

You can either first download this source, and compile 
it in your system, or get a copy of the latest pre built 
release zip, unzipping it to a directory or your choice.

Then in the CMake GUI make sure 'thread_INC' points to 
where the pthread.h header is, and 'thread_LIB' has 
the full path pthreadVC2.lib in it, before you push 
[ Generate ].

Now, if you had chosen say a Visual Studio generator,
you can load fgms.sln into MSVC, and proceed to do 
the build in the IDE.

Installing:

If you want a system wide install, then this is 
by the classic -
$ [sudo] make install

Or the cmake way -
$ cmake --build . --config Release
or
$ cmake -DBUILD_TYPE=Release -P cmake_install.cmake

Or in the MSVC IDE, running the 'INSTALL' project 
manually. Right click on the INSTALL project, and 
select build it. It is disabled by default.

Normally fgms installs in /usr/sbin in unix systems,
but can be controlled by adding the following to 
the cmake command -

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
-v LEVEL      verbosity (loglevel) in range 1 (few) and 4 (much)
-d            do _not_ run as a daemon (stay in foreground)
-D            do run as a daemon

In unix systems the default is to run as a daemon, which can 
be overridden in the config file.

The configuration file can also set all these options, except 
verbosity. See the src/server/fgms_example.conf for details.


Additional Build Options:

Server #2:

The above server will use default file names of fgms.conf,
as the configuration file, fgms-exit, fgms-stat, as the 
file interface files, and fg_server.log. This latter log 
file name can also be established in fgms.conf.

But there is a cmake OPTION to generate a second fgms 
server with different defaults - fgms2.conf, fgms-exit2, 
fgms-stat2, and fg_server2.log - adding -
 -DBUILD_SERVER2:BOOL=TRUE
to the cmake command.

fgtracker:

As at 20170617, fgtracker has been rewritten and its source code
is hosted at https://gitlab.com/fgms/FGTracker

shared libraries:

There is an option to generate the 'intermediate' libraries 
as shared libraries - DLL in windows - but this is NOT 
needed nor recommended. The default static library builds 
are fine. They are only used in fgms, so no need to 
'share' them.

Have fun!

Geoff.
reports _at_ geoffair _dot_ info
20150307

Edited by Hazuki on 20170617 - remove fgtracker
# eof
