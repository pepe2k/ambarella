

default: all

include $(MW_CE_TOPDIR)/buildspec.mk
include $(MW_CE_TOPDIR)/amf/build/core/buildflags.mk
include $(MW_CE_TOPDIR)/config.mk

G711_CFLAG := -I$(MW_CE_TOPDIR)/amf/extern/g711
G711_ARFLAG := rucs

.PHONY: all clean

objects = g711_codec.o

all: libg711.a

$(objects): %.o: %.cpp
	$(CXX) $(AMF_CFLAG) -c $< -o $@

libg711.a: $(objects)
	@$(AMBA_MAKEFILE_V)echo "    compile libg711.a"
	$(RM) libg711.a
	$(AMBA_MAKEFILE_V)$(AR) $(G711_ARFLAG) libg711.a  $(objects)
	@ranlib libg711.a

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean compile libg711]:"
	-rm *.a *.o