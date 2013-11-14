# Based off XFCE4 Terminal build system
LINGUAS = $(shell cd $(srcdir)/po4a && echo `/bin/ls *.po | sed 's,[.]po,,g'`)

# Install the translated man files to the destdir
install-data-local:
	for lang in $(LINGUAS); do \
		file=$(srcdir)/translated/$$lang/$(PACKAGE).1; \
		if test -f $$file ; then \
			if test x"$$lang" = x"C"; then \
				dest=$(DESTDIR)$(mandir)/man1; \
			else \
				dest=$(DESTDIR)$(mandir)/$$lang/man1; \
			fi; \
			$(mkinstalldirs) $$dest; \
			$(INSTALL_DATA) $$file $$dest/$(PACKAGE).1; \
		fi \
	done;

# Remove installed files from the destdir
uninstall-local:
	for lang in $(LINGUAS); do \
		if test x"$$lang" = x"C"; then \
			dest=$(DESTDIR)$(mandir)/man1/$(PACKAGE).1; \
		else \
			dest=$(DESTDIR)$(mandir)/$$lang/man1/$(PACKAGE).1; \
		fi; \
		if test -f $$dest; then \
			rm -f $$dest; \
		fi \
	done

# Include the man files in the dist
dist-hook:
	for lang in $(LINGUAS); do \
		if test -d $(srcdir)/translated/$$lang; then \
			mkdir -p $(distdir)/man/translated/$$lang; \
			cp $(srcdir)/translated/$$lang/* $(distdir)/man/translated/$$lang/; \
		fi; \
	done
