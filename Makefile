# By default, this Makefile only builds and installs software that is
# considered to be somewhat finished and stable. If you want to build other
# software as well, that may be buggy or completely broken, you need to run
# make like this: 
#
#   make build_experimental=yes
#   make build_experimental=yes install
#
# ...or edit this file to set the variable "build_experimental" to "yes".


PACKAGE_NAME = ll-plugins
PACKAGE_VERSION = 0.1.288
PKG_DEPS = \
	jack>=0.102.27 \
	lash-1.0>=0.5.1 \
	liblo>=0.22 \
	gtkmm-2.4>=2.8.8 \
	cairomm-1.0>=1.2.4 \
	gsl>=1.8 \
	sndfile>=1.0.16


ARCHIVES = libpaq.a

ifeq ($(build_experimental),yes)
  ARCHIVES += \
	liblv2_plugin.a \
	liblv2_advanced.a \
	liblv2_gtk2gui.a \
	libkeyboard.a \
	libvgknob.a \
	libenvelopeeditor.a \
	libshapereditor.a \
	libspringeditor.a \
	libpdeditor.a \
	libtransitioneditor.a \
	libpatternwidget.a
endif


PROGRAMS = lv2peg

ifeq ($(build_experimental),yes)
  PROGRAMS += elven
endif


ifeq ($(build_experimental),yes)
  LV2_BUNDLES = \
	arpeggiator.lv2 \
	audio_identity.lv2 \
	control2midi.lv2 \
	euphoria.lv2 \
	horizon.lv2 \
	klaviatur.lv2 \
	midi_identity.lv2 \
	math-constants.lv2 \
	math-functions.lv2 \
	phase-distortion-osc.lv2 \
	sineshaper.lv2 \
	trilobeat.lv2 \
	trilobyte.lv2
endif


# Archives with useful code bits

liblv2_plugin_a_SOURCES = lv2plugin.hpp lv2plugin.cpp
liblv2_plugin_a_CFLAGS = -I.
liblv2_plugin_a_SOURCEDIR = libraries/lv2plugin

liblv2_advanced_a_SOURCES = lv2advanced.hpp lv2advanced.cpp
liblv2_advanced_a_CFLAGS = -Iextensions/command -I.
liblv2_advanced_a_SOURCEDIR = libraries/lv2plugin

libpaq_a_SOURCES = \
	turtleparser.hpp turtleparser.cpp \
	unicode.hpp \
	namespaces.hpp \
	rdf.hpp rdf.cpp \
	query.hpp query.cpp
libpaq_a_SOURCEDIR = libraries/paq

liblv2_gtk2gui_a_SOURCES = lv2gtk2gui.hpp lv2gtk2gui.cpp
liblv2_gtk2gui_a_CFLAGS = `pkg-config --cflags gtkmm-2.4` -Iextensions/gtkgui -I.
liblv2_gtk2gui_a_SOURCEDIR = libraries/lv2gtk2gui

libkeyboard_a_SOURCES = keyboard.hpp keyboard.cpp
libkeyboard_a_CFLAGS = `pkg-config --cflags gtkmm-2.4`
libkeyboard_a_SOURCEDIR = libraries/widgets

libvgknob_a_SOURCES = vgknob.hpp vgknob.cpp
libvgknob_a_CFLAGS = `pkg-config --cflags gtkmm-2.4 cairomm-1.0`
libvgknob_a_SOURCEDIR = libraries/widgets

libenvelopeeditor_a_SOURCES = envelopeeditor.hpp envelopeeditor.cpp
libenvelopeeditor_a_CFLAGS = `pkg-config --cflags gtkmm-2.4 cairomm-1.0`
libenvelopeeditor_a_SOURCEDIR = libraries/widgets

libshapereditor_a_SOURCES = shapereditor.hpp shapereditor.cpp
libshapereditor_a_CFLAGS = `pkg-config --cflags gtkmm-2.4 cairomm-1.0`
libshapereditor_a_SOURCEDIR = libraries/widgets

libspringeditor_a_SOURCES = springeditor.hpp springeditor.cpp
libspringeditor_a_CFLAGS = `pkg-config --cflags gtkmm-2.4 cairomm-1.0`
libspringeditor_a_SOURCEDIR = libraries/widgets

libpdeditor_a_SOURCES = pdeditor.hpp pdeditor.cpp
libpdeditor_a_CFLAGS = `pkg-config --cflags gtkmm-2.4 cairomm-1.0`
libpdeditor_a_SOURCEDIR = libraries/widgets

