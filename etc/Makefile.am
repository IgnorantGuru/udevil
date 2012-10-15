if ADD_SYSTEMD
EXTRA_DIST = udevil.conf devmon devmon@.service
else
EXTRA_DIST = udevil.conf
endif

install-data-hook:
	test -d $(DESTDIR)/etc/udevil || mkdir -p -- $(DESTDIR)/etc/udevil
	test -f $(DESTDIR)/etc/udevil/udevil.conf || $(INSTALL_DATA) \
				$(srcdir)/udevil.conf $(DESTDIR)/etc/udevil/udevil.conf
if ADD_SYSTEMD
	test -d $(DESTDIR)/etc/conf.d || mkdir -p -- $(DESTDIR)/etc/conf.d
	test -f $(DESTDIR)/etc/conf.d/devmon || $(INSTALL_DATA) \
				$(srcdir)/systemd/devmon $(DESTDIR)/etc/conf.d/devmon
	test -d $(DESTDIR)/$(libdir)/systemd/system || \
				mkdir -p -- $(DESTDIR)/$(libdir)/systemd/system
	$(INSTALL_DATA) $(srcdir)/systemd/devmon@.service \
				$(DESTDIR)/$(libdir)/systemd/system/devmon@.service
endif
