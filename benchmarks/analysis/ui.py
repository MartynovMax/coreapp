"""
ui.py — Tkinter-based desktop UI for the offline benchmark analysis tool.

Launch:
    python analysis/ui.py
    # or from anywhere:
    python -m analysis.ui

    # or double-click run_ui.bat (Windows) / run_ui.sh (Linux/macOS)

    # or double-click run_ui.bat (Windows) / run_ui.sh (Linux/macOS)

Pages (tab navigation):
  1. Single Run   — load a run, apply filters, view report + latency chart
  2. Compare Runs — load two runs, apply filters, view diff + throughput chart
  3. Plots        — load run(s), choose plot kind, view embedded chart
  4. Help         — usage reference
"""

from __future__ import annotations

import sys
import os
import traceback
import warnings
from pathlib import Path
from typing import Optional

_THIS_DIR = Path(__file__).resolve().parent
_BENCHMARKS_DIR = _THIS_DIR.parent
if str(_BENCHMARKS_DIR) not in sys.path:
    sys.path.insert(0, str(_BENCHMARKS_DIR))

import tkinter as tk
from tkinter import ttk, filedialog, messagebox, scrolledtext

import matplotlib
matplotlib.use("TkAgg")
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
from matplotlib.figure import Figure

from analysis.assemble import assemble_run_from_files
from analysis.filters import RunFilter, apply_filter
from analysis.report import report_run
from analysis.compare import compare_runs, format_comparison
from analysis.plots import (
    plot_latency_summary,
    plot_throughput_comparison,
    plot_timeseries_trend,
)
from analysis.models import RunModel


# ---------------------------------------------------------------------------
# Constants / theme
# ---------------------------------------------------------------------------

_BG      = "#1e1e2e"
_BG2     = "#181825"
_FG      = "#cdd6f4"
_ACCENT  = "#89b4fa"
_GREEN   = "#a6e3a1"
_RED     = "#f38ba8"
_YELLOW  = "#f9e2af"
_BORDER  = "#313244"
_FONT    = ("Segoe UI", 10)
_FONT_B  = ("Segoe UI", 10, "bold")
_FONT_SM = ("Segoe UI", 9)
_MONO    = ("Consolas", 9)


# ---------------------------------------------------------------------------
# Shared helpers
# ---------------------------------------------------------------------------

def _parse_filter_field(raw: str) -> Optional[list[str]]:
    tokens = [t.strip() for t in raw.split(",") if t.strip()]
    return tokens if tokens else None


def _build_filter(alloc: str, wl: str, prof: str, exp: str) -> Optional[RunFilter]:
    flt = RunFilter(
        allocators=_parse_filter_field(alloc),
        workloads=_parse_filter_field(wl),
        profiles=_parse_filter_field(prof),
        experiments=_parse_filter_field(exp),
    )
    return None if flt.is_empty() else flt


def _load_run(base_path: str):
    """Returns (RunModel, None) or (None, error_str)."""
    if not base_path.strip():
        return None, "Path is empty."
    try:
        with warnings.catch_warnings(record=True):
            warnings.simplefilter("always")
            run = assemble_run_from_files(base_path.strip(), validate=True)
        return run, None
    except FileNotFoundError as exc:
        return None, f"File not found: {exc}"
    except ValueError as exc:
        return None, f"Validation / assembly error:\n{exc}"
    except Exception:
        return None, f"Unexpected error:\n{traceback.format_exc()}"


def _ns_fmt(v) -> str:
    if v is None:
        return "NA"
    v = float(v)
    if v >= 1e9:  return f"{v/1e9:.3f} s"
    if v >= 1e6:  return f"{v/1e6:.3f} ms"
    if v >= 1e3:  return f"{v/1e3:.3f} µs"
    return f"{v:.0f} ns"


def _bytes_fmt(v) -> str:
    if v is None:
        return "NA"
    v = int(v)
    if v >= 1_073_741_824: return f"{v/1_073_741_824:.2f} GiB"
    if v >= 1_048_576:     return f"{v/1_048_576:.2f} MiB"
    if v >= 1_024:         return f"{v/1_024:.2f} KiB"
    return f"{v} B"


def _strip_ext(path: str) -> str:
    for ext in (".csv", ".jsonl"):
        if path.endswith(ext):
            return path[:-len(ext)]
    return path


# ---------------------------------------------------------------------------
# Reusable widgets
# ---------------------------------------------------------------------------

class _Section(ttk.LabelFrame):
    def __init__(self, parent, title: str, **kw):
        super().__init__(parent, text=title, padding=8, **kw)


class _FilterBar(ttk.Frame):
    """Four filter entry fields in one row."""

    def __init__(self, parent, **kw):
        super().__init__(parent, **kw)
        labels = ["Allocators", "Experiments", "Workloads", "Profiles"]
        self._vars: dict[str, tk.StringVar] = {}
        for col, lbl in enumerate(labels):
            key = lbl.lower()
            self._vars[key] = tk.StringVar()
            ttk.Label(self, text=lbl + ":").grid(
                row=0, column=col * 2, sticky="w", padx=(8 if col else 0, 2)
            )
            ttk.Entry(self, textvariable=self._vars[key], width=22).grid(
                row=0, column=col * 2 + 1, sticky="ew", padx=(0, 12)
            )
            self.columnconfigure(col * 2 + 1, weight=1)

    def build_filter(self) -> Optional[RunFilter]:
        return _build_filter(
            self._vars["allocators"].get(),
            self._vars["workloads"].get(),
            self._vars["profiles"].get(),
            self._vars["experiments"].get(),
        )


class _StatusBar(ttk.Label):
    def ok(self, msg: str):   self.config(text="✔  " + msg, foreground=_GREEN)
    def err(self, msg: str):  self.config(text="✘  " + msg, foreground=_RED)
    def info(self, msg: str): self.config(text="ℹ  " + msg, foreground=_YELLOW)
    def clear(self):          self.config(text="")