libtransitioneditor_a_SOURCES = transitioneditor.hpp transitioneditor.cpp
libtransitioneditor_a_CFLAGS = `pkg-config --cflags gtkmm-2.4 cairomm-1.0`
libtransitioneditor_a_SOURCEDIR = libraries/widgets

libpatternwidget_a_SOURCES = patternwidget.hpp patternwidget.cpp
libpatternwidget_a_CFLAGS = `pkg-config --cflags gtkmm-2.4` -Ilibraries/components
libpatternwidget_a_SOURCEDIR = libraries/widgets


# Executable programs

lv2peg_SOURCES = lv2peg.cpp
lv2peg_CFLAGS = -Ilibraries/paq -DVERSION=\"$(PACKAGE_VERSION)\"
lv2peg_ARCHIVES = libraries/paq/libpaq.a
lv2peg_SOURCEDIR = programs/lv2peg

elven_SOURCES = \
	lv2guihost.hpp lv2guihost.cpp \
	lv2host.hpp lv2host.cpp \
	eventqueue.hpp eventqueue.cpp \
	main.cpp
elven_CFLAGS = `pkg-config --cflags jack gtkmm-2.4 lash-1.0 sigc++-2.0` -Iextensions/MidiPort -Iextensions/gtkgui -Iextensions/command -Ilibraries/paq -Ilibraries/components -I.
elven_LDFLAGS = `pkg-config --libs jack gtkmm-2.4 lash-1.0 sigc++-2.0` -lpthread
elven_ARCHIVES = libraries/paq/libpaq.a
elven_SOURCEDIR = programs/elven


# The plugins

PLUGINARCHIVES = libraries/lv2plugin/liblv2_plugin.a
PLUGINCFLAGS = -Ilibraries/lv2plugin -I.
ADVANCEDARCHIVES = libraries/lv2plugin/liblv2_plugin.a libraries/lv2plugin/liblv2_advanced.a
ADVANCEDCFLAGS = -Ilibraries/lv2plugin -I. -Iextensions/command

# Control2MIDI
control2midi_lv2_MODULES = control2midi.so
control2midi_so_SOURCES = control2midi.cpp
control2midi_so_CFLAGS = $(PLUGINCFLAGS) -Iextensions/MidiPort
control2midi_so_ARCHIVES = $(PLUGINARCHIVES)
control2midi_lv2_DATA = manifest.ttl control2midi.ttl
control2midi_lv2_SOURCEDIR = plugins/control2midi

# Phase distortion oscillator
phase-distortion-osc_lv2_MODULES = phase-distortion-osc.so
phase-distortion-osc_so_SOURCES = phase-distortion-osc.cpp
phase-distortion-osc_so_CFLAGS = $(PLUGINCFLAGS) -Ilibraries/components
phase-distortion-osc_so_ARCHIVES = $(PLUGINARCHIVES)
phase-distortion-osc_lv2_DATA = manifest.ttl phase-distortion-osc.ttl
phase-distortion-osc_lv2_SOURCEDIR = plugins/phase-distortion-osc

# Klaviatur
klaviatur_lv2_MODULES = klaviatur.so klaviatur_gtk.so
klaviatur_so_SOURCES = klaviatur.cpp
klaviatur_so_CFLAGS = $(PLUGINCFLAGS) -Iextensions/MidiPort -Ilibraries/components
klaviatur_so_ARCHIVES = $(PLUGINARCHIVES)
klaviatur_gtk_so_SOURCES = klaviatur_gtk.cpp
klaviatur_gtk_so_CFLAGS = `pkg-config --cflags gtkmm-2.4` -Iextensions/gtkgui -Ilibraries/lv2gtk2gui -Ilibraries/widgets -I.
klaviatur_gtk_so_LDFLAGS = `pkg-config --libs gtkmm-2.4` 
klaviatur_gtk_so_ARCHIVES = libraries/lv2gtk2gui/liblv2_gtk2gui.a libraries/widgets/libkeyboard.a
klaviatur_lv2_DATA = manifest.ttl klaviatur.ttl
klaviatur_lv2_PEGFILES = klaviatur.peg
klaviatur_lv2_SOURCEDIR = plugins/klaviatur

