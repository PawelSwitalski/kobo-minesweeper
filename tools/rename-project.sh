#!/bin/sh
# Rebrands a fresh copy of this template into a new project: replaces the
# kobo_app / KOBO_APP / "Kobo App" placeholder tokens throughout the tree
# and renames the handful of paths that embed the app name.
#
# Usage: tools/rename-project.sh <new_name> [--title "Display Title"] [--dry-run]
#
#   <new_name>   snake_case, e.g. my_new_game (becomes CMake project/target
#                names, the C++ namespace, the executable, device paths,
#                env var prefix in SCREAMING_SNAKE form).
#   --title      Display text (README/window title/NickelMenu label/release
#                title). Defaults to a naive title-case of <new_name>
#                ("my_new_game" -> "My New Game"); pass this if that doesn't
#                read well (acronyms, etc).
#   --dry-run    List what would change; write nothing.
#
# Run this ONCE, immediately after copying the template — before you've
# written any project-specific code that might also happen to contain the
# literal text "kobo_app". Requires GNU sed (Git Bash/WSL/Linux/macOS with
# gsed aliased to sed).
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

NEW_NAME=""
TITLE=""
DRY_RUN=0

while [ $# -gt 0 ]; do
    case "$1" in
        --title) TITLE="$2"; shift 2 ;;
        --dry-run) DRY_RUN=1; shift ;;
        -h|--help)
            sed -n '2,20p' "$0"
            exit 0
            ;;
        *)
            if [ -z "$NEW_NAME" ]; then NEW_NAME="$1"; shift
            else echo "!! unexpected argument: $1" >&2; exit 1; fi
            ;;
    esac
done

if [ -z "$NEW_NAME" ]; then
    echo "usage: tools/rename-project.sh <new_name> [--title \"Display Title\"] [--dry-run]" >&2
    exit 1
fi

case "$NEW_NAME" in
    [a-z]*) ;;
    *) echo "!! <new_name> must start with a lowercase letter" >&2; exit 1 ;;
esac
case "$NEW_NAME" in
    *[!a-z0-9_]*) echo "!! <new_name> must match ^[a-z][a-z0-9_]*\$ (snake_case)" >&2; exit 1 ;;
esac
if [ "$NEW_NAME" = "kobo_app" ]; then
    echo "!! <new_name> is the placeholder itself — pick a different name" >&2
    exit 1
fi

SCREAMING=$(printf '%s' "$NEW_NAME" | tr '[:lower:]' '[:upper:]')
if [ -z "$TITLE" ]; then
    # Naive title-case fallback: my_new_game -> My New Game.
    TITLE=$(printf '%s' "$NEW_NAME" | tr '_' ' ' | sed 's/\(^\| \)\(.\)/\1\U\2/g')
fi

# Fresh-copy guard: doubles as re-run protection. After a successful rename
# the token is gone from CMakeLists.txt, so a second run aborts here instead
# of mangling an already-renamed, now project-specific tree.
if ! grep -q "kobo_app" CMakeLists.txt 2>/dev/null; then
    echo "!! 'kobo_app' not found in CMakeLists.txt — this tree has likely" >&2
    echo "   already been renamed. Re-running would corrupt an in-progress" >&2
    echo "   project. Copy a fresh template if you need another new project." >&2
    exit 1
fi

echo ">> renaming: kobo_app -> $NEW_NAME, KOBO_APP -> $SCREAMING, \"Kobo App\" -> \"$TITLE\""

if [ "$DRY_RUN" = "1" ]; then
    FILES=$(find . -type f \
        -not -path './.git/*' \
        -not -path './build*/*' \
        -not -path './third_party/FBInk/*' \
        -not -path './tools/rename-project.sh' \
        -not -name '*.png' -not -name '*.ttf' -not -name '*.a' -not -name '*.zip' -not -name '*.ico')
    echo ">> --dry-run: files that would be content-edited:"
    printf '%s\n' "$FILES" | xargs grep -lF -e "kobo_app" -e "KOBO_APP" -e "Kobo App" 2>/dev/null || true
    echo ">> --dry-run: paths that would be renamed:"
    echo "   dist/.adds/kobo_app/          -> dist/.adds/$NEW_NAME/"
    echo "   dist/.adds/nm/kobo_app        -> dist/.adds/nm/$NEW_NAME"
    echo "   dist/kfmon/config/kobo_app.ini -> dist/kfmon/config/$NEW_NAME.ini"
    echo "   dist/kfmon-kobo_app.png       -> dist/kfmon-$NEW_NAME.png"
    echo ">> --dry-run: no changes written"
    exit 0
fi

mv_tracked() {
    src="$1"; dst="$2"
    [ -e "$src" ] || return 0
    if git rev-parse --is-inside-work-tree >/dev/null 2>&1 && git ls-files --error-unmatch "$src" >/dev/null 2>&1; then
        git mv "$src" "$dst"
    else
        mv "$src" "$dst"
    fi
}

# Path renames FIRST, so the content-edit pass below (which lists files
# fresh, after these moves) finds the renamed files under their new paths
# instead of skipping them as missing.
mv_tracked "dist/.adds/kobo_app" "dist/.adds/$NEW_NAME"
mv_tracked "dist/.adds/nm/kobo_app" "dist/.adds/nm/$NEW_NAME"
mv_tracked "dist/kfmon/config/kobo_app.ini" "dist/kfmon/config/$NEW_NAME.ini"
mv_tracked "dist/kfmon-kobo_app.png" "dist/kfmon-$NEW_NAME.png"

# Files to content-edit: everything tracked-by-intent, excluding VCS/build
# output, the gitignored FBInk build tree, and binary assets sed can't touch.
# Captured AFTER the path renames above so moved files are found at their
# new location, not skipped as missing.
#
# This script itself is deliberately EXCLUDED: it contains the literal guard
# token "kobo_app" (see the fresh-copy guard above). If this pass rewrote it
# too, the guard would silently re-target itself to whatever name was just
# applied, and re-running the script later would never be blocked — quietly
# defeating the re-run protection instead of enforcing it.
FILES=$(find . -type f \
    -not -path './.git/*' \
    -not -path './build*/*' \
    -not -path './third_party/FBInk/*' \
    -not -path './tools/rename-project.sh' \
    -not -name '*.png' -not -name '*.ttf' -not -name '*.a' -not -name '*.zip' -not -name '*.ico')

CHANGED=0
for f in $FILES; do
    [ -f "$f" ] || continue
    if grep -qF -e "kobo_app" -e "KOBO_APP" -e "Kobo App" "$f" 2>/dev/null; then
        sed -i \
            -e "s/kobo_app/$NEW_NAME/g" \
            -e "s/KOBO_APP/$SCREAMING/g" \
            -e "s/Kobo App/$TITLE/g" \
            "$f"
        CHANGED=$((CHANGED + 1))
    fi
done

echo ">> done: $CHANGED file(s) edited, 4 path(s) renamed"
echo ""
echo "Next steps:"
echo "  cmake -B build/sim -D${SCREAMING}_BACKEND=sdl && cmake --build build/sim --config Release"
echo "  git init && git add -A && git commit -m \"Initial commit from kobo-games-template\""
echo "  review LICENSE (copyright holder is unchanged by this script)"
echo "  edit .specify/memory/constitution.md, then run /speckit-constitution or /speckit-specify"
