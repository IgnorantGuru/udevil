if ADD_SYSTEMD
EXTRA_DIST = udevil.conf devmon devmon@.service
else
EXTRA_DIST = udevil.conf
endif

install-data-hook:
	test -d $(DESTDIR)/$(sysconfdir)/udevil || mkdir -p -- \
				$(DESTDIR)/$(sysconfdir)/udevil
	test -f $(DESTDIR)/$(sysconfdir)/udevil/udevil.conf || $(INSTALL_DATA) \
				$(srcdir)/udevil.conf \
				$(DESTDIR)/$(sysconfdir)/udevil/udevil.conf
if ADD_SYSTEMD
	test -d $(DESTDIR)/$(sysconfdir)/conf.d || mkdir -p -- \
				$(DESTDIR)/$(sysconfdir)/conf.d
	test -f $(DESTDIR)/$(sysconfdir)/conf.d/devmon || $(INSTALL_DATA) \
				$(srcdir)/systemd/devmon \
				$(DESTDIR)/$(sysconfdir)/conf.d/devmon
	test -d $(DESTDIR)/$(libdir)/systemd/system || \
				mkdir -p -- $(DESTDIR)/$(libdir)/systemd/system
	$(INSTALL_DATA) $(srcdir)/systemd/devmon@.service \
				$(DESTDIR)/$(libdir)/systemd/system/devmon@.service
endif
