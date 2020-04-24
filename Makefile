#AVRDUDE_MCU = m328p
AVRDUDE_MCU = t13
AVRDUDE_PROGRAMMER = usbasp
#AVRDUDE_PORT = /dev/ttyACM0
AVRDUDE_PORT = 
AVRDUDE_BAUDRATE = 115200

AVRDUDE = avrdude
AVRDUDEOPTS =   -p $(AVRDUDE_MCU) \
        -c $(AVRDUDE_PROGRAMMER) \
        -P $(AVRDUDE_PORT) \
        -b $(AVRDUDE_BAUDRATE) \
        -F -D

#FREQUENCY = 16500000
FREQUENCY = 1200000
MCU = attiny13
#MCU = attiny85

CC = avr-gcc
#CFLAGS = -mmcu=$(MCU) -O2 -DF_CPU=$(FREQUENCY)L -Wall
CFLAGS = -mmcu=$(MCU) -Os -DF_CPU=$(FREQUENCY)L -Wall
#CFLAGS += -fdata-sections -ffunction-sections
OBJCOPY = avr-objcopy
OBJCOPYFLAGS = -j .text -j .data -O ihex 
PROJNAME = rc5
OBJECTS = main.o rc5.o
LD = avr-gcc
LDFLAGS = -mmcu=$(MCU)
#LDFLAGS += -Wl,-gc-sections

all: $(PROJNAME).hex

$(PROJNAME).elf: $(OBJECTS) 
	$(LD) $(LDFLAGS) $(OBJECTS) -o $@
$(PROJNAME).hex: $(PROJNAME).elf  
	$(OBJCOPY)  $(OBJCOPYFLAGS) $< $@
	avr-size -C --format=avr --mcu=$(MCU) $(PROJNAME).elf
	avr-objdump -DS $(PROJNAME).elf > assembler.lst

%.o: %.c %.h $(CC) $(CFLAGS) -c $< -o $@

flash:	all
	avrdude -c usbasp -p ${AVRDUDE_MCU} ${AVRDUDE_PORT} -U flash:w:$(PROJNAME).hex		

rebuild: clean all

clean:
	$(RM) $(PROJNAME) *.o *.lst *.elf *.hex *~

.PHONY: clean rebuild