# Trilobyte
trilobyte_lv2_MODULES = trilobyte.so
trilobyte_so_SOURCES = trilobyte.cpp
trilobyte_so_CFLAGS = $(ADVANCEDCFLAGS) -Iextensions/MidiPort -Iextensions/instrument -Ilibraries/components
trilobyte_so_ARCHIVES = $(ADVANCEDARCHIVES)
trilobyte_gtk_so_SOURCES = trilobyte_gtk.cpp
trilobyte_gtk_so_CFLAGS = `pkg-config --cflags gtkmm-2.4` -Iextensions/gtkgui -Ilibraries/lv2gtk2gui -Ilibraries/widgets -Ilibraries/components -I.
trilobyte_gtk_so_LDFLAGS = `pkg-config --libs gtkmm-2.4`
trilobyte_gtk_so_ARCHIVES = libraries/lv2gtk2gui/liblv2_gtk2gui.a libraries/widgets/libpatternwidget.a
trilobyte_gtk_so_SOURCEDIR = plugins/trilobyte
trilobyte_lv2_DATA = manifest.ttl trilobyte.ttl patternbg.png
trilobyte_lv2_PEGFILES = trilobyte.peg
trilobyte_lv2_SOURCEDIR = plugins/trilobyte

# Trilobeat
trilobeat_lv2_MODULES = trilobeat.so
trilobeat_so_SOURCES = trilobeat.cpp
trilobeat_so_CFLAGS = $(ADVANCEDCFLAGS) -Iextensions/MidiPort -Iextensions/instrument -Ilibraries/components
trilobeat_so_ARCHIVES = $(ADVANCEDARCHIVES)
trilobeat_lv2_DATA = manifest.ttl trilobeat.ttl patternbg.png
trilobeat_lv2_PEGFILES = trilobeat.peg
trilobeat_lv2_SOURCEDIR = plugins/trilobeat
#trilobeat_lv2_MODULES = trilobeat_gtk.so
#trilobeat_gtk_so_SOURCES = trilobeat_gtk.cpp
#trilobeat_gtk_so_CFLAGS = `pkg-config --cflags gtkmm-2.4` -Iextensions/gtkgui -Ilibraries/lv2gtk2gui -Ilibraries/widgets -Ilibraries/components -I.
#trilobeat_gtk_so_LDFLAGS = `pkg-config --libs gtkmm-2.4`
#trilobeat_gtk_so_ARCHIVES = libraries/lv2gtk2gui/liblv2_gtk2gui.a libraries/widgets/libpatternwidget.a
#trilobeat_gtk_so_SOURCEDIR = plugins/trilobeat

# Euphoria
euphoria_lv2_MODULES = euphoria.so euphoria_gtk.so
euphoria_so_SOURCES = \
	euphoria.cpp \
	shaper.hpp shaper.cpp \
	envelope.hpp \
	pdoscillator.hpp \
	wsvoice.cpp wsvoice.hpp \
	pdvoice.hpp pdvoice.cpp \
	chorus.hpp chorus.cpp \
	echo.hpp echo.cpp \
	reverb.hpp
euphoria_so_CFLAGS = $(ADVANCEDCFLAGS) -Ilibraries/components -Iextensions/MidiPort `pkg-config --cflags gsl`
euphoria_so_ARCHIVES = $(ADVANCEDARCHIVES)
euphoria_so_LDFLAGS = `pkg-config --libs gsl`
euphoria_gtk_so_SOURCES = \
	euphoria_gtk.cpp \
	euphoriawidget.cpp euphoriawidget.hpp
euphoria_gtk_so_CFLAGS = `pkg-config --cflags gtkmm-2.4 cairomm-1.0` -Iextensions/gtkgui -Ilibraries/widgets -Ilibraries/lv2gtk2gui -I.
euphoria_gtk_so_LDFLAGS = `pkg-config --libs gtkmm-2.4 cairomm-1.0` 
euphoria_gtk_so_ARCHIVES = \
	libraries/widgets/libvgknob.a \
	libraries/widgets/libenvelopeeditor.a \
	libraries/widgets/libshapereditor.a \
	libraries/widgets/libspringeditor.a \
	libraries/widgets/libpdeditor.a \
	libraries/widgets/libtransitioneditor.a \
	libraries/lv2gtk2gui/liblv2_gtk2gui.a
euphoria_lv2_DATA = manifest.ttl euphoria.ttl presets.ttl
euphoria_lv2_PEGFILES = euphoria.peg
euphoria_lv2_SOURCEDIR = plugins/euphoria

