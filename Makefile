PLUGIN_NAME = g_waveform

HEADERS = g-waveform.h\
			 /usr/local/lib/rtxi_includes/basicplot.h\
			 /usr/local/lib/rtxi_includes/scrollbar.h\
			 /usr/local/lib/rtxi_includes/scrollzoomer.h\
			 /usr/local/lib/rtxi_includes/plotdialog.h\

SOURCES = g-waveform.cpp \
          moc_g-waveform.cpp\
			 /usr/local/lib/rtxi_includes/basicplot.cpp\
			 /usr/local/lib/rtxi_includes/scrollbar.cpp\
			 /usr/local/lib/rtxi_includes/scrollzoomer.cpp\
			 /usr/local/lib/rtxi_includes/plotdialog.cpp\
			 /usr/local/lib/rtxi_includes/moc_basicplot.cpp\
			 /usr/local/lib/rtxi_includes/moc_scrollbar.cpp\
			 /usr/local/lib/rtxi_includes/moc_scrollzoomer.cpp\
			 /usr/local/lib/rtxi_includes/moc_plotdialog.cpp\

LIBS = -lqwt

### Do not edit below this line ###

include $(shell rtxi_plugin_config --pkgdata-dir)/Makefile.plugin_compile
