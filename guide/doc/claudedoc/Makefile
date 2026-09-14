# Build the course PDF.
#
#   make          -> course.pdf (two passes, for the table of contents)
#   make clean    -> remove LaTeX auxiliary files
#   make distclean-> also remove the PDF

LATEX    = pdflatex
LFLAGS   = -interaction=nonstopmode -halt-on-error
MAIN     = course
SOURCES  = $(MAIN).tex style.tex $(wildcard 0*.tex) $(wildcard 1*.tex) $(wildcard A-*.tex)

.PHONY: all clean distclean

all: $(MAIN).pdf

# Two passes: the first writes the .toc, the second typesets it.
$(MAIN).pdf: $(SOURCES)
	$(LATEX) $(LFLAGS) $(MAIN).tex
	$(LATEX) $(LFLAGS) $(MAIN).tex
	@echo
	@echo "built $(MAIN).pdf -- $$(pdfinfo $(MAIN).pdf 2>/dev/null | awk '/Pages/{print $$2}') pages"

clean:
	rm -f *.aux *.log *.toc *.out *.lof *.lot

distclean: clean
	rm -f $(MAIN).pdf
