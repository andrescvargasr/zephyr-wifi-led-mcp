#!/usr/bin/env python3
import os
import gzip

# Files to compress and embed
files = {
    "index.html": ("index_html_gz", "text/html"),
    "style.css": ("style_css_gz", "text/css"),
    "app.js": ("app_js_gz", "application/javascript")
}

output_header = "../../include/web_assets.h"

def generate_header():
    print(f"Generating {output_header}...")
    
    with open(output_header, "w") as f:
        f.write("/* Auto-generated header containing compressed web assets. */\n")
        f.write("#ifndef WEB_ASSETS_H\n")
        f.write("#define WEB_ASSETS_H\n\n")
        f.write("#include <zephyr/kernel.h>\n\n")
        
        for filename, (var_name, mime_type) in files.items():
            if not os.path.exists(filename):
                print(f"Error: {filename} not found!")
                continue
                
            # Read content
            with open(filename, "rb") as f_in:
                data = f_in.read()
                
            # Compress using gzip
            compressed_data = gzip.compress(data)
            length = len(compressed_data)
            
            print(f"  {filename}: {len(data)} bytes -> {length} bytes (gzipped)")
            
            # Format as C hex array
            hex_bytes = [f"0x{byte:02x}" for byte in compressed_data]
            lines = []
            for i in range(0, len(hex_bytes), 12):
                chunk = hex_bytes[i:i+12]
                lines.append("\t" + ", ".join(chunk))
            
            f.write(f"/* {filename} - Original: {len(data)} bytes, MIME: {mime_type} */\n")
            f.write(f"static const uint8_t {var_name}[] = {{\n" + ",\n".join(lines) + "\n};\n")
            f.write(f"static const size_t {var_name}_len = {length};\n\n")
            
        f.write("#endif /* WEB_ASSETS_H */\n")
        
    print("Done!")

if __name__ == "__main__":
    generate_header()
