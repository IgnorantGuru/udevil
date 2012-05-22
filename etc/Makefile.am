EXTRA_DIST = udevil.conf

install-data-hook:
	test -d $(DESTDIR)/etc/udevil || mkdir -p -- $(DESTDIR)/etc/udevil
	test -f $(DESTDIR)/etc/udevil/udevil.conf || $(INSTALL_DATA) \
				$(srcdir)/udevil.conf $(DESTDIR)/etc/udevil/udevil.conf

