import sys
import os
import xml.etree.ElementTree as ET
import json
import webview


# 1. PARSER
def parse_node(element):
    tag = element.tag
    text = element.text.strip() if element.text else ""
    label = tag
    if text: label += f": {text}"
    style_map = {
        "class": {"f": "#1f6feb", "s": "#388bfd"}, "subroutineDec": {"f": "#238636", "s": "#2ea043"},
        "doStatement": {"f": "#8957e5", "s": "#a371f7"}, "letStatement": {"f": "#8957e5", "s": "#a371f7"},
        "ifStatement": {"f": "#d29922", "s": "#e3b341"}, "whileStatement": {"f": "#d29922", "s": "#e3b341"},
        "returnStatement": {"f": "#da3633", "s": "#f85149"}, "identifier": {"f": "#30363d", "s": "#6e7681"},
        "symbol": {"f": "#30363d", "s": "#8b949e"}, "integerConstant": {"f": "#1f6feb", "s": "#58a6ff"},
        "stringConstant": {"f": "#1f6feb", "s": "#58a6ff"}, "keyword": {"f": "#da3633", "s": "#f85149"},
        "default": {"f": "#30363d", "s": "#6e7681"}
    }
    style = style_map.get(tag, style_map["default"])
    return { "name": label, "fill": style["f"], "stroke": style["s"], "children": [parse_node(child) for child in element] }


# 2. LOCAL ASSET LOADER
def get_d3_script():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    d3_path = os.path.join(script_dir, "d3.v7.min.js")
    if os.path.exists(d3_path):
        try:
            with open(d3_path, "r", encoding="utf-8") as f: return f"<script>\n{f.read()}\n</script>"
        except:
            pass
    return '<script src="https://d3js.org/d3.v7.min.js"></script>'


# 3. HTML GENERATOR
def get_html_content(files_payload, d3_script_tag):
    json_str = json.dumps(files_payload)
    return f"""
<!DOCTYPE html>
<html>
<head>
    {d3_script_tag}
    <style>
        body {{ margin: 0; background-color: #0d1117; overflow: hidden; font-family: sans-serif; height: 100vh; }}
        canvas {{ display: block; outline: none; }}
        
        #ui-layer {{ position: absolute; top: 20px; left: 20px; z-index: 100; }}
        
        .card {{
            background: rgba(22, 27, 34, 0.95);
            backdrop-filter: blur(10px);
            border: 1px solid #30363d;
            border-radius: 8px;
            padding: 16px;
            color: #c9d1d9;
            box-shadow: 0 8px 24px rgba(0,0,0,0.4);
            min-width: 250px;
        }}
        
        h1 {{ margin: 0 0 10px 0; font-size: 14px; color: #f0f6fc; font-weight: 600; }}
        
        /* DROPDOWN STYLE */
        select {{
            width: 100%;
            padding: 8px;
            background: #0d1117;
            border: 1px solid #30363d;
            border-radius: 6px;
            color: #c9d1d9;
            font-size: 13px;
            outline: none;
            margin-bottom: 8px;
        }}
        select:hover {{ border-color: #58a6ff; }}
        
        .info {{ font-size: 11px; color: #8b949e; margin-top: 8px; border-top: 1px solid #30363d; padding-top: 8px; }}
    </style>
</head>
<body>
    <div id="ui-layer">
        <div class="card">
            <h1>AST Visualizer</h1>
            <select id="file-select" onchange="loadFile(this.value)"></select>
            <div class="info">Left Click to Pan &bull; Scroll to Zoom</div>
        </div>
    </div>
    <canvas id="viz"></canvas>

    <script>
        const allFiles = {json_str};
        let currentTransform = d3.zoomIdentity;
        const canvas = document.querySelector("#viz");
        const ctx = canvas.getContext("2d", {{ alpha: false }});
        
        // --- Populate Dropdown ---
        const select = document.getElementById("file-select");
        allFiles.forEach((file, index) => {{
            const opt = document.createElement("option");
            opt.value = index;
            opt.textContent = file.filename;
            select.appendChild(opt);
        }});

        function loadFile(index) {{
            const root = d3.hierarchy(allFiles[index].tree);
            const treeLayout = d3.tree().nodeSize([40, 200]);
            treeLayout(root);
            window.currentRoot = root;
            
            // Reset zoom to nicely fit the new tree
            const initialY = window.innerHeight / 2;
            d3.select(canvas).call(zoom.transform, d3.zoomIdentity.translate(100, initialY).scale(0.8));
            
            requestAnimationFrame(draw);
        }}

        function draw() {{
            if (!window.currentRoot) return;
            const root = window.currentRoot;
            const width = canvas.width / window.devicePixelRatio;
            const height = canvas.height / window.devicePixelRatio;
            const k = currentTransform.k, tx = currentTransform.x, ty = currentTransform.y;

            ctx.save();
            ctx.scale(window.devicePixelRatio, window.devicePixelRatio);
            ctx.fillStyle = "#0d1117"; ctx.fillRect(0, 0, width, height);
            ctx.translate(tx, ty); ctx.scale(k, k);

            ctx.beginPath(); ctx.strokeStyle = "#30363d"; ctx.lineWidth = 2;
            const linkGen = d3.linkHorizontal().x(d => d.y).y(d => d.x).context(ctx);
            root.links().forEach(d => linkGen(d));
            ctx.stroke();

            ctx.font = "600 12px sans-serif"; ctx.textBaseline = "middle"; ctx.lineWidth = 1.5;
            root.descendants().forEach(d => {{
                if (d.x * k + ty < -50 || d.x * k + ty > height + 50) return;
                const w = Math.max(120, ctx.measureText(d.data.name).width + 30);
                const h = 26, x = d.y, y = d.x - 13;
                
                ctx.beginPath(); ctx.roundRect(x, y, w, h, 6); 
                ctx.fillStyle = d.data.fill; ctx.fill();
                ctx.strokeStyle = d.data.stroke; ctx.stroke();
                if (k > 0.4) {{ ctx.fillStyle = "#fff"; ctx.fillText(d.data.name, x + 10, d.x + 1); }}
            }});
            ctx.restore();
        }}

        const zoom = d3.zoom().scaleExtent([0.1, 4]).on("zoom", e => {{
            currentTransform = e.transform; requestAnimationFrame(draw);
        }});
        d3.select(canvas).call(zoom).on("dblclick.zoom", null);

        window.addEventListener("resize", () => {{
            const dpr = window.devicePixelRatio || 1;
            canvas.width = window.innerWidth * dpr; canvas.height = window.innerHeight * dpr;
            requestAnimationFrame(draw);
        }});
        window.dispatchEvent(new Event('resize'));

        if (allFiles.length > 0) loadFile(0);
    </script>
</body>
</html>
    """