# Horizon
horizon_lv2_MODULES = horizon.so horizon_gtk.so
horizon_so_SOURCES = \
	action.hpp action.cpp \
	actiontrigger.hpp actiontrigger.cpp \
	chunk.hpp chunk.cpp \
	effect.hpp effect.cpp \
	effectstack.hpp effectstack.cpp \
	horizon.cpp \
	mixer.hpp mixer.cpp \
	normaliseeffect.hpp normaliseeffect.cpp \
	reverseeffect.hpp reverseeffect.cpp \
	sample.hpp sample.cpp \
	samplebuffer.hpp samplebuffer.cpp \
	segmentation.hpp segmentation.cpp \
	voice.hpp voice.cpp
horizon_so_CFLAGS = `pkg-config --cflags sndfile` $(ADVANCEDCFLAGS) -Ilibraries/components -Iextensions/command -Iextensions/MidiPort
horizon_so_ARCHIVES = $(ADVANCEDARCHIVES)
horizon_so_LDFLAGS = `pkg-config --libs sndfile`
horizon_gtk_so_SOURCES = \
	chunkeditor.cpp chunkeditor.hpp \
	controlsourcegui.cpp controlsourcegui.hpp \
	effectmodel.cpp effectmodel.hpp \
	effectstackmodel.cpp effectstackmodel.hpp \
	effectstackview.cpp effectstackview.hpp \
	labelslider.cpp labelslider.hpp \
	lfo_gui.cpp lfo_gui.hpp \
	mod_gui.cpp mod_gui.hpp \
	optionslider.cpp optionslider.hpp \
	sampleeditor.cpp sampleeditor.hpp \
	samplemodel.cpp samplemodel.hpp \
	sampleview.cpp sampleview.hpp \
	triggereditor.cpp triggereditor.hpp \
	horizon_gtk.cpp
horizon_gtk_so_CFLAGS = `pkg-config --cflags gtkmm-2.4 cairomm-1.0` -Iextensions/gtkgui -Ilibraries/widgets -Ilibraries/lv2gtk2gui -I.
horizon_gtk_so_LDFLAGS = `pkg-config --libs gtkmm-2.4 cairomm-1.0` 
horizon_gtk_so_ARCHIVES = \
	libraries/lv2gtk2gui/liblv2_gtk2gui.a
horizon_lv2_DATA = manifest.ttl horizon.ttl
horizon_lv2_PEGFILES = horizon.peg
horizon_lv2_SOURCEDIR = plugins/horizon

# Sineshaper
sineshaper_lv2_MODULES = sineshaper.so
sineshaper_so_SOURCES = \
	sineshaper.hpp sineshaper.cpp \
	sineshaperports.hpp \
	midiiterator.hpp
sineshaper_so_CFLAGS = $(PLUGINCFLAGS) -Ilibraries/components -Iextensions/MidiPort -Iextensions/instrument
sineshaper_so_ARCHIVES = $(PLUGINARCHIVES)
#sineshaper_lv2_MODULES = sineshaper_gtk.so
sineshaper_gtk_so_SOURCES = \
	sineshaper_gtk.cpp \
	sineshaperwidget.cpp sineshaperwidget.hpp
sineshaper_gtk_so_CFLAGS = `pkg-config --cflags gtkmm-2.4` -Iextensions/gtkgui -Ilibraries/widgets -Ilibraries/lv2gtk2gui -I.
sineshaper_gtk_so_LDFLAGS = `pkg-config --libs gtkmm-2.4` 
sineshaper_gtk_so_SOURCEDIR = plugins/sineshaper
#sineshaper_lv2_PROGRAMS = sineshaper_gtk
#sineshaper_gtk_SOURCES = sineshapergui.cpp sineshapergui.hpp main.cpp skindial_gtkmm.hpp skindial_gtkmm.cpp
#sineshaper_gtk_CFLAGS = `pkg-config --cflags gtkmm-2.4 libglademm-2.4 liblo` -Ilibraries/lv2oscui -Ilibraries/components -Iextensions/instrument -I.
#sineshaper_gtk_LDFLAGS = `pkg-config --libs gtkmm-2.4 gthread-2.0 liblo libglademm-2.4` libraries/lv2oscui/liblv2_oscui.a
#sineshaper_gtk_SOURCEDIR = plugins/sineshaper
sineshaper_lv2_DATA = manifest.ttl sineshaper.ttl sineshaper.glade dial.png sineshaper.png presets.ttl icon.svg
sineshaper_lv2_SOURCEDIR = plugins/sineshaper

