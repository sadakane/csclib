BUILDDIR=build

SRCDIR = csclib/
SOURCES = ${wildcard ${SRCDIR}*.h}

OPTIONS = -g -O0 -I $(SRCDIR)

all: $(BUILDDIR)/share.out $(BUILDDIR)/precompute.out $(BUILDDIR)/LOUDS_test.out $(BUILDDIR)/BP_test.out $(BUILDDIR)/aes.out $(BUILDDIR)/suffixarray.out
	
$(BUILDDIR)/share.out: share.c $(SOURCES)
	gcc -o $(BUILDDIR)/share.out share.c $(OPTIONS)

$(BUILDDIR)/precompute.out: $(SRCDIR)precompute.c config.txt $(SOURCES)
	gcc -o $(BUILDDIR)/precompute.out $(SRCDIR)precompute.c $(OPTIONS)
	#$(BUILDDIR)/precompute.out

$(BUILDDIR)/aes.out: aes/aes2.c $(SOURCES)
	gcc -o $(BUILDDIR)/aes.out aes/aes2.c $(OPTIONS)

$(BUILDDIR)/suffixarray.out: suffixarray/suffixarray.c suffixarray/suffixarray.h $(SOURCES)
	gcc -o $(BUILDDIR)/suffixarray.out suffixarray/suffixarray.c $(OPTIONS)

$(BUILDDIR)/LOUDS_test.out: trees/LOUDS_test.c $(SOURCES)
	gcc -o $(BUILDDIR)/LOUDS_test.out trees/LOUDS_test.c $(OPTIONS)

$(BUILDDIR)/BP_test.out: trees/BP_test.c $(SOURCES)
	gcc -o $(BUILDDIR)/BP_test.out trees/BP_test.c $(OPTIONS)

clean:
	rm -f $(BUILDDIR)/*.out