if __name__ == "__main__":
    # Validation: Arguments
    if len(sys.argv) < 2:
        print("Error: No XML files provided.", file=sys.stderr)
        print("Usage: python jack_viz.py <file1.xml> <file2.xml> ...", file=sys.stderr)
        sys.exit(1)

    xml_files = sys.argv[1:]
    files_payload = []
    errors = []

    # Parse Files with Explicit Error Handling
    for xml_path in xml_files:
        if not os.path.exists(xml_path):
            print(f" Error: File not found: {xml_path}", file=sys.stderr)
            errors.append(xml_path)
            continue

        try:
            tree = ET.parse(xml_path)

            # Extract clean name: "Main_18293.xml" -> "Main.jack"
            raw_name = os.path.basename(xml_path)
            display_name = raw_name.split('_')[0] + ".jack" if "_" in raw_name else raw_name

            root = tree.getroot()
            if root is None:
                raise ValueError("Empty XML root")

            files_payload.append({
                "filename": display_name,
                "tree": parse_node(root)
            })

        except ET.ParseError as e:
            print(f" JSON Parse Error in {xml_path}: {e}", file=sys.stderr)
            errors.append(xml_path)
        except Exception as e:
            print(f" Unexpected Error processing {xml_path}: {e}", file=sys.stderr)
            errors.append(xml_path)

    # Critical Failure Check
    if not files_payload:
        print("\n Fatal: No valid Jack ASTs could be loaded.", file=sys.stderr)
        if errors:
            print(f"Failed files: {', '.join(errors)}", file=sys.stderr)
        sys.exit(1)

    # Launch GUI
    try:
        html_content = get_html_content(files_payload, get_d3_script())
        webview.create_window(
            title="Jack Visualizer",
            html=html_content,
            width=1280,
            height=800,
            background_color='#0d1117'
        )
        webview.start()
    except Exception as e:
        print(f" GUI Error: Could not start webview. {e}", file=sys.stderr)
        sys.exit(1)