# MIDIIdentity
midi_identity_lv2_MODULES = midi_identity.so
midi_identity_so_SOURCES = midi_identity.cpp
midi_identity_so_CFLAGS = $(PLUGINCFLAGS) -Iextensions/MidiPort -Ilibraries/components
midi_identity_so_ARCHIVES = $(PLUGINARCHIVES)
midi_identity_lv2_DATA = manifest.ttl midi_identity.ttl
midi_identity_lv2_SOURCEDIR = plugins/midi_identity

# Householder FDN
#hhfdn_lv2_SOURCES = hhfdn.cpp
#hhfdn_lv2_DATA = manifest.ttl hhfdn.ttl
#hhfdn_lv2_CFLAGS = -Ilibraries/lv2plugin -Iextensions/MidiPort -Ilibraries/components -I.
#hhfdn_lv2_ARCHIVES = $(PLUGINARCHIVES)
#hhfdn_lv2_SOURCEDIR = plugins/householderfdn

# AudioIdentity
audio_identity_lv2_MODULES = audio_identity.so
audio_identity_so_SOURCES = audio_identity.cpp
audio_identity_so_CFLAGS = $(PLUGINCFLAGS)
audio_identity_so_ARCHIVES = $(PLUGINARCHIVES)
audio_identity_lv2_DATA = manifest.ttl audio_identity.ttl
audio_identity_lv2_SOURCEDIR = plugins/audio_identity

# Arpeggiator
arpeggiator_lv2_MODULES = arpeggiator.so
arpeggiator_so_SOURCES = arpeggiator.cpp
arpeggiator_so_CFLAGS = $(PLUGINCFLAGS) -Iextensions/MidiPort
arpeggiator_so_ARCHIVES = $(PLUGINARCHIVES)
arpeggiator_lv2_DATA = manifest.ttl arpeggiator.ttl
arpeggiator_lv2_SOURCEDIR = plugins/arpeggiator

# Math constants
math-constants_lv2_MODULES = math-constants.so
math-constants_so_SOURCES = math-constants.cpp
math-constants_so_CFLAGS = $(PLUGINCFLAGS)
math-constants_so_ARCHIVES = $(PLUGINARCHIVES)
math-constants_lv2_DATA = manifest.ttl math-constants.ttl
math-constants_lv2_SOURCEDIR = plugins/math-constants

# Math functions
math-functions_lv2_MODULES = math-functions.so
math-functions_so_SOURCES = math-functions.cpp
math-functions_so_CFLAGS = $(PLUGINCFLAGS)
math-functions_so_ARCHIVES = $(PLUGINARCHIVES)
math-functions_lv2_DATA = manifest.ttl math-functions.ttl
math-functions_lv2_SOURCEDIR = plugins/math-functions


# The shared headers need to go in the distribution too
EXTRA_DIST = COPYING \
	\
	lv2.h \
	\
	libraries/components/adsr.hpp \
	libraries/components/chebyshevshaper.hpp \
	libraries/components/dcblocker.hpp \
	libraries/components/delay.hpp \
	libraries/components/distortion.hpp \
	libraries/components/envelopegenerator.hpp \
	libraries/components/frequencytable.hpp \
	libraries/components/ladspawrapper.hpp \
	libraries/components/lv2-midifunctions.h \
	libraries/components/monophonicmidinote.hpp \
	libraries/components/monostep.hpp \
	libraries/components/mooglpf.hpp \
	libraries/components/pdosc.hpp \
	libraries/components/polyphonicmidinote.hpp \
	libraries/components/programmanager.hpp \
	libraries/components/randomsineoscillator.hpp \
	libraries/components/ringbuffer.hpp \
	libraries/components/sineoscillator.hpp \
	libraries/components/slide.hpp \
	libraries/components/voicehandler.hpp \
	libraries/components/wavewrapper.hpp \
	\
	extensions/command/lv2-command.h \
	extensions/MidiPort/lv2-midiport.h \
	extensions/midimap/lv2-midimap.rdfs \
	extensions/instrument/lv2-instrument.h \
	extensions/transporttype/lv2-transport.h \
	extensions/gtkgui/lv2-gtk2gui.h \
	extensions/gtkgui/lv2-guicomm-gtk2gui.h \
	extensions/gtkgui/lv2-instrument-gtk2gui.h \
	extensions/gtkgui/lv2-program-gtk2gui.h \
	extensions/gtkgui/lv2-miditype-gtk2gui.h 



# Do the magic
include Makefile.template
