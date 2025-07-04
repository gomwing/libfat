#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/ds_rules

	export CC	:=	V:\\_DEVELOP_\\devkitPro\\msys2\\mingw32\\bin\\gcc.exe
	export CXX	:=	V:\\_DEVELOP_\\devkitPro\\msys2\\mingw32\\bin\\g++.exe

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:=	$(shell basename $(CURDIR))
BUILD		:=	build
SOURCES		:=	source testbench
INCLUDES	:=	include build

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
#ARCH	:=	-march=armv5te -mtune=arm946e-s -mthumb

CFLAGS	:=	-g -Wall -O0 -ffunction-sections -fdata-sections -Wno-return-type\
-IV:/_DEVELOP_/devkitPro/msys2/mingw32/i686-w64-mingw32/include\
-IV:/_DEVELOP_/devkitPro/msys2/mingw32/lib/gcc/i686-w64-mingw32/15.1.0/include\
-IV:/_DEVELOP_/devkitPro/msys2/mingw32/lib/gcc/i686-w64-mingw32/15.1.0/include/c++/i686-w64-mingw32
#-IV:/_DEVELOP_/devkitPro/msys2/mingw32/lib/gcc/i686-w64-mingw32/15.1.0/include/c++/tr1\

#V:\_DEVELOP_\devkitPro\msys2\mingw32\lib\gcc\i686-w64-mingw32\15.1.0\include\c++\i686-w64-mingw32\bits
#requires_hosted
#V:/DEVELOP_/devkitPro/msys2/mingw32/i686-w64-mingw32/include
#-Iv:/_DEVELOP_/devkitPro.old/libnds/include
#			$(ARCH)

CFLAGS	+=	$(INCLUDE) -DARM9
CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS	:=	-g $(ARCH)
#LDFLAGS	=	-specs=ds_arm9.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
LDFLAGS	=	-g $(ARCH) -Wl,-Map,$(notdir $*.map) 
#-static-libgcc -static-libstdc++

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS	:= #-lnds9


#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:=	$(LIBNDS)

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.bin)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES	:=	$(BINFILES:.bin=.o) \
					$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean

#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).exe 


#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
#$(OUTPUT).exe	: 	$(OUTPUT).elf
$(OUTPUT).exe	:	$(OFILES)

#---------------------------------------------------------------------------------
%.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	$(bin2o)

#---------------------------------------------------------------------------------
#%.exe	:	%.o
#---------------------------------------------------------------------------------
#	@echo $(notdir $<)
#	$(bin2o)
#%.o: %.c
#	$(SILENTMSG) $(notdir $<)
#	$(ADD_COMPILE_COMMAND) add $(CC) "$(_EXTRADEFS) $(CPPFLAGS) $(CFLAGS) -c $< -o $@" $<
#	$(SILENTCMD)$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d $(_EXTRADEFS) $(CPPFLAGS) $(CFLAGS) -c $< -o $@ $(ERROR_FILTER)
#	$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d $(_EXTRADEFS) $(CPPFLAGS) $(CFLAGS) -c $< -o $@ $(ERROR_FILTER)

#---------------------------------------------------------------------------------
#%.elf	:	%.exe
#---------------------------------------------------------------------------------
#	@echo $(notdir $<)
#	rename $(notdir $<) $(notdir $<).elf
%.exe:
	$(SILENTMSG) linking $(notdir $@)
	$(ADD_COMPILE_COMMAND) end
	$(SILENTCMD)$(LD) $(LIBPATHS) $(_LDFLAGS) $(OFILES) $(LIBS) $(_EXTRALIBS) -o $@
	#$(SILENTCMD)$(NM) -CSn $@ > $(notdir $*.lst)

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
