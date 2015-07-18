include rules.mk

TARGET_LIB = $(BIN_DIR)/s14-traffic-steering-app.so
TARGET_ULIB = $(BIN_DIR)/s16-tsc-cli.so

All: makesrc maketest maketarget makeucm makeutarget

makesrc: 
	make -w -C $(tscapp_dir)/src

makeucm: 
	make -w -C $(tscapp_dir)/ucm

maketest: 
	make -w -C $(tscapp_dir)/test

maketarget:$(TARGET_LIB)
makeutarget:$(TARGET_ULIB)

OBJS = $(BIN_DIR)/traffic_steering-app.o
UCM_OBJS =  $(BIN_DIR)/ucmtsccbk.o
OBJS += $(BIN_DIR)/traffic_steering-test.o

$(TARGET_LIB) : $(OBJS)
	$(CC) -shared -o $@ $(OBJS) -lm

$(TARGET_ULIB) : $(UCM_OBJS)
	$(CC) -shared -o $@ $(UCM_OBJS) -lm

clean:
	make -C $(tscapp_dir)/src clean
	make -C $(tscapp_dir)/test clean
	make -C $(tscapp_dir)/ucm clean
	$(RM) $(TARGET_LIB) $(TARGET_ULIB)

install:
	sudo cp $(TARGET_LIB) $(ondir_platform_inst_dir)/lib/apps 
	sudo cp $(TARGET_ULIB) $(ondir_platform_inst_dir)/lib/apps 
	sudo cp tsc.conf $(ondir_platform_inst_dir)/lib/apps 