class _MetricCards(ttk.Frame):
    def __init__(self, parent, metrics: list[tuple[str, str]], **kw):
        super().__init__(parent, **kw)
        for i, (label, value) in enumerate(metrics):
            card = ttk.Frame(self, relief="groove", padding=8)
            card.grid(row=0, column=i, padx=6, pady=4, sticky="ew")
            ttk.Label(card, text=label, font=_FONT_SM).pack(anchor="w")
            ttk.Label(card, text=str(value), font=_FONT_B,
                      foreground=_ACCENT).pack(anchor="w")
            self.columnconfigure(i, weight=1)


class _SummaryTable(ttk.Frame):
    COLUMNS = (
        "#", "Experiment", "Allocator", "Status",
        "Min", "Median", "p95", "Max", "ops/sec", "Alloc bytes",
    )

    def __init__(self, parent, **kw):
        super().__init__(parent, **kw)
        vsb = ttk.Scrollbar(self, orient="vertical")
        hsb = ttk.Scrollbar(self, orient="horizontal")
        self._tree = ttk.Treeview(
            self, columns=self.COLUMNS, show="headings",
            yscrollcommand=vsb.set, xscrollcommand=hsb.set, selectmode="browse",
        )
        vsb.config(command=self._tree.yview)
        hsb.config(command=self._tree.xview)
        widths = [30, 200, 120, 70, 90, 90, 90, 90, 90, 100]
        for col, w in zip(self.COLUMNS, widths):
            self._tree.heading(col, text=col)
            self._tree.column(col, width=w, minwidth=w, anchor="center")
        self._tree.grid(row=0, column=0, sticky="nsew")
        vsb.grid(row=0, column=1, sticky="ns")
        hsb.grid(row=1, column=0, sticky="ew")
        self.rowconfigure(0, weight=1)
        self.columnconfigure(0, weight=1)

    def populate(self, records):
        self._tree.delete(*self._tree.get_children())
        for i, r in enumerate(records, 1):
            ops = getattr(r, "ops_per_sec_mean", None)
            self._tree.insert("", "end", values=(
                i,
                r.metadata.experiment_name or "—",
                r.metadata.allocator or "—",
                r.status or "NA",
                _ns_fmt(r.repetition_min_ns),
                _ns_fmt(r.repetition_median_ns),
                _ns_fmt(r.repetition_p95_ns),
                _ns_fmt(r.repetition_max_ns),
                f"{ops:.0f}" if ops is not None else "NA",
                _bytes_fmt(r.bytes_allocated_total),
            ))


class _ChartFrame(ttk.Frame):
    """Embedded matplotlib figure with navigation toolbar."""

    def __init__(self, parent, figsize=(10, 4), **kw):
        super().__init__(parent, **kw)
        self._fig = Figure(figsize=figsize, dpi=96, facecolor=_BG2)
        self._canvas = FigureCanvasTkAgg(self._fig, master=self)
        self._toolbar = NavigationToolbar2Tk(self._canvas, self, pack_toolbar=False)
        self._toolbar.pack(side="top", fill="x")
        self._canvas.get_tk_widget().pack(side="top", fill="both", expand=True)

    @property
    def fig(self) -> Figure:
        return self._fig

    def refresh(self):
        self._canvas.draw()

    def clear(self):
        self._fig.clf()
        self.refresh()


def _embed_png(chart: _ChartFrame, png_path: str):
    """Read a PNG file and display it inside a _ChartFrame."""
    chart.clear()
    try:
        from PIL import Image
        img = Image.open(png_path)
        ax = chart.fig.add_subplot(111)
        ax.imshow(img)
        ax.axis("off")
    except ImportError:
        img = plt.imread(png_path)
        ax = chart.fig.add_subplot(111)
        ax.imshow(img)
        ax.axis("off")
    chart.refresh()


# ---------------------------------------------------------------------------
# Page: Single Run
# ---------------------------------------------------------------------------

class PageSingleRun(ttk.Frame):

    def __init__(self, parent, **kw):
        super().__init__(parent, padding=12, **kw)
        self._run: Optional[RunModel] = None
        self._build()

    def _build(self):
        # ── Input ─────────────────────────────────────────────────────────────
        sec = _Section(self, "Run")
        sec.pack(fill="x", pady=(0, 6))
        row = ttk.Frame(sec)
        row.pack(fill="x")
        ttk.Label(row, text="Base path:").pack(side="left")
        self._path_var = tk.StringVar()
        ttk.Entry(row, textvariable=self._path_var, width=60).pack(
            side="left", padx=6, fill="x", expand=True
        )
        ttk.Button(row, text="Browse…", command=self._browse).pack(side="left", padx=(0, 6))
        ttk.Button(row, text="Load & Report", command=self._load,
                   style="Accent.TButton").pack(side="left")

        # ── Filters ───────────────────────────────────────────────────────────
        fsec = _Section(self, "Filters")
        fsec.pack(fill="x", pady=(0, 6))
        self._filters = _FilterBar(fsec)
        self._filters.pack(fill="x")

        # ── Status ────────────────────────────────────────────────────────────
        self._status = _StatusBar(self, font=_FONT_SM)
        self._status.pack(anchor="w", pady=(0, 4))

        # ── Metric cards ──────────────────────────────────────────────────────
        self._cards_frame = ttk.Frame(self)
        self._cards_frame.pack(fill="x", pady=(0, 6))

        # ── Paned: table / chart ──────────────────────────────────────────────
        paned = ttk.PanedWindow(self, orient="vertical")
        paned.pack(fill="both", expand=True)

        tsec = _Section(self, "Summary Metrics")
        self._table = _SummaryTable(tsec)
        self._table.pack(fill="both", expand=True)
        paned.add(tsec, weight=2)

        csec = _Section(self, "Latency Chart")
        self._chart = _ChartFrame(csec, figsize=(10, 3))
        self._chart.pack(fill="both", expand=True)
        paned.add(csec, weight=3)

        # ── Raw report ────────────────────────────────────────────────────────
        rsec = _Section(self, "Raw Markdown Report")
        rsec.pack(fill="x", pady=(6, 0))
        self._report_text = scrolledtext.ScrolledText(
            rsec, height=8, font=_MONO, state="disabled", wrap="none",
            bg=_BG2, fg=_FG, insertbackground=_FG,
        )
        self._report_text.pack(fill="both", expand=True)

    def _browse(self):
        path = filedialog.askopenfilename(
            title="Select run file",
            filetypes=[("CSV / JSONL", "*.csv *.jsonl"), ("All files", "*.*")],
        )
        if path:
            self._path_var.set(_strip_ext(path))

    def _load(self):
        self._status.info("Loading…")
        self.update_idletasks()

        run, err = _load_run(self._path_var.get())
        if err:
            self._status.err(err)
            return

        flt = self._filters.build_filter()
        eff = apply_filter(run, flt) if flt else run

        # Cards
        for w in self._cards_frame.winfo_children():
            w.destroy()
        _MetricCards(self._cards_frame, [
            ("Summary records",    len(eff.summary)),
            ("Timeseries records", len(eff.timeseries)),
            ("Experiments",        len(eff.experiments)),
            ("Allocators",         len(eff.allocators)),
            ("Run ID",             eff.metadata.run_id or "—"),
            ("Build type",         eff.metadata.build_type or "—"),
        ]).pack(fill="x")

        # Table
        self._table.populate(eff.summary)

        # Latency chart
        self._chart.clear()
        if eff.summary:
            import tempfile
            try:
                with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as tmp:
                    tmp_path = tmp.name
                with warnings.catch_warnings(record=True):
                    warnings.simplefilter("always")
                    plot_latency_summary(eff, tmp_path)
                _embed_png(self._chart, tmp_path)
            except Exception as exc:
                self._status.info(f"Chart not available: {exc}")
            finally:
                try:
                    os.unlink(tmp_path)
                except Exception:
                    pass

        # Raw report
        try:
            md = report_run(eff, flt)
        except Exception:
            md = traceback.format_exc()
        self._report_text.config(state="normal")
        self._report_text.delete("1.0", "end")
        self._report_text.insert("end", md)
        self._report_text.config(state="disabled")

        self._run = run
        self._status.ok(f"Loaded: {run.metadata.run_id or self._path_var.get().strip()}")


