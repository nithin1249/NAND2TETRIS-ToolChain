import json
import os
import sys
import atexit
import traceback
from textual.app import App, ComposeResult
from textual.widgets import Header, Footer, DataTable, Input
from textual.containers import Container
from textual.binding import Binding

class RegistryDashboard(App):
    CSS = """
    Screen {
        layout: vertical;
        background: $surface;
    }
    .search-container {
        dock: top;
        height: auto;
        margin: 0;
        padding: 0 1;
        background: $surface;
    }
    Input {
        width: 100%;
        height: auto;
        margin: 0;
        border: tall $accent;
        background: $panel;
        color: $text;
    }
    DataTable {
        height: 1fr;
        border: none;
        margin: 0;
        background: $surface;
    }
    DataTable > .datatable--header {
        text-style: bold;
        background: $accent;
        color: auto;
    }
    """

    BINDINGS = [
        Binding("q", "quit", "Quit"),
        Binding("r", "refresh_data", "Refresh"),
        Binding("/", "focus_search", "Search"),
    ]

    def __init__(self, json_path):
        super().__init__()
        self.json_path = json_path
        self.raw_data = []

    def compose(self) -> ComposeResult:
        yield Container(
            Input(placeholder=f"Search Registry", id="search"),
            classes="search-container"
        )
        yield DataTable(zebra_stripes=True)
        yield Footer()

    def on_mount(self) -> None:
        try:
            self.title = "Global Registry"
            table = self.query_one(DataTable)
            table.cursor_type = "row"
            table.add_columns("Class", "Method", "Type", "Return", "Parameters")
            self.load_data()

            table.focus()

        except Exception as e:
            self.notify(f"Critical Error on Mount: {e}", severity="error", timeout=10)

    def load_data(self):
        try:
            table = self.query_one(DataTable)
            table.clear()
            self.raw_data = []

            if not os.path.exists(self.json_path):
                self.notify(f"File not found: {self.json_path}", severity="error")
                return

            with open(self.json_path, 'r') as f:
                data = json.load(f)

            registry = data.get("registry", [])
            registry.sort(key=lambda x: (x['class'] not in ['Math','Array','Output','Screen','Keyboard','Memory','Sys','String'], x['class'], x['method']))

            for item in registry:
                kind = "ƒ static" if item['type'] == 'function' else "ⓜ method"
                row = (
                    item['class'],
                    item['method'],
                    kind,
                    item['return'],
                    item['params']
                )
                self.raw_data.append(row)
                table.add_row(*row)

            self.notify(f"Loaded {len(registry)} signatures.")

        except json.JSONDecodeError:
            self.notify("Error: JSON file is corrupted or empty.", severity="error")
        except KeyError as e:
            self.notify(f"Error: Missing key in JSON data: {e}", severity="error")
        except Exception as e:
            self.notify(f"Unexpected Error: {e}", severity="error")

    def on_input_changed(self, event: Input.Changed) -> None:
        try:
            query = event.value.lower()
            table = self.query_one(DataTable)
            table.clear()
            for row in self.raw_data:
                if query in row[0].lower() or query in row[1].lower():
                    table.add_row(*row)
        except Exception:
            pass # Silent fail for UI flicker

    def action_refresh_data(self):
        self.load_data()

    def action_focus_search(self):
        self.query_one(Input).focus()

def cleanup_temp_file(path):
    if os.path.exists(path):
        try:
            os.remove(path)
        except OSError as e:
            # Print to stderr so it doesn't mess up TUI output
            print(f"Warning: Could not delete temp file: {e}", file=sys.stderr)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 global_registry_viz.py <path_to_json>")
        sys.exit(1)

    json_path = sys.argv[1]

    # Register Cleanup
    atexit.register(cleanup_temp_file, json_path)

    try:
        # Run App Protected
        app = RegistryDashboard(json_path)
        app.run()

    except Exception as e:
        # Catch ALL Crashes (Textual errors, Python errors, etc.)
        # Clear screen so user sees the error
        os.system('cls' if os.name == 'nt' else 'clear')
        print("VISUALIZER CRASH", file=sys.stderr)
        print("--------------------------------------------------", file=sys.stderr)
        traceback.print_exc()  # Prints the full stack trace
        print("--------------------------------------------------", file=sys.stderr)
        sys.exit(1)