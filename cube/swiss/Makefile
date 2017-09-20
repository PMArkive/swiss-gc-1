#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif

include $(DEVKITPPC)/gamecube_rules

	
#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:=	swiss
BUILD		:=	build
SOURCES		:=  source \
              source/aram \
			  source/fonts \
			  source/gui \
			  source/config \
			  source/crc32 \
              source/devices \
              source/devices/system \
              source/devices/dvd \
              source/devices/fat \
			  source/devices/ftp \
              source/devices/memcard \
              source/devices/qoob \
			  source/devices/smb \
			  source/devices/usbgecko \
              source/devices/wode \
			  source/devices/wiikeyfusion \
              source/elmchan \
              source/elmchan/option \
              source/cheats \
              source/patches \
			  source/patches/card \
			  source/patches/misc \
			  source/patches/gx \
			  source/patches/mtx \
			  source/patches/vi \
              source/images \
			  source/images/buttons \
			  source/libwiigui \
			  source/sounds
DATA		:=	data  
TEXTURES	:=	source/images \
              source/images/buttons
INCLUDES	:= include $(SOURCES)

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------

CFLAGS		= 	-g -O3 -Wall -fno-strict-aliasing \
				$(MACHDEP) $(INCLUDE) -DNO_SOUND \
				-D'GITREVISION="$(shell git rev-parse --short HEAD)"' \
				-D'GITVERSION="$(shell git rev-list HEAD | wc -l)"'
CXXFLAGS	= $(CFLAGS)