# ---------------------------------------------------------------------------
# Page: Compare Runs
# ---------------------------------------------------------------------------

class PageCompare(ttk.Frame):

    def __init__(self, parent, **kw):
        super().__init__(parent, padding=12, **kw)
        self._build()

    def _build(self):
        # ── Input ─────────────────────────────────────────────────────────────
        sec = _Section(self, "Runs")
        sec.pack(fill="x", pady=(0, 6))
        self._base_var = tk.StringVar()
        self._cand_var = tk.StringVar()
        for label, var in [("Baseline:", self._base_var), ("Candidate:", self._cand_var)]:
            row = ttk.Frame(sec)
            row.pack(fill="x", pady=2)
            ttk.Label(row, text=label, width=10).pack(side="left")
            ttk.Entry(row, textvariable=var, width=60).pack(
                side="left", padx=6, fill="x", expand=True
            )
            ttk.Button(row, text="Browse…",
                       command=lambda v=var: self._browse(v)).pack(side="left", padx=(0, 6))
        btn_row = ttk.Frame(sec)
        btn_row.pack(fill="x", pady=(6, 0))
        ttk.Button(btn_row, text="Load & Compare", command=self._compare,
                   style="Accent.TButton").pack(side="left")

        # ── Filters ───────────────────────────────────────────────────────────
        fsec = _Section(self, "Filters")
        fsec.pack(fill="x", pady=(0, 6))
        self._filters = _FilterBar(fsec)
        self._filters.pack(fill="x")

        # ── Status ────────────────────────────────────────────────────────────
        self._status = _StatusBar(self, font=_FONT_SM)
        self._status.pack(anchor="w", pady=(0, 4))

        # ── Cards ─────────────────────────────────────────────────────────────
        self._cards_frame = ttk.Frame(self)
        self._cards_frame.pack(fill="x", pady=(0, 6))

        # ── Paned: chart / diff ───────────────────────────────────────────────
        paned = ttk.PanedWindow(self, orient="vertical")
        paned.pack(fill="both", expand=True)

        csec = _Section(self, "Throughput Comparison")
        self._chart = _ChartFrame(csec, figsize=(10, 3))
        self._chart.pack(fill="both", expand=True)
        paned.add(csec, weight=3)

        dsec = _Section(self, "Diff Report")
        self._diff_text = scrolledtext.ScrolledText(
            dsec, height=12, font=_MONO, state="disabled", wrap="none",
            bg=_BG2, fg=_FG, insertbackground=_FG,
        )
        self._diff_text.pack(fill="both", expand=True)
        paned.add(dsec, weight=2)

    def _browse(self, var: tk.StringVar):
        path = filedialog.askopenfilename(
            title="Select run file",
            filetypes=[("CSV / JSONL", "*.csv *.jsonl"), ("All files", "*.*")],
        )
        if path:
            var.set(_strip_ext(path))

    def _compare(self):
        self._status.info("Loading…")
        self.update_idletasks()

        baseline, err = _load_run(self._base_var.get())
        if err:
            self._status.err(f"Baseline: {err}")
            return
        candidate, err = _load_run(self._cand_var.get())
        if err:
            self._status.err(f"Candidate: {err}")
            return

        flt = self._filters.build_filter()
        try:
            cmp = compare_runs(baseline, candidate, flt)
            diff_md = format_comparison(cmp)
        except Exception:
            self._status.err("Comparison failed.")
            messagebox.showerror("Error", traceback.format_exc())
            return

        # Cards
        for w in self._cards_frame.winfo_children():
            w.destroy()
        _MetricCards(self._cards_frame, [
            ("Matched records",     len(cmp.matched)),
            ("Unmatched baseline",  len(cmp.unmatched_base)),
            ("Unmatched candidate", len(cmp.unmatched_cand)),
            ("Baseline ID",         baseline.metadata.run_id or self._base_var.get().strip()),
            ("Candidate ID",        candidate.metadata.run_id or self._cand_var.get().strip()),
        ]).pack(fill="x")

        # Chart
        self._chart.clear()
        bid = baseline.metadata.run_id or self._base_var.get().strip()
        cid = candidate.metadata.run_id or self._cand_var.get().strip()
        beff = apply_filter(baseline, flt) if flt else baseline
        ceff = apply_filter(candidate, flt) if flt else candidate
        if beff.summary or ceff.summary:
            import tempfile
            try:
                with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as tmp:
                    tmp_path = tmp.name
                with warnings.catch_warnings(record=True):
                    warnings.simplefilter("always")
                    plot_throughput_comparison({bid: beff, cid: ceff}, tmp_path)
                _embed_png(self._chart, tmp_path)
            except Exception as exc:
                self._status.info(f"Chart not available: {exc}")
            finally:
                try:
                    os.unlink(tmp_path)
                except Exception:
                    pass

        # Diff
        self._diff_text.config(state="normal")
        self._diff_text.delete("1.0", "end")
        self._diff_text.insert("end", diff_md)
        self._diff_text.config(state="disabled")

        self._status.ok(
            f"Compared  {baseline.metadata.run_id or 'baseline'}  vs  "
            f"{candidate.metadata.run_id or 'candidate'}"
        )


