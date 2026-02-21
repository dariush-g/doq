import msgpack
from collections import defaultdict

def deserialize_index(file: str) -> list[dict]:
    with open(file, 'rb') as f:
        return msgpack.unpack(f, raw=False)

def serialize_index(files: list[dict], output_path: str):
    index = []
    for f in files:
        entry = {
            'path': f['path'],
            'name': f['name'],
            'extension': f['extension'],
            'size': f['size'],
            'type': f['type'],
            'pages': f['pages'] if f['type'] == 'text' else []
        }
        index.append(entry)
    
    with open(output_path, 'wb') as out:
        msgpack.pack(index, out, use_bin_type=True)
    
    print(f"Indexed {len(index)} files -> {output_path}")
    
def serialize_inverted_index(inv_index: dict, output_path: str):
    # inv_index: term -> [(doc_idx, page_idx, tf), ...]
    with open(output_path, 'wb') as f:
        msgpack.pack(inv_index, f, use_bin_type=True)

def serialize_bm25_meta(doc_freq: dict, page_lengths: dict, avg_page_len: float, total_pages: int, output_path: str):
    meta = {
        'doc_freq': doc_freq,
        'page_lengths': page_lengths,
        'avg_page_len': avg_page_len,
        'total_pages': total_pages
    }
    with open(output_path, 'wb') as f:
        msgpack.pack(meta, f, use_bin_type=True)

def tokenize(text: str) -> list[str]:
    import re
    return re.findall(r'[a-z0-9]+', text.lower())

def build_inverted_index(files: list[dict]) -> tuple[dict, dict, dict, float, int]:
    inv = defaultdict(list)
    doc_freq = defaultdict(int)
    page_lengths = defaultdict(dict)
    total_pages = 0
    total_len = 0

    for di, doc in enumerate(files):
        for pi, page in enumerate(doc.get('pages', [])):
            tokens = tokenize(page['text'])
            page_len = len(tokens)
            page_lengths[di][pi] = page_len
            total_len += page_len
            total_pages += 1

            tf = {}
            for t in tokens:
                tf[t] = tf.get(t, 0) + 1
            for term, freq in tf.items():
                inv[term].append([di, pi, freq])
                doc_freq[term] += 1  # count once per page not per occurrence

    avg_page_len = total_len / total_pages if total_pages > 0 else 1.0
    return dict(inv), dict(doc_freq), dict(page_lengths), avg_page_len, total_pages