# Solder Reflow Oven

ELF = reflow.elf
OBJS = oven.o reflow.o pid.o temp.o uart.o

MMCU = atxmega32a4u
PROGDEV = atmelice_pdi

all: $(ELF)

download: $(ELF)
	sudo avrdude -c $(PROGDEV) -p x32a4u -U flash:w:$<

$(ELF): $(OBJS)
	avr-gcc -mmcu=$(MMCU) -o $@ $^

%.o: %.c
	avr-gcc -mmcu=$(MMCU) -D USE_LCD_DISPLAY -c -o $@ $<

clean:
	rm -rf *.o $(ELF)