# ---------------------------------------------------------------------------
# Page: Plots
# ---------------------------------------------------------------------------

class PagePlots(ttk.Frame):

    def __init__(self, parent, **kw):
        super().__init__(parent, padding=12, **kw)
        self._last_png: Optional[str] = None
        self._build()

    def _build(self):
        # ── Input ─────────────────────────────────────────────────────────────
        sec = _Section(self, "Primary Run")
        sec.pack(fill="x", pady=(0, 6))
        row = ttk.Frame(sec)
        row.pack(fill="x")
        ttk.Label(row, text="Base path:").pack(side="left")
        self._path_var = tk.StringVar()
        ttk.Entry(row, textvariable=self._path_var, width=60).pack(
            side="left", padx=6, fill="x", expand=True
        )
        ttk.Button(row, text="Browse…", command=self._browse).pack(side="left")

        # ── Options ───────────────────────────────────────────────────────────
        osec = _Section(self, "Plot Options")
        osec.pack(fill="x", pady=(0, 6))
        og = ttk.Frame(osec)
        og.pack(fill="x")

        ttk.Label(og, text="Plot type:").grid(row=0, column=0, sticky="w", padx=(0, 6))
        self._kind_var = tk.StringVar(value="latency")
        kind_cb = ttk.Combobox(
            og, textvariable=self._kind_var,
            values=["latency", "throughput", "timeseries"],
            state="readonly", width=18,
        )
        kind_cb.grid(row=0, column=1, sticky="w", padx=(0, 20))
        kind_cb.bind("<<ComboboxSelected>>", self._on_kind_change)

        self._extra_lbl = ttk.Label(og, text="Extra runs (one per line):")
        self._extra_lbl.grid(row=0, column=2, sticky="w", padx=(0, 6))
        self._extra_text = tk.Text(og, height=3, width=40, font=_MONO,
                                   bg=_BG2, fg=_FG, insertbackground=_FG)
        self._extra_text.grid(row=0, column=3, rowspan=2, sticky="ew", padx=(0, 6))
        og.columnconfigure(3, weight=1)

        self._metric_lbl = ttk.Label(og, text="Metric key:")
        self._metric_lbl.grid(row=1, column=0, sticky="w", padx=(0, 6), pady=(4, 0))
        self._metric_var = tk.StringVar()
        self._metric_entry = ttk.Entry(og, textvariable=self._metric_var, width=24)
        self._metric_entry.grid(row=1, column=1, sticky="w", pady=(4, 0))

        self._on_kind_change()  # initial visibility

        # ── Filters ───────────────────────────────────────────────────────────
        fsec = _Section(self, "Filters")
        fsec.pack(fill="x", pady=(0, 6))
        self._filters = _FilterBar(fsec)
        self._filters.pack(fill="x")

        # ── Buttons ───────────────────────────────────────────────────────────
        btn_row = ttk.Frame(self)
        btn_row.pack(fill="x", pady=(0, 6))
        ttk.Button(btn_row, text="Generate Plot", command=self._generate,
                   style="Accent.TButton").pack(side="left")
        ttk.Button(btn_row, text="Save PNG…", command=self._save).pack(side="left", padx=8)

        # ── Status ────────────────────────────────────────────────────────────
        self._status = _StatusBar(self, font=_FONT_SM)
        self._status.pack(anchor="w", pady=(0, 4))

        # ── Chart ─────────────────────────────────────────────────────────────
        csec = _Section(self, "Chart")
        csec.pack(fill="both", expand=True)
        self._chart = _ChartFrame(csec, figsize=(10, 5))
        self._chart.pack(fill="both", expand=True)

    def _on_kind_change(self, _event=None):
        kind = self._kind_var.get()
        if kind == "throughput":
            self._extra_lbl.grid()
            self._extra_text.grid()
            self._metric_lbl.grid_remove()
            self._metric_entry.grid_remove()
        elif kind == "timeseries":
            self._extra_lbl.grid_remove()
            self._extra_text.grid_remove()
            self._metric_lbl.grid()
            self._metric_entry.grid()
        else:
            self._extra_lbl.grid_remove()
            self._extra_text.grid_remove()
            self._metric_lbl.grid_remove()
            self._metric_entry.grid_remove()

    def _browse(self):
        path = filedialog.askopenfilename(
            title="Select run file",
            filetypes=[("CSV / JSONL", "*.csv *.jsonl"), ("All files", "*.*")],
        )
        if path:
            self._path_var.set(_strip_ext(path))

    def _generate(self):
        import tempfile
        kind = self._kind_var.get()
        if kind == "timeseries" and not self._metric_var.get().strip():
            messagebox.showerror("Error", "Metric key is required for timeseries plots.")
            return

        self._status.info("Loading…")
        self.update_idletasks()

        run, err = _load_run(self._path_var.get())
        if err:
            self._status.err(err)
            return

        flt = self._filters.build_filter()
        tmp_path = None
        try:
            with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as tmp:
                tmp_path = tmp.name

            with warnings.catch_warnings(record=True):
                warnings.simplefilter("always")

                if kind == "latency":
                    eff = apply_filter(run, flt) if flt else run
                    plot_latency_summary(eff, tmp_path)

                elif kind == "throughput":
                    runs_by_label: dict = {}
                    label = run.metadata.run_id or self._path_var.get().strip()
                    runs_by_label[label] = apply_filter(run, flt) if flt else run
                    for xp in [
                        l.strip()
                        for l in self._extra_text.get("1.0", "end").splitlines()
                        if l.strip()
                    ]:
                        xrun, xerr = _load_run(xp)
                        if xerr:
                            self._status.err(f"Extra run {xp!r}: {xerr}")
                            return
                        runs_by_label[xrun.metadata.run_id or xp] = (
                            apply_filter(xrun, flt) if flt else xrun
                        )
                    plot_throughput_comparison(runs_by_label, tmp_path)

                elif kind == "timeseries":
                    plot_timeseries_trend(
                        run, self._metric_var.get().strip(), tmp_path,
                        timeseries_filter=flt,
                    )

            # Keep PNG for Save button
            if self._last_png and os.path.exists(self._last_png):
                try:
                    os.unlink(self._last_png)
                except OSError:
                    pass
            self._last_png = tmp_path
            tmp_path = None  # don't delete in finally

            _embed_png(self._chart, self._last_png)
            self._status.ok(f"{kind.capitalize()} plot ready.")

        except ValueError as exc:
            self._status.err(str(exc))
        except Exception:
            self._status.err("Unexpected error — see console.")
            print(traceback.format_exc())
        finally:
            if tmp_path:
                try:
                    os.unlink(tmp_path)
                except OSError:
                    pass

    def _save(self):
        if not self._last_png or not os.path.exists(self._last_png):
            messagebox.showinfo("Nothing to save", "Generate a plot first.")
            return
        dest = filedialog.asksaveasfilename(
            defaultextension=".png",
            filetypes=[("PNG image", "*.png")],
            title="Save plot as PNG",
        )
        if dest:
            import shutil
            shutil.copy2(self._last_png, dest)
            self._status.ok(f"Saved → {dest}")


