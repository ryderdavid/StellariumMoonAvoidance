import os
import sys
from pathlib import Path

try:
    import yaml
except ImportError:
    print("PyYAML is required to resolve Stellarium compatibility mapping.", file=sys.stderr)
    sys.exit(1)


COMPAT_PATH = Path("ci/stellarium-compat.yml")


def main() -> int:
    if not COMPAT_PATH.is_file():
        print(f"Compatibility file '{COMPAT_PATH}' not found.", file=sys.stderr)
        return 1

    try:
        config = yaml.safe_load(COMPAT_PATH.read_text(encoding="utf-8")) or {}
    except Exception as exc:  # noqa: BLE001
        print(f"Failed to parse YAML from '{COMPAT_PATH}': {exc}", file=sys.stderr)
        return 1

    requested = os.getenv("REQUESTED_TARGET", "").strip()
    target_key = requested or config.get("default")

    if not target_key:
        print(
            "No default target defined in compatibility file and no target requested.",
            file=sys.stderr,
        )
        return 1

    targets = config.get("targets") or {}
    if target_key not in targets:
        print(
            f"Requested Stellarium target '{target_key}' not found in {COMPAT_PATH}.",
            file=sys.stderr,
        )
        print(
            "Available targets: " + ", ".join(sorted(targets.keys())),
            file=sys.stderr,
        )
        return 1

    target = targets[target_key] or {}
    qt = target.get("qt") or {}
    macos = target.get("macos") or {}
    windows = target.get("windows") or {}
    msvc = windows.get("msvc") or {}

    github_env = os.getenv("GITHUB_ENV")
    if not github_env:
        print("GITHUB_ENV is not set; cannot export environment variables.", file=sys.stderr)
        return 1

    def write_env(key: str, value: object) -> None:
        with open(github_env, "a", encoding="utf-8") as env_file:
            env_file.write(f"{key}={value}\n")

    write_env("STELLARIUM_TARGET", target_key)
    write_env("STELLARIUM_VERSION", target.get("stellarium_version", ""))
    write_env("QT_MAJOR", qt.get("major", ""))
    write_env("QT_VERSION", qt.get("version", ""))
    
    # macOS specific
    write_env("MACOS_RUNNER", macos.get("runner", ""))
    write_env("MACOS_ARCH", macos.get("arch", ""))
    write_env("MACOS_DEPLOYMENT_TARGET", macos.get("deployment_target", ""))
    
    # Windows specific
    write_env("WINDOWS_ARCH", windows.get("arch", ""))
    write_env("MSVC_YEAR", msvc.get("year", ""))
    write_env("MSVC_TOOLSET", msvc.get("toolset", ""))

    print(
        "Using Stellarium target: "
        f"{target_key} (Stellarium {target.get('stellarium_version')}, "
        f"Qt {qt.get('version')})"
    )

    return 0


if __name__ == "__main__":  # pragma: no cover
    raise SystemExit(main())
