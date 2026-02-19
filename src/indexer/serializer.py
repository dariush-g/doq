import msgpack

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