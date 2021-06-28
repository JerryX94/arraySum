OBJDIR	 = ./build
OBJECTS	 = $(OBJDIR)/main.o $(OBJDIR)/sum.o $(OBJDIR)/slave.o
INCLUDES = -I./inc -I/home/export/online3/swmore/opensource/swlu/include/
LIBS	 = -L/home/export/online3/swmore/opensource/swlu/lib/ -lswlu
FLAGS	 = -O3
SFLAGS	 = -O3
VPATH	 = ./src

all: slave.o sum.o main.o arraySum

arraySum: $(OBJECTS) | dir
	mpicc -hybrid -o $(OBJDIR)/arraySum $^ $(LIBS)
main.o: main.c | dir
	mpicc $(FLAGS) $(INCLUDES) -c $< -o $(OBJDIR)/main.o
sum.o: sum.c | dir
	sw5cc -host $(FLAGS) $(INCLUDES) -c -std=gnu99 $< -o $(OBJDIR)/sum.o
slave.o: slave.c | dir
	sw5cc -slave -msimd $(SFLAGS) $(INCLUDES) -c -std=gnu99 $< -o $(OBJDIR)/slave.o

dir:
	@mkdir -p $(OBJDIR)

.PHONY: clean
clean:
	rm -rf ./build
