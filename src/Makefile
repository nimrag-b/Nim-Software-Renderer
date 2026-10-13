#---------------------------------------------------------------------------------
# SR makefiile
#---------------------------------------------------------------------------------

ifeq ($(OS),Windows_NT)
    MKDIR := mkdir
	RM := rmdir /s /q
# some windows cmds like rmdir get funny with forward slashes, so use backslash to make sure
	OBJDIR := ..\obj
	SRC_PLATFORM := platform/SR_platform_win32.c
else
    MKDIR := mkdir
	RM := rm -r
	OBJDIR := ../obj
endif

CC := gcc
CFLAGS := -g -Wall -Wextra -Werror

INCLUDEDIR := ../include

HEADER := SR.h SR_type.h SR_matrix.h SR_platform.h
SRC := SR_matrix.c SR_renderer.c $(SRC_PLATFORM)
OBJ := $(patsubst %.c,%.o,$(SRC))

.PHONY: all clean generateheader

#---------------------------------------------------------------------------------
# da rulez
#---------------------------------------------------------------------------------

dummy: generateheader
	$(info Please Specify target)

tests: generateheader ${OBJDIR} $(OBJ) 
	$(CC) $(CFLAGS) -I$(INCLUDEDIR) $(patsubst %, ${OBJDIR}/%,$(notdir $(OBJ))) ../tests/windows/main.c -lGdi32 -o ../tests/windows/sr

$(OBJ): %.o: %.c
	$(CC) $(CFLAGS) -I$(INCLUDEDIR) -c $^ -o ${OBJDIR}/$(notdir $@)

${OBJDIR}: 
	$(MKDIR) ${OBJDIR}

generateheader:
	..\gen_header_only.bat

clean:
	$(RM) $(OBJDIR)
