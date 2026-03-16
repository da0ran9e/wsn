# ============================================================
# Makefile — IEEE Conference Paper (version1)
# Usage:
#   make          — compile PDF (runs pdflatex twice)
#   make bib      — compile with BibTeX (if switching to .bib)
#   make clean    — remove generated files
#   make cleanall — also remove the PDF
# ============================================================

TEMPLATE_DIR := ../template/IEEE_Conference_Template
MAIN         := paper
CLS          := IEEEtran.cls

.PHONY: all bib clean cleanall

all: $(CLS) $(MAIN).pdf

# Copy IEEEtran.cls from the template directory if not present
$(CLS):
	@echo "[INFO] Copying $(CLS) from template..."
	cp $(TEMPLATE_DIR)/$(CLS) .

$(MAIN).pdf: $(MAIN).tex $(CLS)
	@echo "[INFO] First pass — pdflatex..."
	pdflatex -halt-on-error $(MAIN).tex
	@echo "[INFO] Second pass — pdflatex (resolve cross-references)..."
	pdflatex -halt-on-error $(MAIN).tex
	@echo "[OK]   $(MAIN).pdf generated."

# Target for BibTeX workflow (future use)
bib: $(CLS) $(MAIN).tex
	pdflatex -halt-on-error $(MAIN).tex
	bibtex $(MAIN)
	pdflatex -halt-on-error $(MAIN).tex
	pdflatex -halt-on-error $(MAIN).tex

clean:
	rm -f *.aux *.log *.bbl *.blg *.out *.toc *.lof *.lot *.fls *.fdb_latexmk

cleanall: clean
	rm -f $(MAIN).pdf $(CLS)
