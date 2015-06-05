include ../rules.mk

$(BIN_DIR)/%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

TARGET = $(BIN_DIR)/traffic_steering-test.o

all: $(TARGET)

OBJS =  $(BIN_DIR)/crm_test.o\
        $(BIN_DIR)/nsrm_test.o\

$(TARGET) : $(OBJS) 
	$(LD) -r $(OBJS) -o $(TARGET) 

clean:
	$(RM) $(TARGET)
	$(RM) $(OBJS)       

