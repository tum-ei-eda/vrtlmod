CFG=doxyfile.gmake.in

all: pdf

pdf: tex
	cp latex/refman.pdf vrtlmod.pdf

tex: dox
	$(MAKE) -C latex

dox:
	doxygen $(CFG)
	
clean:
	rm -R -f html latex
