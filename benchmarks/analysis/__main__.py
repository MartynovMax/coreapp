"""
__main__.py — allows the package to be invoked as:

    python -m analysis <command> [options]

Equivalent to:

    python -m analysis.cli <command> [options]
"""
from analysis.cli import main

main()

