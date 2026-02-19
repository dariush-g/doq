
from extractor import extract_text, walk_directory
from serializer import serialize_index, build_inverted_index, serialize_inverted_index
from multiprocessing import Pool
import sys
import os



if __name__ == '__main__':
    root = os.path.expanduser(sys.argv[1] if len(sys.argv) > 1 else "~")
    results = walk_directory(root)
    
    extracted = []
    with Pool() as pool:
        for f in pool.imap_unordered(extract_text, results):
            print(f"{f['name']} [{f['type']}] - {len(f['pages'])} pages")
            extracted.append(f)
    
    serialize_index(extracted, 'index.bin')
    map_ = build_inverted_index(extracted)
    serialize_inverted_index(map_, "inv_index.bin")