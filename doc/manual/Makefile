all:		wlziipsrv.pdf

clean:
		rm -f *.aux *.log *.out *.toc

clobber:	clean
		rm -f wlziipsrv.pdf

wlziipsrv.pdf:	architecture.pdf biblio.bib coordinates.pdf proxyarchit.pdf \
		wlziipsrv.tex

%.pdf:                  %.tex
			( \
			  pdflatex $<; \
			  while \grep -q \
				      "Rerun to get cross-references right." \
				      $(<:.tex=.log); \
			  do \
			   pdflatex $<; \
			  done \
			)
