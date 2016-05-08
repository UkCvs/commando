SUBDIRS = \
	fonts \
	htdocs \
	pictures \
	resources \
	skins \
	templates \
	lirc

if TARGET_CDK
if !ENABLE_UCLIBC
SUBDIRS += \
	sysconfig
endif 
endif 

install-data-local:
	install -d $(DESTDIR)$(CONFIGDIR)/enigma
	install -m 0644 $(srcdir)/encoding.conf $(DESTDIR)$(CONFIGDIR)/enigma
