# Usage: python minify_gzip.py <input.html|.js|.css> <output.gz>
import gzip, sys, os

ext = os.path.splitext(sys.argv[1])[1].lower()
with open(sys.argv[1], 'r', encoding='utf-8') as f:
    text = f.read()

if ext == '.html':
    try:
        import htmlmin
        text = htmlmin.minify(text, remove_comments=True, remove_empty_space=True)
    except ImportError:
        print("ERROR: htmlmin not installed. Run: pip install htmlmin")
        sys.exit(1)
elif ext == '.js':
    try:
        import jsmin
        text = jsmin.jsmin(text)
    except ImportError:
        print("ERROR: jsmin not installed. Run: pip install jsmin")
        sys.exit(1)
elif ext == '.css':
    try:
        import csscompressor
        text = csscompressor.compress(text)
    except ImportError:
        print("ERROR: csscompressor not installed. Run: pip install csscompressor")
        sys.exit(1)

compressed = gzip.compress(text.encode('utf-8'), 9)
with open(sys.argv[2], 'wb') as f:
    f.write(compressed)
