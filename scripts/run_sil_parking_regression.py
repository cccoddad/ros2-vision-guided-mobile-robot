#!/usr/bin/env python3
"""Run the normal-parking and Tag-loss SIL regressions against an active simulation."""

from pathlib import Path
import subprocess
import sys


def run_script(script_path: Path) -> int:
    command = [sys.executable, str(script_path)]
    print(f'Running: {" ".join(command)}', flush=True)
    completed = subprocess.run(command, check=False)
    if completed.returncode != 0:
        print(
            f'FAIL: {script_path.name} exited with status {completed.returncode}.',
            file=sys.stderr,
        )
    return completed.returncode


def main() -> int:
    scripts_directory = Path(__file__).resolve().parent
    for script_name in ('run_simulated_parking.py', 'verify_tag_loss_stop.py'):
        exit_code = run_script(scripts_directory / script_name)
        if exit_code != 0:
            return exit_code

    print('PASS: SIL parking regression completed normal parking and Tag-loss stop checks.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
