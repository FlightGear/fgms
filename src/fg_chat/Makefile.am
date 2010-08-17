sbin_PROGRAMS	= fgms
noinst_LIBRARIES = libfg_server.a
INCLUDES = -I$(top_srcdir)/src 

libfg_server_a_SOURCES = \
	fg_chat.cxx	\
	fg_chat.hxx

fgms_SOURCES	= \
	main.cxx

fgms_DEPENDENCIES = \
	libfg_server.a \
	$(top_builddir)/src/plib/libplib.a				\
	$(top_builddir)/src/simgear/debug/libsgdebug.a			\
	$(top_builddir)/src/flightgear/MultiPlayer/libMultiPlayer.a

fgms_LDADD	= \
	libfg_server.a \
	$(top_builddir)/src/plib/libplib.a				\
	$(top_builddir)/src/simgear/debug/libsgdebug.a			\
	$(top_builddir)/src/flightgear/MultiPlayer/libMultiPlayer.a