# ---------------------------------------------------------------------------
# Batch run discovery helpers
# ---------------------------------------------------------------------------

_RUNS_ROOT = (_BENCHMARKS_DIR.parent / "runs").resolve()


def _discover_batch_runs(runs_root: Path) -> list[dict]:
    """
    Scan runs_root recursively for manifest.json files.
    Returns list of dicts sorted newest-first (by batch_id desc).
    """
    import json as _json
    results = []
    if not runs_root.exists():
        return results
    for manifest_path in sorted(runs_root.rglob("manifest.json"), reverse=True):
        try:
            with open(manifest_path, encoding="utf-8") as f:
                m = _json.load(f)
            if m.get("schema_version") != "batch_manifest.v1":
                continue
            results.append({
                "batch_id":       m.get("batch_id", manifest_path.parent.name),
                "run_prefix":     m.get("run_prefix", ""),
                "scenario_count": m.get("scenario_count", 0),
                "duration_ms":    m.get("total_duration_ms", 0),
                "build_type":     m.get("environment", {}).get("build_type", ""),
                "cpu":            m.get("environment", {}).get("cpu_model", ""),
                "dir":            str(manifest_path.parent),
                "manifest":       m,
            })
        except Exception:
            pass
    return results


def _fmt_ms(ms) -> str:
    try:
        ms = int(ms)
    except Exception:
        return "?"
    if ms >= 60000:
        return f"{ms//60000}m {(ms%60000)//1000}s"
    if ms >= 1000:
        return f"{ms/1000:.1f}s"
    return f"{ms}ms"


# ---------------------------------------------------------------------------
# Page: Batch Runs
# ---------------------------------------------------------------------------

