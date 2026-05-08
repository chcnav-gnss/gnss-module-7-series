#Makefile for simple programs

#28
#CC = arm-none-linux-gnueabi-gcc
#ixm6
#(1)source  /usr/sbin/source /opt/imx6/environment-setup-cortexa7hf-vfp-neon-poky-linux-gnueabi
#(2)make
CFLAGS = -Wall -g -std=gnu99

TARGET  = update_m7xx
OBJS 	= md5.o CRCGenerate.o main.o UartCommon.o UpgradeProtocol.o UpgradeEntry.o UpgradeM7xx.o

all: $(TARGET)

$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
	
%.o:%.c
	$(CC) $(CFLAGS) -c $^ -o $@

.PHONY:
	clean
clean:
	rm *.o $(TARGET)

