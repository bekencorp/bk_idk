#!/usr/bin/env python3
import click

VERSION = '1.0.0.3'

def get_version():
    """returns the version of the tool."""
    return VERSION

@click.command()
def version():
    """Displays the version of the tool."""
    click.echo(f'main.py, version {get_version()}')