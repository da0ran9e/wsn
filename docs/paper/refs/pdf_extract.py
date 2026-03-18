#!/usr/bin/env python3
"""
Simple PDF-to-text extractor for a folder.
Usage:
  python pdf_extract.py /path/to/folder [--recursive] [--overwrite]

Finds all .pdf files (optionally recursively) and writes a .txt file
with extracted text next to each PDF (same folder). Uses PyPDF2.
"""
import argparse
from pathlib import Path
from PyPDF2 import PdfReader
import sys


def extract_pdf_to_text(pdf_path: Path, out_path: Path, overwrite: bool = False) -> bool:
    if out_path.exists() and not overwrite:
        print(f"Skipping existing: {out_path}")
        return False
    try:
        reader = PdfReader(str(pdf_path))
    except Exception as e:
        print(f"ERROR: cannot open PDF {pdf_path}: {e}")
        return False
    parts = []
    for i, page in enumerate(reader.pages):
        try:
            txt = page.extract_text()
        except Exception as e:
            txt = None
        parts.append(f"--- PAGE {i+1} ---\n")
        parts.append(txt or "")
    out_text = "\n".join(parts)
    try:
        out_path.write_text(out_text, encoding="utf-8")
        print(f"Wrote: {out_path}")
        return True
    except Exception as e:
        print(f"ERROR: cannot write {out_path}: {e}")
        return False


def find_pdfs(folder: Path, recursive: bool):
    if recursive:
        yield from folder.rglob('*.pdf')
    else:
        yield from folder.glob('*.pdf')


def main():
    parser = argparse.ArgumentParser(description='Extract text from PDFs in a folder (writes .txt next to each PDF)')
    parser.add_argument('folder', type=str, help='Folder containing PDFs')
    parser.add_argument('--recursive', action='store_true', help='Search folders recursively')
    parser.add_argument('--overwrite', action='store_true', help='Overwrite existing .txt files')
    args = parser.parse_args()

    folder = Path(args.folder)
    if not folder.exists() or not folder.is_dir():
        print(f"ERROR: folder does not exist or is not a directory: {folder}")
        sys.exit(2)

    pdfs = list(find_pdfs(folder, args.recursive))
    if not pdfs:
        print("No PDF files found.")
        return

    for pdf in pdfs:
        out = pdf.with_suffix('.txt')
        extract_pdf_to_text(pdf, out, overwrite=args.overwrite)

if __name__ == '__main__':
    main()
