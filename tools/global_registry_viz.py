import json
import os
import sys
from pathlib import Path
from textual.app import App, ComposeResult
from textual.containers import Container
from textual.widgets import Header, Footer, DataTable, Input, Static
from textual.binding import Binding

class RegistryViewer(App):
    """A Modern TUI for inspecting the Jack Compiler Global Registry."""

    CSS = """
    Screen {
        layout: vertical;
    }
    Input {
        dock: top;
        margin: 1 0;
        border: tall $accent;
    }
    DataTable {
        height: 1fr;
        border: solid $secondary;
    }
    .header-box {
        background: $accent;
        color: $text;
        content-align: center middle;
        text-style: bold;
    }
    """

    BINDINGS = [
        Binding("q", "quit", "Quit"),
        Binding("r", "refresh_table", "Reload File"),
    ]

    def __init__(self, json_path):
        super().__init__()
        self.json_path = json_path
        self.full_data = [] # Store raw data for filtering

    def compose(self) -> ComposeResult:
        yield Header(show_clock=True)
        yield Input(placeholder="Search by Class or Method name...", id="search_box")
        yield DataTable()
        yield Footer()

    def on_mount(self) -> None:
        table = self.query_one(DataTable)
        table.cursor_type = "row"
        table.zebra_stripes = True

        # Define Columns
        table.add_columns("Class", "Method", "Type", "Return", "Parameters")

        self.load_data()

    def load_data(self):
        """Loads JSON data and populates the table."""
        table = self.query_one(DataTable)
        table.clear()
        self.full_data = []

        if not os.path.exists(self.json_path):
            self.notify(f"Error: {self.json_path} not found!", severity="error")
            return

        try:
            with open(self.json_path, 'r') as f:
                content = json.load(f)

            registry = content.get("registry", [])

            # Sort by Class then Method
            registry.sort(key=lambda x: (x['class'], x['method']))

            for entry in registry:
                row = (
                    entry['class'],
                    entry['method'],
                    "𝑓 static" if entry['type'] == "function" else "ⓜ method",
                    entry['return'],
                    entry['params']
                )
                self.full_data.append(row)
                table.add_row(*row)

            self.sub_title = f"Loaded {len(registry)} entries from {self.json_path}"

        except Exception as e:
            self.notify(f"JSON Error: {e}", severity="error")

    def on_input_changed(self, message: Input.Changed) -> None:
        """Filter the table live as the user types."""
        query = message.value.lower()
        table = self.query_one(DataTable)
        table.clear()

        for row in self.full_data:
            # Row[0] is Class, Row[1] is Method
            if query in row[0].lower() or query in row[1].lower():
                table.add_row(*row)

    def action_refresh_table(self):
        self.load_data()
        self.notify("Registry reloaded!")

def find_json_file():
    """Smartly locates the registry_debug.json file."""
    # 1. Check command line args for a specific file
    if len(sys.argv) > 1 and sys.argv[1].endswith(".json"):
        return sys.argv[1]

    # 2. Check if we are in 'tools' and file is in 'tests/JackTestCode' (common dev pattern)
    candidates = [
        "registry_debug.json",                  # Current dir
        "build/registry_debug.json",            # Build dir
        "../tests/JackTestCode/registry_debug.json", # Test dir (relative to tools)
        "tests/JackTestCode/registry_debug.json",    # Test dir (relative to root)
    ]

    # Search recursively for the most recent registry file if specific ones aren't found
    for root, dirs, files in os.walk("."):
        if "registry_debug.json" in files:
            candidates.append(os.path.join(root, "registry_debug.json"))
            break

    for path in candidates:
        if os.path.exists(path):
            return path

    return "registry_debug.json" # Default fallback

if __name__ == "__main__":
    path = find_json_file()
    app = RegistryViewer(path)
    app.run()