class PageBatchRuns(ttk.Frame):
    """Auto-discover and browse all batch runs from the runs/ directory."""

    _SCENARIO_COLS = (
        "Scenario", "Allocator", "Status", "Duration",
        "Median", "ops/sec", "Overhead", "Fallback",
    )

    def __init__(self, parent, **kw):
        super().__init__(parent, padding=8, **kw)
        self._runs: list[dict] = []
        self._current_run: Optional[dict] = None
        self._scenario_model_cache: dict[str, object] = {}
        self._build()
        self.after(100, self._refresh)

    def _build(self):
        # ── Top bar ───────────────────────────────────────────────────────────
        top = ttk.Frame(self)
        top.pack(fill="x", pady=(0, 6))
        ttk.Label(top, text="Runs root:").pack(side="left")
        self._root_var = tk.StringVar(value=str(_RUNS_ROOT))
        ttk.Entry(top, textvariable=self._root_var, width=55).pack(
            side="left", padx=6, fill="x", expand=True
        )
        ttk.Button(top, text="Browse…", command=self._browse_root).pack(side="left", padx=(0,4))
        ttk.Button(top, text="🔄 Refresh", command=self._refresh,
                   style="Accent.TButton").pack(side="left")
        ttk.Button(top, text="▶ Run Benchmark", command=self._run_benchmark,
                   style="Accent.TButton").pack(side="left", padx=(8,0))
        self._status = _StatusBar(self, font=_FONT_SM)
        self._status.pack(anchor="w", pady=(0, 4))

        # ── Main paned: run list | detail ─────────────────────────────────────
        main_paned = ttk.PanedWindow(self, orient="horizontal")
        main_paned.pack(fill="both", expand=True)

        # Left: run list
        left = _Section(self, "Discovered Runs")
        vsb_l = ttk.Scrollbar(left, orient="vertical")
        self._run_list = tk.Listbox(
            left, font=_FONT_SM, bg=_BG2, fg=_FG,
            selectbackground=_ACCENT, selectforeground=_BG,
            activestyle="none", yscrollcommand=vsb_l.set, width=30,
        )
        vsb_l.config(command=self._run_list.yview)
        self._run_list.pack(side="left", fill="both", expand=True)
        vsb_l.pack(side="right", fill="y")
        self._run_list.bind("<<ListboxSelect>>", self._on_run_select)
        main_paned.add(left, weight=1)

        # Right: detail panel (notebook)
        right = ttk.Frame(self)
        main_paned.add(right, weight=4)

        # Run info cards
        self._cards_frame = ttk.Frame(right)
        self._cards_frame.pack(fill="x", pady=(0, 4))

        # Detail notebook: Scenarios | Chart | Drill-down
        self._detail_nb = ttk.Notebook(right)
        self._detail_nb.pack(fill="both", expand=True)

        # Tab 1: scenario table
        tab_table = ttk.Frame(self._detail_nb, padding=4)
        self._detail_nb.add(tab_table, text="📋  Scenarios")
        self._build_scenario_table(tab_table)

        # Tab 2: latency chart
        tab_chart = ttk.Frame(self._detail_nb, padding=4)
        self._detail_nb.add(tab_chart, text="📊  Latency Chart")
        self._chart = _ChartFrame(tab_chart, figsize=(10, 4))
        self._chart.pack(fill="both", expand=True)

        # Tab 3: scenario drill-down
        tab_drill = ttk.Frame(self._detail_nb, padding=4)
        self._detail_nb.add(tab_drill, text="🔍  Drill-down")
        self._build_drilldown(tab_drill)

    def _build_scenario_table(self, parent):
        vsb = ttk.Scrollbar(parent, orient="vertical")
        hsb = ttk.Scrollbar(parent, orient="horizontal")
        self._scen_tree = ttk.Treeview(
            parent, columns=self._SCENARIO_COLS, show="headings",
            yscrollcommand=vsb.set, xscrollcommand=hsb.set, selectmode="browse",
        )
        vsb.config(command=self._scen_tree.yview)
        hsb.config(command=self._scen_tree.xview)
        widths = [260, 120, 60, 70, 90, 90, 80, 80]
        for col, w in zip(self._SCENARIO_COLS, widths):
            self._scen_tree.heading(col, text=col)
            self._scen_tree.column(col, width=w, minwidth=w, anchor="center")
        self._scen_tree.column("Scenario", anchor="w")
        self._scen_tree.grid(row=0, column=0, sticky="nsew")
        vsb.grid(row=0, column=1, sticky="ns")
        hsb.grid(row=1, column=0, sticky="ew")
        parent.rowconfigure(0, weight=1)
        parent.columnconfigure(0, weight=1)
        self._scen_tree.bind("<<TreeviewSelect>>", self._on_scenario_select)

    def _build_drilldown(self, parent):
        self._drill_status = _StatusBar(parent, font=_FONT_SM)
        self._drill_status.pack(anchor="w")
        self._drill_cards = ttk.Frame(parent)
        self._drill_cards.pack(fill="x", pady=(0, 4))
        paned = ttk.PanedWindow(parent, orient="vertical")
        paned.pack(fill="both", expand=True)
        tsec = _Section(parent, "Scenario Metrics")
        self._drill_table = _SummaryTable(tsec)
        self._drill_table.pack(fill="both", expand=True)
        paned.add(tsec, weight=1)
        rsec = _Section(parent, "Report")
        self._drill_report = scrolledtext.ScrolledText(
            rsec, height=8, font=_MONO, state="disabled", wrap="none",
            bg=_BG2, fg=_FG, insertbackground=_FG,
        )
        self._drill_report.pack(fill="both", expand=True)
        paned.add(rsec, weight=1)

    # ── Actions ───────────────────────────────────────────────────────────────

    def _browse_root(self):
        d = filedialog.askdirectory(title="Select runs root directory",
                                    initialdir=self._root_var.get())
        if d:
            self._root_var.set(d)
            self._refresh()

    def _refresh(self):
        self._status.info("Scanning…")
        self.update_idletasks()
        root = Path(self._root_var.get())
        self._runs = _discover_batch_runs(root)
        self._run_list.delete(0, "end")
        for r in self._runs:
            label = f"{r['batch_id']}  [{r['scenario_count']} sc, {_fmt_ms(r['duration_ms'])}]"
            self._run_list.insert("end", label)
        if self._runs:
            self._status.ok(f"Found {len(self._runs)} batch run(s) in {root}")
        else:
            self._status.info(f"No batch runs found in {root}")

    def _on_run_select(self, _event=None):
        sel = self._run_list.curselection()
        if not sel:
            return
        run_info = self._runs[sel[0]]
        if run_info is self._current_run:
            return
        self._current_run = run_info
        self._scenario_model_cache.clear()
        self._load_run_detail(run_info)

    def _load_run_detail(self, run_info: dict):
        self._status.info(f"Loading {run_info['batch_id']}…")
        self.update_idletasks()

        # Cards
        for w in self._cards_frame.winfo_children():
            w.destroy()
        env = run_info["manifest"].get("environment", {})
        _MetricCards(self._cards_frame, [
            ("Batch ID",    run_info["batch_id"]),
            ("Prefix",      run_info["run_prefix"]),
            ("Scenarios",   run_info["scenario_count"]),
            ("Duration",    _fmt_ms(run_info["duration_ms"])),
            ("Build",       env.get("build_type", "?")),
            ("CPU",         env.get("cpu_model", "?")[:40]),
            ("Compiler",    f"{env.get('compiler_name','')} {env.get('compiler_version','')}"),
            ("Git SHA",     env.get("git_sha", "?")[:12]),
        ]).pack(fill="x")

        # Populate scenario table from manifest
        self._scen_tree.delete(*self._scen_tree.get_children())
        scenarios = run_info["manifest"].get("scenarios", [])

        # Try to load summary.csv for rich metrics
        summary_map: dict = {}
        summary_path = str(Path(run_info["dir"]) / "summary")
        try:
            with warnings.catch_warnings(record=True):
                warnings.simplefilter("always")
                batch_run = assemble_run_from_files(summary_path, validate=False)
            for rec in batch_run.summary:
                key = rec.metadata.experiment_name or ""
                summary_map[key] = rec
        except Exception:
            pass

        for sc in scenarios:
            name = sc.get("name", "")
            status = sc.get("status", "")
            dur = _fmt_ms(sc.get("duration_ms", 0))
            rec = summary_map.get(name)
            allocator = ""
            median = ""
            ops = ""
            overhead = ""
            fallback = ""
            if rec:
                # allocator from name segment
                parts = name.split("/")
                allocator = parts[1] if len(parts) > 1 else (rec.metadata.allocator or "")
                median = _ns_fmt(rec.repetition_median_ns)
                ops_val = getattr(rec, "ops_per_sec_mean", None)
                ops = f"{ops_val/1e6:.1f}M" if ops_val else ""
                oh = getattr(rec, "overhead_ratio", None)
                overhead = f"{oh:.2f}×" if oh else ""
                fb = getattr(rec, "fallback_count", None)
                fallback = str(int(fb)) if fb else ""
            else:
                parts = name.split("/")
                allocator = parts[1] if len(parts) > 1 else ""

            tag = "ok" if status == "success" else ("fail" if status else "")
            self._scen_tree.insert("", "end", iid=name, values=(
                name, allocator, status, dur, median, ops, overhead, fallback,
            ), tags=(tag,))

        self._scen_tree.tag_configure("ok",   foreground=_GREEN)
        self._scen_tree.tag_configure("fail", foreground=_RED)

        # Latency chart from batch summary
        self._chart.clear()
        if summary_map:
            import tempfile
            try:
                with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as tmp:
                    tmp_path = tmp.name
                with warnings.catch_warnings(record=True):
                    warnings.simplefilter("always")
                    plot_latency_summary(batch_run, tmp_path)
                _embed_png(self._chart, tmp_path)
            except Exception as exc:
                self._status.info(f"Chart unavailable: {exc}")
            finally:
                try:
                    os.unlink(tmp_path)
                except Exception:
                    pass

        self._status.ok(
            f"Loaded {run_info['batch_id']} — "
            f"{len(scenarios)} scenarios, {len(summary_map)} with metrics"
        )

    def _on_scenario_select(self, _event=None):
        sel = self._scen_tree.selection()
        if not sel:
            return
        name = sel[0]  # iid == scenario name
        self._load_scenario_drilldown(name)
        self._detail_nb.select(2)  # switch to drill-down tab

    def _load_scenario_drilldown(self, name: str):
        if not self._current_run:
            return

        self._drill_status.info(f"Loading {name}…")
        self.update_idletasks()

        # Find output_dir from manifest
        output_dir = None
        for sc in self._current_run["manifest"].get("scenarios", []):
            if sc.get("name") == name:
                output_dir = sc.get("output_dir")
                break

        if not output_dir:
            self._drill_status.err(f"No output_dir for {name!r}")
            return

        base_path = str(Path(self._current_run["dir"]) / output_dir / "data")

        # Lazy cache
        if name in self._scenario_model_cache:
            run = self._scenario_model_cache[name]
        else:
            try:
                with warnings.catch_warnings(record=True):
                    warnings.simplefilter("always")
                    run = assemble_run_from_files(base_path, validate=False)
                self._scenario_model_cache[name] = run
            except Exception as exc:
                self._drill_status.err(str(exc))
                return

        # Cards
        for w in self._drill_cards.winfo_children():
            w.destroy()
        if run.summary:
            r = run.summary[0]
            ops = getattr(r, "ops_per_sec_mean", None)
            oh  = getattr(r, "overhead_ratio", None)
            fb  = getattr(r, "fallback_count", None)
            reserved = getattr(r, "reserved_bytes", None)
            _MetricCards(self._drill_cards, [
                ("Scenario",      name.split("/", 1)[-1]),
                ("Allocator",     r.metadata.allocator or "—"),
                ("Median",        _ns_fmt(r.repetition_median_ns)),
                ("ops/sec",       f"{ops/1e6:.2f}M" if ops else "NA"),
                ("Overhead",      f"{oh:.3f}×" if oh else "NA"),
                ("Fallback",      str(int(fb)) if fb else "0"),
                ("Reserved mem",  _bytes_fmt(reserved) if reserved else "NA"),
            ]).pack(fill="x")

        # Table + report
        self._drill_table.populate(run.summary)
        try:
            from analysis.report import report_run
            md = report_run(run, None)
        except Exception:
            md = traceback.format_exc()
        self._drill_report.config(state="normal")
        self._drill_report.delete("1.0", "end")
        self._drill_report.insert("end", md)
        self._drill_report.config(state="disabled")

        self._drill_status.ok(f"Scenario: {name}")

    def _run_benchmark(self):
        """Placeholder for the benchmark execution logic."""
        messagebox.showinfo("Run Benchmark", "Benchmark execution is not yet implemented.")


