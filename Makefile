CC = gcc

LIBS = -lpthread\
	/home/jun/Documents/unpv13e/libunp.a\

FLAGS = -g -O2

CFLAGS = ${FLAGS} -I/home/jun/Documents/unpv13e/lib\


OBJ = tour_func.o mul_func.o ping_func.o get_hw_addrs.o addr2name.o sendPing.o

all: tour_module ping ping4_ll ${OBJ} 

tour_module: tour_module.o ${OBJ}
	${CC} ${FLAGS} -o tour_module tour_module.o ${OBJ} ${LIBS}

ping: ping.o ${OBJ}
	${CC} ${FLAGS} -o ping ping.o ${OBJ} ${LIBS}

ping4_ll: ping4_ll.o ${OBJ}
	${CC} ${FLAGS} -o ping4_ll ping4_ll.o ${OBJ} ${LIBS}

%.o: %.c
	${CC} ${CFLAGS} -c $^

clean:
	rm tour_module ping ping4_ll *.o *~
