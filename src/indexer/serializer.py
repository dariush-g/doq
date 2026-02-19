import msgpack
from collections import defaultdict


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

def tokenize(text: str) -> list[str]:
    import re
    return re.findall(r'[a-z0-9]+', text.lower())

def build_inverted_index(files: list[dict]) -> dict:
    inv = defaultdict(list)
    for di, doc in enumerate(files):
        for pi, page in enumerate(doc.get('pages', [])):
            tokens = tokenize(page['text'])
            tf = {}
            for t in tokens:
                tf[t] = tf.get(t, 0) + 1
            for term, freq in tf.items():
                inv[term].append([di, pi, freq])
    return dict(inv)

