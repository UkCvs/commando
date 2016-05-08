EXTRA_DIST = $(srcdir)/eit.tmp $(srcdir)/index.tmp $(srcdir)/helpDMMSites.tmp $(srcdir)/helpOtherSites.tmp $(srcdir)/helpForums.tmp \
	$(srcdir)/mepg.tmp $(srcdir)/audioSelection.tmp $(srcdir)/editTimerEvent.tmp $(srcdir)/sendMessage.tmp $(srcdir)/movieplayer.xml \
	$(srcdir)/configSwapFile.tmp $(srcdir)/msgWindow.tmp $(srcdir)/zap.tmp $(srcdir)/movies.tmp $(srcdir)/index2.tmp $(srcdir)/rotorSat.tmp \
	$(srcdir)/index_small.tmp $(srcdir)/index_big.tmp $(srcdir)/eit_small.tmp $(srcdir)/addTimerEvent.tmp $(srcdir)/wap.tmp $(srcdir)/wapepg.tmp $(srcdir)/wapzap.tmp \
	$(srcdir)/wapEPGDetails.tmp $(srcdir)/wapTimerList.tmp $(srcdir)/queryEditTimer.tmp $(srcdir)/queryDeleteTimer.tmp $(srcdir)/deleteTimerComplete.tmp \
	$(srcdir)/mountPoint.tmp $(srcdir)/mountPoints.tmp $(srcdir)/mountPointWindow.tmp $(srcdir)/subChannelSelection.tmp $(srcdir)/mepg_small.tmp \
	$(srcdir)/pdaResponse.tmp $(srcdir)/flashMgr.tmp $(srcdir)/configSettings.tmp $(srcdir)/remoteControl.tmp $(srcdir)/streamingServer.tmp \
	$(srcdir)/pdaRemoteControl.tmp $(srcdir)/sat.tmp $(srcdir)/satFinder.tmp $(srcdir)/remoteControlDbox2.tmp $(srcdir)/leftnavi.tmp $(srcdir)/rotor.tmp \
	$(srcdir)/root.tmp $(srcdir)/flashMgrReadMenu.tmp $(srcdir)/flashMgrWriteMenu.tmp $(srcdir)/mtd.tmp $(srcdir)/flashMgrProgressData.tmp $(srcdir)/flashMgrProgress.tmp \
	$(srcdir)/pdaRemoteControlDbox2.tmp $(srcdir)/configSwapFileMenu.tmp $(srcdir)/movieSources.tmp $(srcdir)/bootMenus.tmp $(srcdir)/bootMgr.tmp \
	$(srcdir)/XMLStreaminfo.tmp $(srcdir)/streaminfo_satellite.xsl $(srcdir)/streaminfo_cable.xsl $(srcdir)/streaminfo_terrestrial.xsl $(srcdir)/serviceepg.xsl \
	$(srcdir)/HTMLImageInfo.tmp $(srcdir)/image.tmp $(srcdir)/bootMgrSettings.tmp $(srcdir)/chttpdSettings.tmp $(srcdir)/instimg.tmp \
	$(srcdir)/XMLCurrentServiceData.tmp $(srcdir)/editImageSettingsWindow.tmp $(srcdir)/addImageWindow.tmp $(srcdir)/XSLMPSettings.xsl \
	$(srcdir)/editStreamingServerSettings.tmp $(srcdir)/editStreamingServerVLCSettings.tmp $(srcdir)/editStreamingServerVideoSettings.tmp \
	$(srcdir)/dvrcontrols.tmp $(srcdir)/webxtvMSIE.tmp $(srcdir)/webxtvFirefox.tmp $(srcdir)/teletext.tmp \
	$(srcdir)/XMLBoxInfo.tmp  $(srcdir)/HTMLBoxInfo.tmp \
	$(srcdir)/XMLData.tmp $(srcdir)/HTMLData.tmp \
	$(srcdir)/XMLServiceEPG.tmp $(srcdir)/XMLEPGEntry.tmp $(srcdir)/HTMLServiceEPG.tmp $(srcdir)/HTMLEPGEntry.tmp \
	$(srcdir)/XMLTimerList.tmp $(srcdir)/XMLTimerEntry.tmp $(srcdir)/XMLTimerListBody.tmp $(srcdir)/HTMLTimerEntry.tmp $(srcdir)/HTMLTimerListBody.tmp

install-data-local:
	install -d $(DESTDIR)$(TUXBOXDATADIR)/enigma/templates
	install -m 0644 $(EXTRA_DIST) $(DESTDIR)$(TUXBOXDATADIR)/enigma/templates
