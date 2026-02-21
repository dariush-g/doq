# parse all *.txt, *.md, *.pdf, *.docx
import sys
import os
from pathlib import Path
import pdfplumber
import warnings


SUPPORTED_EXTENSIONS = {'.pdf', '.txt', '.md'}

def walk_directory(root: str) -> list[dict]:
    files = []
    for path in Path(root).rglob('*'):
        if path.suffix.lower() in SUPPORTED_EXTENSIONS:
            files.append({
                'path': str(path.resolve()),
                'name': path.name,
                'extension': path.suffix.lower(),
                'size': path.stat().st_size,
            })
    return files


def extract_text(file: dict) -> dict:
    ext = file['extension']
    
    try:
        if ext in ('.txt', '.md'):
            with open(file['path'], 'r', errors='ignore') as f:
                text = f.read()
            file['pages'] = [{'page': 1, 'text': text}]
            file['type'] = 'text'
        
        elif ext == '.pdf':
            file['pages'] = extract_pdf(file['path'])
            file['type'] = classify_pdf(file['pages'])

        else:
            file['pages'] = []
            file['type'] = 'unsupported'

    except Exception as e:
        print(f"Warning: could not extract {file['path']}: {e}")
        file['pages'] = []
        file['type'] = 'empty'
    
    return file


def extract_pdf(path: str) -> list[dict]:
    pages = []
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")

        devnull = open(os.devnull, 'w')
        old_stderr = sys.stderr
        sys.stderr = devnull
        try:
            with pdfplumber.open(path) as pdf:
                for i, page in enumerate(pdf.pages):
                    text = page.extract_text()
                    pages.append({
                        'page': i + 1,
                        'text': text if text else ''
                    })
        finally:
            sys.stderr = old_stderr
            devnull.close()
    return pages

def classify_pdf(pages: list[dict]) -> str:
    total = len(pages)
    if total == 0:
        return 'empty'
    non_empty = sum(1 for p in pages if p['text'].strip())
    ratio = non_empty / total
    if ratio > 0.3:
        return 'text'
    else:
        return 'scanned'

# 