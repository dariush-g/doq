from extractor import extract_text, walk_directory
from serializer import serialize_index, build_inverted_index, serialize_inverted_index, serialize_bm25_meta, deserialize_index
from scanner import parse_scanned_document
from multiprocessing import Pool
import struct
import sys
import os


def serialize_embeddings(embeddings: dict[str, list[float]], path: str):
    with open(path, 'wb') as f:
        f.write(struct.pack('I', len(embeddings)))
        for filepath, vec in embeddings.items():
            encoded = filepath.encode('utf-8')
            f.write(struct.pack('I', len(encoded)))
            f.write(encoded)
            f.write(struct.pack('I', len(vec)))
            f.write(struct.pack(f'{len(vec)}f', *vec))


def deserialize_embeddings(path: str) -> dict[str, list[float]]:
    if not os.path.exists(path):
        return {}
    with open(path, 'rb') as f:
        count = struct.unpack('I', f.read(4))[0]
        result = {}
        for _ in range(count):
            path_len = struct.unpack('I', f.read(4))[0]
            filepath = f.read(path_len).decode('utf-8')
            vec_len = struct.unpack('I', f.read(4))[0]
            vec = list(struct.unpack(f'{vec_len}f', f.read(vec_len * 4)))
            result[filepath] = vec
        return result


if __name__ == '__main__':
    if len(sys.argv) == 3 and sys.argv[2] == "--file":
        path = sys.argv[1]

        existing = deserialize_index('index.bin')
        existing = [d for d in existing if d['path'] != path]

        embeddings = deserialize_embeddings('embeddings.bin')
        embeddings.pop(path, None)

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
                try:
                    embedding = parse_scanned_document(path)
                    if embedding:
                        embeddings[path] = embedding
                        print(f"embedded scanned: {f['name']}")
                except ConnectionError:
                    print("ollama not running - skipping scanned document embedding")
            else:
                existing.append(f)

        serialize_index(existing, 'index.bin')
        serialize_embeddings(embeddings, 'embeddings.bin')
        inv, doc_freq, page_lengths, avg_page_len, total_pages = build_inverted_index(existing)
        serialize_inverted_index(inv, "inv_index.bin")
        serialize_bm25_meta(doc_freq, page_lengths, avg_page_len, total_pages, "bm25_meta.bin")
    else:
        root = os.path.expanduser(sys.argv[1] if len(sys.argv) > 1 else "~")
        results = walk_directory(root)

        extracted = []
        scanned_paths = []

        with Pool() as pool:
            for f in pool.imap_unordered(extract_text, results):
                if f['type'] == 'scanned':
                    scanned_paths.append(f['path'])
                    print(f"{f['name']} [scanned] - queued for embedding")
                    continue
                print(f"{f['name']} [{f['type']}] - {len(f['pages'])} pages")
                extracted.append(f)

        embeddings = {}
        for p in scanned_paths:
            try:
                embedding = parse_scanned_document(p)
                if embedding:
                    embeddings[p] = embedding
                    print(f"embedded: {os.path.basename(p)}")
            except ConnectionError:
                print("ollama not running - skipping scanned document embeddings")
                break

        serialize_index(extracted, 'index.bin')
        serialize_embeddings(embeddings, 'embeddings.bin')
        inv, doc_freq, page_lengths, avg_page_len, total_pages = build_inverted_index(extracted)
        serialize_inverted_index(inv, "inv_index.bin")
        serialize_bm25_meta(doc_freq, page_lengths, avg_page_len, total_pages, "bm25_meta.bin")