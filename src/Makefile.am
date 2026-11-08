## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@

AM_CFLAGS = -Wall

bin_PROGRAMS = carthage

carthage_SOURCES = \
	main.c \
	support.c support.h \
	interface.c interface.h \
	callbacks.c callbacks.h \
	ui_theme.c ui_theme.h \
	ui_anim.c ui_anim.h \
	ui_shell.c ui_shell.h \
	connexion.c connexion.h \
	passager_admin.c passager_admin.h \
	passager_espace.c passager_espace.h \
	passager.c passager.h \
	utils.c utils.h \
	auth.c auth.h \
	session.c session.h \
	chemins.h

carthage_LDADD = @PACKAGE_LIBS@ $(INTLLIBS) -lm

