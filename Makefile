OBJDIR   = ./build
OBJECTS  = $(OBJDIR)/main.o $(OBJDIR)/sum.o $(OBJDIR)/slave.o
INCLUDES = -I ./inc
LIBS	 = 
FLAGS	 = -O2
VPATH    = ./src

all: slave.o sum.o main.o arraySum

arraySum: $(OBJECTS) | dir
	sw5cc -hybrid -o $(OBJDIR)/arraySum $^ $(LIBS)
main.o: main.c | dir
	sw5cc -host $(INCLUDES) -c $< -o $(OBJDIR)/main.o
sum.o: sum.c | dir
	sw5cc -host $(INCLUDES) -c -std=gnu99 $< -o $(OBJDIR)/sum.o
slave.o: slave.c | dir
	sw5cc -slave -msimd $(FLAGS) $(INCLUDES) -c -std=gnu99 $< -o $(OBJDIR)/slave.o

dir:
	@mkdir -p $(OBJDIR)

.PHONY: clean
clean:
	rm -rf ./build
