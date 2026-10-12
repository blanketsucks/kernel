from __future__ import annotations

from typing import Any

from dataclasses import dataclass, field
from enum import StrEnum, IntEnum
from urllib.parse import urlparse
from pathlib import Path
import contextlib
import subprocess
import argparse
import difflib
import shutil
import types
import toml
import sys
import os

CWD = Path(__file__).parent
SYSROOT = Path(__file__).parent.parent.parent / 'root'

PREFIX = SYSROOT / 'usr'

BIN = PREFIX / 'bin'
LIB = PREFIX / 'lib'

class PortStorage:
    def __init__(self) -> None:
        self._ports: dict[str, Port] = {}
        self._dependecies: dict[str, list[Port]] = {}

    def all(self) -> list[Port]:
        return list(self._ports.values()) 

    def names(self) -> list[str]:
        return list(self._ports.keys())

    def load(self) -> None:
        for file in CWD.glob('*/*.toml'):
            port = Port.from_dict(file.parent, toml.load(file))
            self._ports[port.name] = port

    def resolve(self, name: str) -> Port | None:
        return self._ports.get(name)

    def build_candidates(self, to_exclude: list[str]) -> list[Port]:
        ports: list[Port] = []
        for port in self._ports.values():
            if port.state is PortState.Built:
                continue
            elif len(port.dependencies) == 0:
                ports.append(port)
                continue

            if any(self._ports[dep].state is PortState.Pending for dep in port.dependencies):
                continue

            if port.name in to_exclude:
                continue

            ports.append(port)

        return ports

class SourceType(StrEnum):
    Git = 'git'
    Tar = 'tar'

class BuildType(StrEnum):
    Configure = 'configure'
    Make = 'make'

class PortState(IntEnum):
    Pending = 0
    Built = 1


_curr_port: Port | None = None
def set_current_port(port: Port | None) -> None:
    global _curr_port
    _curr_port = port

def dbgln(*args: object) -> None:
    if _curr_port:
        print(f'[{_curr_port.name}]', *args)
    else:
        print(*args)

def run(*args: str) -> subprocess.CompletedProcess[bytes]:
    dbgln(f"Running '{' '.join(args)}'")
    process = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    if process.returncode != 0:
        print()
        print(process.stderr.decode())
        process.check_returncode()

    return process

@contextlib.contextmanager
def chdir(path: str | Path):
    cwd = os.getcwd()
    os.chdir(path)
    yield
    os.chdir(cwd)

@dataclass
class Source:
    type: SourceType
    url: str
    commit: str | None = field(default=None)
    directory: str | None = field(default=None)

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> Source:
        return Source(
            type=SourceType(data['type']),
            url=data['url'],
            commit=data.get('commit'),
            directory=data.get('directory')
        )

    @property
    def basename(self) -> str:
        if self.directory:
            return self.directory

        url = urlparse(self.url)
        basename = os.path.basename(url.path)

        return basename.removesuffix('.git')

    def fetch(self) -> None:
        dbgln(f'Fetching source from {self.type.value}.')
        match self.type:
            case SourceType.Git:
                self._fetch_git()
            case SourceType.Tar:
                self._fetch_tar()

    def _fetch_git(self) -> None:
        run('git', 'clone', self.url)
        if not self.commit:
            return

        with chdir(self.basename):
            run('git', 'checkout', self.commit)

    def _fetch_tar(self) -> None:
        url = urlparse(self.url)
        basename = os.path.basename(url.path)

        run('wget', self.url)
        run('tar', 'xvf', basename)

        os.remove(basename)
    
@dataclass
class Build:
    type: BuildType
    directory: Path
    binary: Path | None

    install: bool = field(default=False)

    override_prefix: bool = field(default=False)
    manual_install: bool = field(default=False)

    extra_configure_flags: list[str] | None = field(default=None)
    extra_cflags: list[str] = field(default_factory=list)

    make_targets: list[str] = field(default_factory=list)

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> Build:
        binary = data.get('binary')

        return Build(
            type=BuildType(data['type']),
            directory=Path(data['directory']),
            binary=Path(binary) if binary else None,
            override_prefix=data.get('override_prefix', False),
            extra_configure_flags=data.get('extra_configure_flags'),
            make_targets=data.get('make_targets', []),
            manual_install=data.get('manual_install', False),
            install=data.get('install', False)
        )

    def _run_make(self) -> None:
        run('make')
        for target in self.make_targets:
            run('make', target)

    def _run_make_install(self) -> None:
        run('make', f'DESTDIR={str(SYSROOT)}', 'install')

    def run(self) -> Path | None:
        match self.type:
            case BuildType.Configure:
                flags: list[str] = ['--host', 'x86_64-corn']
                if self.override_prefix:
                    flags.extend(['--prefix', '/usr'])

                flags.extend((self.extra_configure_flags or []))
                cflags = ' '.join(self.extra_cflags)

                if cflags:
                    run(f'CFLAGS="{cflags}"', './configure', *flags)
                else:
                    run('./configure', *flags)

                self._run_make()
                if self.install:
                    self._run_make_install()

                return self.binary.resolve() if self.binary else None
            case BuildType.Make:
                with chdir(self.directory):
                    self._run_make()
                    binary = self.binary.resolve() if self.binary else None

                return binary

