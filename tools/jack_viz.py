import webview
import xml.etree.ElementTree as ET
import json
import os
import sys

def xml_to_dict(el):
    """Converts Jack XML to JSON structure for d3.v7.min.js."""
    tag_class = el.tag.lower()
    name = el.tag
    content = el.text.strip() if el.text and el.text.strip() else ""
    node = {"name": name, "content": content, "class": tag_class, "children": []}
    for child in el:
        node["children"].append(xml_to_dict(child))
    return node

def generate_html(data):
    # 1. Get the absolute path to the folder where THIS script lives
    script_dir = os.path.dirname(os.path.abspath(__file__))

    # 2. Build the full path to the JS file (e.g., /.../tools/d3.v7.min.js)
    d3_path = os.path.join(script_dir, "d3.v7.min.js")

    d3_script = ""
    if os.path.exists(d3_path):
        print(f"[+] Found local D3.js at: {d3_path}")
        with open(d3_path, "r", encoding="utf-8") as f:
            d3_script = f"<script>\n{f.read()}\n</script>"
    else:
        print(f"[!] Warning: d3.v7.min.js not found at {d3_path}")
        print("    -> Falling back to online CDN.")
        d3_script = '<script src="https://d3js.org/d3.v7.min.js"></script>'

    json_str = json.dumps(data)
    return f"""
    <!DOCTYPE html>
    <html>
    <head>
        <style>
            html, body {{ 
                background-color: #0d1117; 
                margin: 0; padding: 0;
                width: 100%; height: 100%;
                overflow: hidden;
                font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
            }}
            #canvas {{ width: 100vw; height: 100vh; display: block; cursor: grab; }}
            #canvas:active {{ cursor: grabbing; }}
            
            /* High-Performance SVG Styles */
            .link {{ fill: none; stroke: #30363d; stroke-width: 2px; transition: stroke 0.2s; }}
            .node rect {{ stroke-width: 2px; rx: 6; ry: 6; transition: filter 0.2s; }}
            .node:hover rect {{ filter: brightness(1.2); }}
            .node text {{ font-size: 11px; font-weight: bold; fill: #ffffff; pointer-events: none; }}
            .node .content-text {{ fill: #8b949e; font-family: "SFMono-Regular", Consolas, monospace; font-weight: normal; }}
            
            /* GitHub-Inspired Syntax Theme */
            .class rect {{ fill: #1f6feb; stroke: #388bfd; }}
            .subroutinedec rect {{ fill: #238636; stroke: #2ea043; }}
            .letstatement rect, .ifstatement rect, .whilestatement rect, .returnstatement rect {{ fill: #8957e5; stroke: #a371f7; }}
            .expression rect {{ fill: #d29922; stroke: #e3b341; }}
            .term rect {{ fill: #30363d; stroke: #8b949e; }}
        </style>
        {d3_script}
    </head>
    <body>
        <div id="canvas"></div>
        <script>
            const data = {json_str};
            
            const svg = d3.select("#canvas").append("svg")
                .attr("width", "100%")
                .attr("height", "100%")
                .call(d3.zoom().scaleExtent([0.05, 10]).on("zoom", (e) => g.attr("transform", e.transform)));

            const g = svg.append("g");
            
            // Layout Settings for Sideways
            const treeLayout = d3.tree().nodeSize([50, 240]); 
            const root = d3.hierarchy(data);
            treeLayout(root);

            // Draw Links - Horizontal Curves
            g.selectAll(".link")
                .data(root.links())
                .enter().append("path")
                .attr("class", "link")
                .attr("d", d3.linkHorizontal().x(d => d.y).y(d => d.x));

            // Draw Nodes
            const node = g.selectAll(".node")
                .data(root.descendants())
                .enter().append("g")
                .attr("class", d => "node " + d.data.class)
                .attr("transform", d => `translate(${{d.y}},${{d.x}})`);

            // Node Cards
            node.append("rect")
                .attr("y", -20)
                .attr("x", 0)
                .attr("width", d => Math.max(140, (d.data.content.length * 8) + 40))
                .attr("height", 40);

            node.append("text")
                .attr("x", 10)
                .attr("y", -5)
                .text(d => d.data.name.toUpperCase());

            node.append("text")
                .attr("class", "content-text")
                .attr("x", 10)
                .attr("y", 12)
                .text(d => d.data.content);

            // Initial Positioning: Center left
            const initialScale = 0.8;
            svg.call(d3.zoom().transform, d3.zoomIdentity.translate(100, window.innerHeight / 2).scale(initialScale));

            // Fullscreen Resize Handler
            window.addEventListener('resize', () => {{
                svg.attr("width", window.innerWidth).attr("height", window.innerHeight);
            }});
        </script>
    </body>
    </html>
    """
if __name__ == "__main__":

    # 1. Check if an argument was actually passed
    if len(sys.argv) < 2:
        print("\n[!] ERROR: No XML file provided.")
        print("Usage: python3 jack_viz_web.py <path_to_xml>\n")
        sys.exit(1)

    xml_path = sys.argv[1]

    # 2. Check if the file physically exists on the disk
    if not os.path.exists(xml_path):
        print(f"\n[!] ERROR: File not found at: {xml_path}")
        print("Ensure your C++ program is generating the XML correctly.\n")
        sys.exit(1)

    try:
        # 3. Proceed with parsing and launching
        tree_xml = ET.parse(xml_path)
        data_json = xml_to_dict(tree_xml.getroot())
        html_content = generate_html(data_json)

        window = webview.create_window('Jack AST Explorer (Horizontal)', html=html_content, background_color='#0d1117')
        webview.start()
    except ET.ParseError as e:
        print(f"\n[!] ERROR: Invalid XML format in {xml_path}")
        print(f"Details: {e}\n")
        sys.exit(1)