# ---------------------------------------------------------------------------
# Page: Help
# ---------------------------------------------------------------------------

class PageHelp(ttk.Frame):

    def __init__(self, parent, **kw):
        super().__init__(parent, padding=12, **kw)
        text = scrolledtext.ScrolledText(
            self, font=_FONT, wrap="word", state="normal",
            bg=_BG2, fg=_FG, insertbackground=_FG,
        )
        text.pack(fill="both", expand=True)
        text.insert("end", _HELP_TEXT)
        text.config(state="disabled")


_HELP_TEXT = """\
BENCHMARK ANALYSIS — Desktop UI
════════════════════════════════════════════════════════════════════════

HOW TO USE
──────────

📄  Single Run
    Enter (or Browse…) the base path of a run — without .csv / .jsonl extension.
    Example:  results/run1   →   loads results/run1.csv + results/run1.jsonl
    Press "Load & Report" to see metrics, table, and latency chart.

⚖️  Compare Runs
    Enter two base paths — baseline and candidate.
    Records are matched by (experiment, allocator, repetition index).
    The diff report shows % change for all numeric metrics.
    NA on either side is never treated as 0.

📈  Plots
    Three plot types:
      • latency     — min / median / p95 / max per experiment / allocator
      • throughput  — ops/sec across multiple runs (add extras one per line)
      • timeseries  — numeric payload metric vs elapsed time (enter metric key)
    Use "Save PNG…" to export the chart.

FILTERS
───────
    All pages support filtering by:
      Allocators   — e.g.  tlsf, mimalloc
      Experiments  — e.g.  exp_mixed_sizes
      Workloads    — optional
      Profiles     — optional
    Values are comma-separated.  AND semantics across dimensions.

INPUT FILES
───────────
    File         Schema       Content
    ─────────    ──────────   ──────────────────────────────────────────
    *.csv        summary.v2   Aggregated per-repetition metrics
    *.jsonl      ts.v2        Time-series event / tick / warning log

LAUNCH
──────
    python analysis/ui.py
    python -m analysis.ui
    run_ui.bat           (Windows)
    ./run_ui.sh          (Linux / macOS)
"""


