PLUGIN_NAME = Gwaveform

HEADERS = Gwaveform.h\
    include/basicplot.h\
    include/scrollbar.h\
    include/scrollzoomer.h\
	 include/plotdialog.h

SOURCES = Gwaveform.cpp \
    moc_Gwaveform.cpp\
    include/basicplot.cpp\
    include/scrollbar.cpp\
    include/scrollzoomer.cpp\
	 include/plotdialog.cpp\
    include/moc_scrollbar.cpp\
    include/moc_scrollzoomer.cpp\
    include/moc_basicplot.cpp\
	 include/moc_plotdialog.cpp

LIBS = -lqwt

### Do not edit below this line ###

include $(shell rtxi_plugin_config --pkgdata-dir)/Makefile.plugin_compile
