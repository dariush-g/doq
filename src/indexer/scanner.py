import ollama
import base64
import fitz

def get_document_description(image_b64: str) -> str:
    response = ollama.chat(
        model='llava',
        messages=[{
            'role': 'user',
            'content': 'Describe what this document is about in 3-5 sentences for search indexing.',
            'images': [image_b64]
        }]
    )
    return response['message']['content']

def get_embedding(text: str) -> list[float]:
    response = ollama.embeddings(model='nomic-embed-text', prompt=text)
    return response['embedding']

def extract_first_significant_page_image(path: str) -> str | None:
    doc = fitz.open(path)
    for page in doc:
        pix = page.get_pixmap()
        w, h = pix.width, pix.height
        
        # crop to center 60% to avoid edge shadows
        x0 = int(w * 0.2)
        y0 = int(h * 0.2)
        x1 = int(w * 0.8)
        y1 = int(h * 0.8)
        
        # sample center pixels only
        center_samples = []
        for y in range(y0, y1, 10):  # step by 10 for speed
            for x in range(x0, x1, 10):
                idx = (y * w + x) * pix.n  # pix.n = bytes per pixel
                center_samples.append(pix.samples[idx])
        
        if max(center_samples) - min(center_samples) > 30:
            img_bytes = pix.tobytes("png")
            return base64.b64encode(img_bytes).decode('utf-8')
    
    return None

def parse_scanned_document(path: str) -> list[float] | None:
    first_page_b64 = extract_first_significant_page_image(path)
    if first_page_b64 is None:
        return None
    description = get_document_description(first_page_b64)
    embedding = get_embedding(description)
    return embedding