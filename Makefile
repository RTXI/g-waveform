PLUGIN_NAME = g_waveform

RTXI_INCLUDES =

HEADERS = g-waveform.h\

SOURCES = g-waveform.cpp \
          moc_g-waveform.cpp\

LIBS = -lqwt-qt5 -lrtplot

### Do not edit below this line ###

include $(shell rtxi_plugin_config --pkgdata-dir)/Makefile.plugin_compile