LDFLAGS		= -g $(MACHDEP) -Wl,-Map,$(notdir $@).map -Wl,--section-start,.init=0x80003100

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS	:=	-lpng -lz -lbba -logc -lm -lfat -lmad -lasnd -lm -logc -ltinysmb -ldb -lfreetype

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= $(PORTLIBS)

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir)) \
					$(foreach dir,$(TEXTURES),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

#---------------------------------------------------------------------------------
# automatically build a list of object files for our project
#---------------------------------------------------------------------------------
CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
sFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))
PNGFILES	:=	$(foreach dir,$(TEXTURES),$(notdir $(wildcard $(dir)/*.png)))
BMPFILES	:=	$(foreach dir,$(TEXTURES),$(notdir $(wildcard $(dir)/*.bmp)))
SCFFILES	:=	$(foreach dir,$(BUILD),$(notdir $(wildcard $(dir)/*.scf)))
TPLFILES	:=	$(SCFFILES:.scf=.tpl)
TTFFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.ttf)))
LANGFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.lang)))
OGGFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.ogg)))
PCMFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.pcm)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif


export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
					$(addsuffix .o,$(PNGFILES)) \
					$(addsuffix .o,$(BMPFILES)) \
					$(sFILES:.s=.o) $(SFILES:.S=.o) \
					$(TTFFILES:.ttf=.ttf.o) $(LANGFILES:.lang=.lang.o) \
					$(OGGFILES:.ogg=.ogg.o) $(PCMFILES:.pcm=.pcm.o) \
					$(CPPFILES:.cpp=.o) $(CFILES:.c=.o)

#---------------------------------------------------------------------------------
# build a list of include paths
#---------------------------------------------------------------------------------
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD) \
			-I$(LIBOGC_INC) -I$(PORTLIBS)/include/freetype2

#---------------------------------------------------------------------------------
# build a list of library paths
#---------------------------------------------------------------------------------
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib) -L$(LIBOGC_LIB)

export OUTPUT	:=	$(CURDIR)/$(TARGET)
.PHONY: $(BUILD) clean

#---------------------------------------------------------------------------------
$(BUILD):
	@echo Current Directory $(CURDIR)
	@echo GIT Revision $(shell git rev-parse --short HEAD)
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile RUNDIR=$(CURDIR)

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(OUTPUT).elf $(OUTPUT).dol
#---------------------------------------------------------------------------------
run:	
	$(DEVKITPRO)/emulators/gcube/gcube $(OUTPUT).dol

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).dol: $(OUTPUT).elf

$(OUTPUT).elf: $(OFILES)

%.ttf.o : %.ttf
	@echo $(notdir $<)
	$(bin2o)

%.lang.o : %.lang
	@echo $(notdir $<)
	$(bin2o)

%.dol: %.elf
	@echo output ... $(notdir $@)
	@elf2dol $< $@

%.png.o: %.png
	@echo $(notdir $<)
	@cp $< $(CURDIR)/$(notdir $<)
	@echo "<filepath=\"$(notdir $<)\" id=\"$(notdir $(basename $<))\" colfmt=6 />" > $(CURDIR)/$(notdir $(basename $<)).scf
	@gxtexconv -s $(CURDIR)/$(notdir $(basename $<)).scf -d $(CURDIR)/$(notdir $(basename $<)).d -o $(CURDIR)/$(notdir $(basename $<)).tpl
	@bin2s -a 32 $(CURDIR)/$(notdir $(basename $<)).tpl | $(AS) -o $(basename $(@)).o
	@echo "extern const u8" `(echo $(notdir $(basename $<)) | sed -e 's/^\([0-9]\)/_\1/')`"_tpl_end[];" > `(echo $(CURDIR)/$(notdir $(basename $<))_tpl)`.h
	@echo "extern const u8" `(echo $(notdir $(basename $<)) | sed -e 's/^\([0-9]\)/_\1/')`"_tpl[];" >> `(echo $(CURDIR)/$(notdir $(basename $<))_tpl)`.h
	@echo "extern const u32" `(echo $(notdir $(basename $<)) | sed -e 's/^\([0-9]\)/_\1/')`_tpl_size";" >> `(echo $(CURDIR)/$(notdir $(basename $<))_tpl)`.h
	@rm $(CURDIR)/$(notdir $<)
	@rm $(CURDIR)/$(notdir $(basename $<)).tpl
	@rm $(CURDIR)/$(notdir $(basename $<)).scf

%.bmp.o: %.bmp
	@echo $(notdir $<)
	@cp $< $(CURDIR)/$(notdir $<)
	@echo "<filepath=\"$(notdir $<)\" id=\"$(notdir $(basename $<))\" colfmt=1 />" > $(CURDIR)/$(notdir $(basename $<)).scf
	@gxtexconv -s $(CURDIR)/$(notdir $(basename $<)).scf -d $(CURDIR)/$(notdir $(basename $<)).d -o $(CURDIR)/$(notdir $(basename $<)).tpl
	@bin2s -a 32 $(CURDIR)/$(notdir $(basename $<)).tpl | $(AS) -o $(basename $(@)).o
	@echo "extern const u8" `(echo $(notdir $(basename $<)) | sed -e 's/^\([0-9]\)/_\1/')`"_tpl_end[];" > `(echo $(CURDIR)/$(notdir $(basename $<))_tpl)`.h
	@echo "extern const u8" `(echo $(notdir $(basename $<)) | sed -e 's/^\([0-9]\)/_\1/')`"_tpl[];" >> `(echo $(CURDIR)/$(notdir $(basename $<))_tpl)`.h
	@echo "extern const u32" `(echo $(notdir $(basename $<)) | sed -e 's/^\([0-9]\)/_\1/')`_tpl_size";" >> `(echo $(CURDIR)/$(notdir $(basename $<))_tpl)`.h
	@rm $(CURDIR)/$(notdir $<)
	@rm $(CURDIR)/$(notdir $(basename $<)).tpl
	@rm $(CURDIR)/$(notdir $(basename $<)).scf

%.elf:
	@echo linking ... $(notdir $@)
	@$(LD)  $^ $(LDFLAGS) $(LIBPATHS) $(LIBS) -o $@

%.ogg.o : %.ogg
	@echo $(notdir $<)
	$(bin2o)

%.pcm.o : %.pcm
	@echo $(notdir $<)
	$(bin2o)
	
-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
