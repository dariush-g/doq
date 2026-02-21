import os
import sys
import tempfile
import msgpack
import pytest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src', 'indexer'))

from extractor import extract_text, walk_directory, classify_pdf
from serializer import tokenize, build_inverted_index, serialize_index, deserialize_index


# --- tokenize ---

def test_tokenize_basic():
    assert tokenize("Hello World") == ["hello", "world"]

def test_tokenize_numbers():
    assert tokenize("page 42") == ["page", "42"]

def test_tokenize_punctuation():
    assert tokenize("foo, bar. baz!") == ["foo", "bar", "baz"]

def test_tokenize_empty():
    assert tokenize("") == []

def test_tokenize_mixed_case():
    assert tokenize("PyThOn") == ["python"]


# --- classify_pdf ---

def test_classify_pdf_empty():
    assert classify_pdf([]) == "empty"

def test_classify_pdf_text():
    pages = [{"page": i, "text": "some content"} for i in range(10)]
    assert classify_pdf(pages) == "text"

def test_classify_pdf_scanned():
    pages = [{"page": i, "text": ""} for i in range(10)]
    assert classify_pdf(pages) == "scanned"

def test_classify_pdf_at_threshold():
    # 3 of 10 pages have text = 0.3 ratio → boundary, should be 'text' (> 0.3 is text)
    pages = [{"page": i, "text": "content" if i < 3 else ""} for i in range(10)]
    assert classify_pdf(pages) == "scanned"

def test_classify_pdf_above_threshold():
    pages = [{"page": i, "text": "content" if i < 4 else ""} for i in range(10)]
    assert classify_pdf(pages) == "text"


# --- extract_text ---

def test_extract_text_txt():
    with tempfile.NamedTemporaryFile(suffix=".txt", mode="w", delete=False) as f:
        f.write("hello world")
        path = f.name
    try:
        file = {"path": path, "name": os.path.basename(path), "extension": ".txt", "size": 11}
        result = extract_text(file)
        assert result["type"] == "text"
        assert len(result["pages"]) == 1
        assert result["pages"][0]["text"] == "hello world"
        assert result["pages"][0]["page"] == 1
    finally:
        os.unlink(path)

def test_extract_text_md():
    with tempfile.NamedTemporaryFile(suffix=".md", mode="w", delete=False) as f:
        f.write("# Title\ncontent")
        path = f.name
    try:
        file = {"path": path, "name": os.path.basename(path), "extension": ".md", "size": 15}
        result = extract_text(file)
        assert result["type"] == "text"
        assert result["pages"][0]["text"] == "# Title\ncontent"
    finally:
        os.unlink(path)

def test_extract_text_missing_file():
    file = {"path": "/nonexistent/file.txt", "name": "file.txt", "extension": ".txt", "size": 0}
    result = extract_text(file)
    assert result["type"] == "empty"
    assert result["pages"] == []

def test_extract_text_unsupported_extension():
    with tempfile.NamedTemporaryFile(suffix=".xyz", mode="w", delete=False) as f:
        f.write("data")
        path = f.name
    try:
        file = {"path": path, "name": os.path.basename(path), "extension": ".xyz", "size": 4}
        result = extract_text(file)
        assert result["type"] == "unsupported"
        assert result["pages"] == []
    finally:
        os.unlink(path)


# --- walk_directory ---

def test_walk_directory_finds_supported():
    with tempfile.TemporaryDirectory() as d:
        for name in ["a.txt", "b.md", "c.pdf", "d.docx", "e.py"]:
            open(os.path.join(d, name), "w").close()
        results = walk_directory(d)
        names = {r["name"] for r in results}
        assert "a.txt" in names
        assert "b.md" in names
        assert "c.pdf" in names
        assert "d.docx" not in names
        assert "e.py" not in names

def test_walk_directory_entry_format():
    with tempfile.TemporaryDirectory() as d:
        path = os.path.join(d, "doc.txt")
        open(path, "w").close()
        results = walk_directory(d)
        assert len(results) == 1
        r = results[0]
        assert r["extension"] == ".txt"
        assert r["name"] == "doc.txt"
        assert "path" in r
        assert "size" in r

def test_walk_directory_empty():
    with tempfile.TemporaryDirectory() as d:
        assert walk_directory(d) == []


# --- build_inverted_index ---

def make_doc(path, name, text):
    return {
        "path": path,
        "name": name,
        "extension": ".txt",
        "size": len(text),
        "type": "text",
        "pages": [{"page": 1, "text": text}]
    }

def test_build_inverted_index_basic():
    docs = [make_doc("/a.txt", "a.txt", "hello world")]
    inv, doc_freq, page_lengths, avg, total = build_inverted_index(docs)
    assert "hello" in inv
    assert "world" in inv
    assert doc_freq["hello"] == 1
    assert total == 1

def test_build_inverted_index_term_frequency():
    docs = [make_doc("/a.txt", "a.txt", "cat cat dog")]
    inv, doc_freq, page_lengths, avg, total = build_inverted_index(docs)
    cat_entry = inv["cat"][0]  # [doc_idx, page_idx, freq]
    assert cat_entry[2] == 2  # tf = 2

def test_build_inverted_index_multi_doc():
    docs = [
        make_doc("/a.txt", "a.txt", "shared term"),
        make_doc("/b.txt", "b.txt", "shared other"),
    ]
    inv, doc_freq, page_lengths, avg, total = build_inverted_index(docs)
    assert doc_freq["shared"] == 2
    assert doc_freq["term"] == 1
    assert total == 2

def test_build_inverted_index_avg_page_len():
    docs = [
        make_doc("/a.txt", "a.txt", "one two three"),   # 3 tokens
        make_doc("/b.txt", "b.txt", "one"),              # 1 token
    ]
    _, _, _, avg, _ = build_inverted_index(docs)
    assert avg == 2.0

def test_build_inverted_index_empty():
    _, _, _, avg, total = build_inverted_index([])
    assert total == 0
    assert avg == 1.0


# --- serialize / deserialize roundtrip ---

def test_serialize_deserialize_roundtrip():
    docs = [make_doc("/a.txt", "a.txt", "hello world")]
    with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
        path = f.name
    try:
        serialize_index(docs, path)
        loaded = deserialize_index(path)
        assert len(loaded) == 1
        assert loaded[0]["path"] == "/a.txt"
        assert loaded[0]["name"] == "a.txt"
        assert loaded[0]["type"] == "text"
        assert loaded[0]["pages"] == [{"page": 1, "text": "hello world"}]
    finally:
        os.unlink(path)

def test_serialize_drops_pages_for_non_text():
    doc = {
        "path": "/a.pdf", "name": "a.pdf", "extension": ".pdf",
        "size": 100, "type": "scanned",
        "pages": [{"page": 1, "text": ""}]
    }
    with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
        path = f.name
    try:
        serialize_index([doc], path)
        loaded = deserialize_index(path)
        assert loaded[0]["pages"] == []
    finally:
        os.unlink(path)