# ---------------------------------------------------------------------------
# Main application window
# ---------------------------------------------------------------------------

class App(tk.Tk):

    def __init__(self):
        super().__init__()
        self.title("Benchmark Analysis")
        self.geometry("1200x800")
        self.minsize(900, 600)
        self._apply_theme()
        self._build()

    def _apply_theme(self):
        style = ttk.Style(self)
        for preferred in ("clam", "alt", "default"):
            if preferred in style.theme_names():
                style.theme_use(preferred)
                break

        self.configure(bg=_BG)
        style.configure(".", background=_BG, foreground=_FG, font=_FONT,
                        bordercolor=_BORDER, troughcolor=_BG2,
                        selectbackground=_ACCENT)
        style.configure("TFrame",      background=_BG)
        style.configure("TLabelframe", background=_BG, foreground=_ACCENT,
                        bordercolor=_BORDER)
        style.configure("TLabelframe.Label", background=_BG, foreground=_ACCENT,
                        font=_FONT_B)
        style.configure("TLabel",   background=_BG, foreground=_FG)
        style.configure("TEntry",   fieldbackground=_BG2, foreground=_FG,
                        insertcolor=_FG, bordercolor=_BORDER)
        style.configure("TButton",  background=_BG2, foreground=_FG,
                        bordercolor=_BORDER, focuscolor=_ACCENT)
        style.map("TButton",
                  background=[("active", _BORDER), ("pressed", _ACCENT)],
                  foreground=[("active", _FG)])
        style.configure("Accent.TButton", background=_ACCENT, foreground=_BG,
                        font=_FONT_B)
        style.map("Accent.TButton",
                  background=[("active", "#74c7ec"), ("pressed", "#89dceb")])
        style.configure("TNotebook",     background=_BG2, bordercolor=_BORDER)
        style.configure("TNotebook.Tab", background=_BG2, foreground=_FG,
                        padding=[14, 6], font=_FONT)
        style.map("TNotebook.Tab",
                  background=[("selected", _BG)],
                  foreground=[("selected", _ACCENT)])
        style.configure("Treeview", background=_BG2, foreground=_FG,
                        fieldbackground=_BG2, bordercolor=_BORDER, rowheight=22)
        style.configure("Treeview.Heading", background=_BORDER, foreground=_ACCENT,
                        font=_FONT_B)
        style.map("Treeview",
                  background=[("selected", _ACCENT)],
                  foreground=[("selected", _BG)])
        style.configure("TCombobox", fieldbackground=_BG2, foreground=_FG,
                        background=_BG2, arrowcolor=_ACCENT)
        style.configure("TScrollbar", background=_BG2, troughcolor=_BG,
                        arrowcolor=_FG)
        style.configure("TPanedwindow", background=_BORDER)
        # matplotlib dark theme
        plt.rcParams.update({
            "figure.facecolor": _BG2,
            "axes.facecolor":   _BG2,
            "axes.edgecolor":   _BORDER,
            "axes.labelcolor":  _FG,
            "xtick.color":      _FG,
            "ytick.color":      _FG,
            "text.color":       _FG,
            "grid.color":       _BORDER,
            "legend.facecolor": _BG,
            "legend.edgecolor": _BORDER,
        })

    def _build(self):
        # ── Menu bar ──────────────────────────────────────────────────────────
        menubar = tk.Menu(self, bg=_BG2, fg=_FG,
                          activebackground=_ACCENT, activeforeground=_BG,
                          tearoff=False)
        file_menu = tk.Menu(menubar, bg=_BG2, fg=_FG,
                            activebackground=_ACCENT, activeforeground=_BG,
                            tearoff=False)
        file_menu.add_command(label="Open run…", command=self._open_run)
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=self.destroy)
        menubar.add_cascade(label="File", menu=file_menu)

        help_menu = tk.Menu(menubar, bg=_BG2, fg=_FG,
                            activebackground=_ACCENT, activeforeground=_BG,
                            tearoff=False)
        help_menu.add_command(label="Help", command=lambda: self._notebook.select(4))
        help_menu.add_command(label="About", command=self._about)
        menubar.add_cascade(label="Help", menu=help_menu)
        self.config(menu=menubar)

        # ── Notebook tabs ─────────────────────────────────────────────────────
        self._notebook = ttk.Notebook(self)
        self._notebook.pack(fill="both", expand=True, padx=8, pady=8)

        self._page_batch = PageBatchRuns(self._notebook)
        self._notebook.add(self._page_batch, text="🗂️  Batch Runs")
        self._notebook.select(0)

    def _open_run(self):
        path = filedialog.askopenfilename(
            title="Select run CSV or JSONL file",
            filetypes=[("CSV / JSONL", "*.csv *.jsonl"), ("All files", "*.*")],
        )
        if path:
            self._page_single._path_var.set(_strip_ext(path))
            self._notebook.select(0)

    def _about(self):
        messagebox.showinfo(
            "About",
            "Benchmark Analysis Desktop UI\n\n"
            "Offline analysis tool for coreapp benchmark outputs.\n\n"
            "Python · Tkinter · matplotlib",
        )


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    app = App()
    app.mainloop()


if __name__ == "__main__":
    main()

