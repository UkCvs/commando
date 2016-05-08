EXTRA_DIST = $(srcdir)/rcdboxnew.xml $(srcdir)/rcdboxold.xml $(srcdir)/rcdboxbuttons.xml $(srcdir)/help.xml \
	$(srcdir)/rcdm7000.xml $(srcdir)/rcdm5xxx.xml $(srcdir)/rctrdvb272.xml \
	$(srcdir)/rcdreambox_inputdev.xml $(srcdir)/rcgeneric_inputdev.xml $(srcdir)/rcdbox_inputdev.xml \
	$(srcdir)/keymappings.xml $(srcdir)/rcdreambox_keyboard.xml

install-data-local:
	install -d $(DESTDIR)$(TUXBOXDATADIR)/enigma/resources
	install -m 0644 $(EXTRA_DIST) $(DESTDIR)$(TUXBOXDATADIR)/enigma/resources
