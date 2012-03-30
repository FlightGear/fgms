sbin_PROGRAMS	= fgms
noinst_LIBRARIES = libfg_server.a
INCLUDES = -I$(top_srcdir)/src 

libfg_server_a_SOURCES = \
	fg_server.cxx	\
	fg_server.hxx	\
	fg_tracker.cxx	\
	fg_tracker.hxx	\
	fg_config.cxx	\
	fg_config.hxx	\
	fg_player.cxx	\
	fg_player.hxx	\
	daemon.cxx	\
	daemon.hxx	\
	typcnvt.hxx	\
	common.h	\
	fg_geometry.cxx	\
	fg_geometry.hxx 

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

install-data-hook:
	test -d $(DESTDIR)$(sysconfdir) || mkdir $(DESTDIR)$(sysconfdir)
	test -e $(DESTDIR)$(sysconfdir)/fgms.conf \
	|| $(INSTALL) -m 600 $(srcdir)/fgms_example.conf \
	   $(DESTDIR)$(sysconfdir)/fgms.conf

EXTRA_DIST = fgms_example.conf FILES

