import json
import os
import sys
from textual.app import App, ComposeResult
from textual.widgets import Header, Footer, DataTable, Input
from textual.containers import Container
from textual.binding import Binding

class RegistryDashboard(App):
    """A futuristic TUI for inspecting the Jack Compiler Global Registry."""

    CSS = """
    Screen {
        layout: vertical;
        background: $surface;
    }
    
    /* TIGHT LAYOUT: Removed margins and fixed heights */
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
        color: auto; /* <--- FIXED: 'auto' ensures readability on accent color */
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
        # Using a simple label for the placeholder based on filename
        filename = os.path.basename(self.json_path) if self.json_path else "Registry"
        yield Container(
            Input(placeholder=f"Search {filename}...", id="search"),
            classes="search-container"
        )
        yield DataTable(zebra_stripes=True)
        yield Footer()

    def on_mount(self) -> None:
        self.title = "Global Registry"
        table = self.query_one(DataTable)
        table.cursor_type = "row"

        # Add styled columns
        table.add_columns("Class", "Method", "Type", "Return", "Parameters")

        self.load_data()

    def load_data(self):
        table = self.query_one(DataTable)
        table.clear()
        self.raw_data = []

        if not os.path.exists(self.json_path):
            self.notify(f"File not found: {self.json_path}", severity="error")
            return

        try:
            with open(self.json_path, 'r') as f:
                data = json.load(f)

            registry = data.get("registry", [])
            # Sort: OS classes first, then User classes
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

        except Exception as e:
            self.notify(f"Error parsing JSON: {e}", severity="error")

    def on_input_changed(self, event: Input.Changed) -> None:
        """Live search filter."""
        query = event.value.lower()
        table = self.query_one(DataTable)
        table.clear()

        for row in self.raw_data:
            if query in row[0].lower() or query in row[1].lower():
                table.add_row(*row)

    def action_refresh_data(self):
        self.load_data()

    def action_focus_search(self):
        self.query_one(Input).focus()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 global_registry_viz.py <path_to_json>")
        sys.exit(1)

    json_path = sys.argv[1]
    app = RegistryDashboard(json_path)
    app.run()