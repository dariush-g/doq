
from extractor import extract_text, walk_directory
from serializer import serialize_index, build_inverted_index, serialize_inverted_index, serialize_bm25_meta, deserialize_index
from multiprocessing import Pool
import sys
import os



if __name__ == '__main__':
    if len(sys.argv) == 3 and sys.argv[2] == "--file":
        path = sys.argv[1]

        existing = deserialize_index('index.bin')
        existing = [d for d in existing if d['path'] != path]

        if not os.path.exists(path):
            print(f"file removed: {os.path.basename(path)}")
        else:
            file = {
                'path': path,
                'name': os.path.basename(path),
                'extension': os.path.splitext(path)[1].lower(),
                'size': os.path.getsize(path)
            }
            f = extract_text(file)

            if f['type'] == 'scanned':
                print(f"skipping scanned: {f['name']}")
                sys.exit(0)

            existing.append(f)
        
        serialize_index(existing, 'index.bin')
        inv, doc_freq, page_lengths, avg_page_len, total_pages = build_inverted_index(existing)
        serialize_inverted_index(inv, "inv_index.bin")
        serialize_bm25_meta(doc_freq, page_lengths, avg_page_len, total_pages, "bm25_meta.bin")
    else:
        root = os.path.expanduser(sys.argv[1] if len(sys.argv) > 1 else "~")
        results = walk_directory(root)
        
        extracted = []
        with Pool() as pool:
            for f in pool.imap_unordered(extract_text, results):
                if f['type'] == 'scanned':
                    print(f"skipping scanned: {f['name']}")
                    continue
                print(f"{f['name']} [{f['type']}] - {len(f['pages'])} pages")
                extracted.append(f)
        
        serialize_index(extracted, 'index.bin')
        inv, doc_freq, page_lengths, avg_page_len, total_pages = build_inverted_index(extracted)
        serialize_inverted_index(inv, "inv_index.bin")
        serialize_bm25_meta(doc_freq, page_lengths, avg_page_len, total_pages, "bm25_meta.bin")