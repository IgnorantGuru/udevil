INCLUDES = -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"

noinst_PROGRAMS = udevil

udevil_SOURCES = udevil.c device-info.c canonicalize.c

bin_SCRIPTS = devmon

INSTALL_DIR = $(DESTDIR)/$(prefix)/bin
INSTALL_SRC = $(top_builddir)/src

install-data-hook:
	test -z $(INSTALL_DIR) || mkdir -p -- $(INSTALL_DIR)
	$(INSTALL_DATA) -o root -g root -m 4755 -D $(INSTALL_SRC)/udevil $(INSTALL_DIR)/udevil

uninstall-hook:
	rm -f $(INSTALL_DIR)/udevil

udevil_CFLAGS = \
    @GLIB_CFLAGS@ \
    @LIBUDEV_CFLAGS@ \
    -Werror-implicit-function-declaration

udevil_LDADD = \
    $(INTLLIBS) \
    @GLIB_LIBS@ \
    @LIBUDEV_LIBS@

