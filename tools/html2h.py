import os

def html_to_header(input_path, output_path, var_name):
    with open(input_path, "r", encoding="utf-8") as f:
        html = f.read()

    byte_array = ', '.join(f'0x{ord(c):02x}' for c in html)
    length = len(html)

    header_content = f"""
#ifndef {var_name.upper()}_H
#define {var_name.upper()}_H

const unsigned char {var_name}[] = {{
    {byte_array}
}};
const unsigned int {var_name}_len = {length};

#endif
"""

    with open(output_path, "w", encoding="utf-8") as f:
        f.write(header_content)

def before_build(source, target, env):
    lib_dir = os.path.join(env["PROJECT_DIR"], "lib", "ESPWifiAssist")
    input_html = os.path.join(lib_dir, "index.html")
    output_header = os.path.join(lib_dir, "index_html.h")
    html_to_header(input_html, output_header, "index_html")

if __name__ == "__main__":
    # For manual testing only
    input_html = os.path.join("../", "assets", "index.html")
    output_header = os.path.join("../", "src", "index_html.h")
    html_to_header(input_html, output_header, "index_html")