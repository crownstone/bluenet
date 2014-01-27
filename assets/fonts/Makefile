# Makefile to generate bitmap versions of fonts at different sizes.

FONTS := $(wildcard *.ttf)
SIZES = 07 08 09 10 11 12 14 16 18 24 28 32 46 64 96
COMPRESSED=   #_compressed
PFOS := $(foreach font,$(FONTS),$(foreach size, $(SIZES), $(font:.ttf=)_$(size)$(COMPRESSED).pfo))
HEADERS = $(PFOS:.pfo=.h)

all: $(HEADERS)

bin2c:
	$(CC) -Wno-format-security -o bin2c bin2c.c

# template magic, see http://stackoverflow.com/a/3196823/1409535
define PFO_template

%_$(1)$(COMPRESSED).pfo: %.ttf
	python2.7 fontgen.py pfo --rle $(if $(strip $(COMPRESSED)),true,false) --filter '[ -~\xE9]' $(1) $$< $$@

endef

templates = $(foreach size, $(SIZES), $(call PFO_template,$(size)))

# $(warning templates: $(templates))

$(eval $(templates))
	
%.h: %.pfo bin2c
	bin2c -o $@ $<

clean:
	rm -f *.o *.h *.pfo bin2c
	

.SECONDARY: # don't delete intermediates
	#nothing
