
from extractor import extract_text, walk_directory
from serializer import serialize_index
from multiprocessing import Pool
import sys


if __name__ == '__main__':
    import sys
    results = walk_directory(sys.argv[1])
    
    extracted = []
    with Pool() as pool:
        for f in pool.imap_unordered(extract_text, results):
            print(f"{f['name']} [{f['type']}] - {len(f['pages'])} pages")
            extracted.append(f)
    
    serialize_index(extracted, 'index.bin')