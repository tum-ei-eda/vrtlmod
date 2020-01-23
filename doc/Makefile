CFG=doxyfile

all: pdf

pdf: tex
	cp latex/refman.pdf vrtlmod.pdf

tex: dox
	$(MAKE) -C latex

dox:
	doxygen $(doxyfile)
	
clean:
	rm -R -f html latex