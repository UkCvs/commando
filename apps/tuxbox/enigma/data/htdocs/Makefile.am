EXTRA_DIST = \
	$(srcdir)/webif.css $(srcdir)/led_on.gif $(srcdir)/led_off.gif $(srcdir)/on.gif $(srcdir)/off.gif  \
	$(srcdir)/speak_off.gif $(srcdir)/speak_on.gif $(srcdir)/topbalk.png $(srcdir)/trans.gif $(srcdir)/squ.png $(srcdir)/timer.gif \
	$(srcdir)/index.js $(srcdir)/epg.js $(srcdir)/trash.gif $(srcdir)/dm7000.jpg $(srcdir)/blinking_red.gif \
	$(srcdir)/format_on.png $(srcdir)/format_off.png $(srcdir)/dolby_on.png $(srcdir)/dolby_off.png \
	$(srcdir)/crypt_on.png $(srcdir)/crypt_off.png $(srcdir)/mountMgr.js $(srcdir)/rc_small.jpg $(srcdir)/rc_big.jpg \
	$(srcdir)/bg.png $(srcdir)/rc_dbox2.jpg $(srcdir)/dhtml.js $(srcdir)/rc_dbox2_small.jpg \
	$(srcdir)/mepg_grad.png $(srcdir)/edit.gif $(srcdir)/timer.js $(srcdir)/window.js $(srcdir)/topbalk2.png $(srcdir)/topbalk3.png \
	$(srcdir)/topbalk4.png $(srcdir)/zap.js $(srcdir)/zapdata.js $(srcdir)/up.gif $(srcdir)/down.gif $(srcdir)/topbalk_small.png \
	$(srcdir)/webif_small.css $(srcdir)/topbalk2_small.png $(srcdir)/topbalk3_small.png \
	$(srcdir)/topbalk4_small.png $(srcdir)/blue.png $(srcdir)/green.png $(srcdir)/red.png $(srcdir)/grey.png $(srcdir)/yellow.png \
	$(srcdir)/locked.png $(srcdir)/unlocked.png $(srcdir)/1rightarrow.gif $(srcdir)/1leftarrow.gif $(srcdir)/player_stop.gif \
	$(srcdir)/player_fwd.gif $(srcdir)/player_rew.gif $(srcdir)/dvr.gif $(srcdir)/movieplayer.js

install-data-local:
	install -d $(DESTDIR)$(TUXBOXDATADIR)/enigma/htdocs
	install -m 0644 $(EXTRA_DIST) $(DESTDIR)$(TUXBOXDATADIR)/enigma/htdocs
