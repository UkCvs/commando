INCLUDES = \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/src \
	@CONFIGFILE_CFLAGS@ \
	@EVENTSERVER_CFLAGS@ \
	@FREETYPE_CFLAGS@ \
	@NET_CFLAGS@ \
	@PLUGINS_CFLAGS@ \
	@ZAPIT_CFLAGS@

noinst_LIBRARIES = libneutrino_gui_widget.a libneutrino_gui_widget2.a libneutrino_gui_widget3.a

libneutrino_gui_widget_a_SOURCES = \
	buttons.cpp \
	colorchooser.cpp keychooser.cpp \
	menue.cpp stringinput.cpp stringinput_ext.cpp \
	messagebox.cpp hintbox.cpp listbox.cpp listbox_legacy.cpp \
	lcdcontroler.cpp \
	drawable.cpp hintboxext.cpp helpbox.cpp mountchooser.cpp \
	listframe.cpp dirchooser.cpp progressbar.cpp

if BOXTYPE_DBOX2
libneutrino_gui_widget_a_SOURCES += \
	rgbcsynccontroler.cpp
endif

libneutrino_gui_widget2_a_SOURCES = \
	progresswindow.cpp

libneutrino_gui_widget3_a_SOURCES = \
	msgbox.cpp textbox.cpp