@dataclass
class Port:
    directory: Path
    name: str
    version: str
    source: Source
    patches: list[Path]
    build: Build
    dependencies: list[str]

    state: PortState = PortState.Pending

    @classmethod
    def from_dict(cls, directory: Path, data: dict[str, Any]) -> Port:
        return Port(
            directory=directory,
            name=data['name'],
            version=data['version'],
            source=Source.from_dict(data['port']['source']),
            patches=[(directory / p).resolve() for p in data.get('patches', [])],
            build=Build.from_dict(data['port']['build']),
            dependencies=data.get('dependencies', [])
        )

    def is_git_source(self) -> bool:
        return self.source.type is SourceType.Git

    def is_tar_source(self) -> bool:
        return self.source.type is SourceType.Tar

    def apply_patches(self) -> None:
        dbgln(f'Applying {len(self.patches)} patches.')

        with chdir(self.source.basename):
            for patch in self.patches:
                run('patch', '-l', '-p1', '-i', str(patch))

    def install(self, binary: Path) -> None:
        dst = BIN / binary.parts[-1]
        dbgln(f'{dst} already exists. Removing.')

        if dst.exists():
            dst.unlink()

        try:
            shutil.move(binary, BIN)
        except shutil.Error as exc:
            dbgln(f'Could not move {binary} into {BIN}: {exc}')
        else:
            dbgln(f'Successfully moved {binary} into {BIN}.')

    def run(self) -> None:
        set_current_port(self)

        with chdir(self.directory):
            if os.path.exists(self.source.basename):
                shutil.rmtree(self.source.basename)

            self.source.fetch()
            self.apply_patches()

            with chdir(self.source.basename):
                binary = self.build.run()

            if self.build.manual_install and binary:
                self.install(binary)

        self.state = PortState.Built
        set_current_port(None)

        print()

    def print(self) -> None:
        print(f'{self.name}:')
        print(f'  - Version: {self.version}')

        if self.dependencies:
            print(f'  - Dependencies: {', '.join(self.dependencies)}')

        if self.source.commit:
            print(f'  - Source: {self.source.url} (commit: {self.source.commit})')
        else:
            print(f'  - Source: {self.source.url}')

        print()

class Args(types.SimpleNamespace):
    only: str | None
    list: bool
    exclude: str | None

def main() -> int:
    parser = argparse.ArgumentParser()

    parser.add_argument('--only', default=None, required=False, help='Only build this port.')
    parser.add_argument('--list', action='store_true', required=False, help='List all the ports')
    parser.add_argument('--exclude', required=False, help='Comma seperated list of ports to exclude while building')

    args = parser.parse_args(namespace=Args())

    ports = PortStorage()
    ports.load()

    if args.list:
        print('List of all the available ports:', end='\n\n')
        for port in ports.all():
            port.print()

        return 0

    if args.only:
        if args.exclude:
            print('Cannot mix --exclude and --only.')
            return 1

        port = ports.resolve(args.only)
        if not port:
            matches = difflib.get_close_matches(args.only, ports.names())
            if matches:
                print(f'Could not find port {args.only!r}.')
                print(f'Did you mean: {' or '.join(matches)}?')
            else:
                print(f'Could not find port {args.only!r}.')

            return 1

        if port.dependencies:
            print(f'Cannot only build {args.only!r} because it has dependencies.')
            print(f'{args.only!r} depends on: {', '.join(port.dependencies)}')

            return 1

        with chdir(port.directory):
            port.run()

        return 0

    # TODO: Track what has been built previously/in previous runs to avoid a rebuild of everything.

    to_exclude = args.exclude.split(',') if args.exclude else []
    candidates = ports.build_candidates(to_exclude)

    while candidates:
        for port in candidates:
            with chdir(port.directory):
                port.run()

        candidates = ports.build_candidates(to_exclude)

    return 1

if __name__ == '__main__':
    sys.exit(main())