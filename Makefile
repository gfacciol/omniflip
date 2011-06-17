all: v.cpp libiio/iio.o
	g++ -O3 -Wall -L/usr/lib -L/usr/X11R6/lib -Ilibiio -lpng -ljpeg -ltiff -lX11 -lz -lm -ldl -lpthread libiio/iio.o v.cpp -o vflip 

libiio/iio.o: libiio/iio.c 
	c99 -O3 -DNDEBUG -c libiio/iio.c  -o libiio/iio.o

install: vflip
	cp vflip ~/bin/


clean:
	rm libiio/iio.o vflip


EXRINC=-I/usr/local/include/OpenEXR/
EXRLIB=-L/usr/local/lib/ -lIex -lHalf -lIlmImf

EXR:
	c99 -DNDEBUG -DI_CAN_HAS_LIBEXR -O3 -c libiio/iio.c ${EXRINC} -o libiio/iio.o
#	g++ -DI_CAN_HAS_LIBEXR -c -Wall -Ilibiio ${EXRINC} libiio/read_exr_float.cpp  -o libiio/read_exr_float.o
#	g++ -DI_CAN_HAS_LIBEXR -O3 -Wall -L/usr/lib -L/usr/X11R6/lib -Ilibiio -lpng -ljpeg -ltiff -lX11 -lz -lm -ldl -lpthread libiio/read_exr_float.o libiio/iio.o  ${EXRLIB}  v.cpp   -o vflip
	g++ -DI_CAN_HAS_LIBEXR -O3 -Wall -L/usr/lib -L/usr/X11R6/lib -Ilibiio -lpng -ljpeg -ltiff -lX11 -lz -lm -ldl -lpthread libiio/iio.o  ${EXRLIB}  v.cpp   -o vflip
	cp vflip ~/bin/

zip:
	zip omniflip2.zip  ../omniflip/*.h ../omniflip/v.cpp ../omniflip/libiio/* ../omniflip/Makefile ../omniflip/samples/*
