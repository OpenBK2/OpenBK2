"""Open an object from the Game Database window, the way double-clicking it does.

    python gdbopen.py BuildingRPGStats            # the first leaf of that table's tree
    python gdbopen.py Model --tree 400            # if the tree has to be named by id

The Game Database window has a combo box of tables and a tree per table.
Choosing the table and double-clicking a leaf opens the object in its editor --
a building in the building editor, and so on -- which is how the state-driven
palettes of editors other than the map editor are reached.

Neither control is found by id any more. The MFC frame gave them 135 and 400;
the wx frame assigns its own ids (-31961 and -31957 in one run, and not stable
between runs either), so this looks for what they contain instead: the table
combo is the visible ComboBox whose items include the table asked for, which
is self-checking, and the tree is the visible tree below it in the same pane.

The table has to be one the editor shows: tick it in File -> Select Tables
(1047) first. That dialog's list is a wxCheckListBox, whose items toggle with
Space and not with LB_SETSEL -- LB_SETSEL is a silent no-op there and
LB_GETSELCOUNT answers -1, because it is a single-select listbox with the
check drawn by the owner.

A clean exit saves the table choice into UserData.xml, so bracket the run with
editorstate.py.
"""
import argparse
import sys
import time

import uiprobe as P


def find_table_combo(table):
    """The visible ComboBox that offers this table, and what the others offer."""
    seen = {}
    for combo in P.find(class_is='ComboBox', visible=True):
        try:
            items = P.combo_items(combo)
        except Exception:
            continue
        if not items:
            continue
        seen[combo] = items
        if table in items:
            return combo, seen
    return None, seen


def find_tree_below(combo, tree_id=None):
    """The visible tree in the combo's pane: same column, just below it."""
    trees = P.find(class_is='SysTreeView32', visible=True)
    if tree_id is not None:
        trees = [t for t in trees if P.find(class_is='SysTreeView32', ctrl_id=tree_id, visible=True)]
    if not trees:
        return None
    cr = P.rect(combo)
    # Horizontally overlapping and starting below the combo: that is the pane
    # it belongs to. Nearest one wins, so a second database pane does not.
    def score(t):
        r = P.rect(t)
        overlaps = min(r.right, cr.right) - max(r.left, cr.left)
        return (r.top >= cr.top and overlaps > 0, -(r.top - cr.top))
    below = [t for t in trees if score(t)[0]]
    if not below:
        return trees[0]
    return sorted(below, key=lambda t: P.rect(t).top - cr.top)[0]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('table')
    ap.add_argument('--tree', type=int, default=None,
                    help='the tree control id, if it has to be said explicitly')
    ap.add_argument('--desktop', default='bk2probe')
    ap.add_argument('--timeout', type=int, default=60)
    a = ap.parse_args()
    P.attach(a.desktop)

    combo, seen = find_table_combo(a.table)
    if combo is None:
        if not seen:
            raise SystemExit('no Game Database window')
        # Several combos are visible -- the object filter and the player one
        # among them -- so report them apart rather than as one heap.
        print('%r is in none of the visible combos:' % a.table, file=sys.stderr)
        for h, items in seen.items():
            head = ', '.join(items[:8])
            print('  %s  %d items: %s%s'
                  % (hex(h), len(items), head, ' ...' if len(items) > 8 else ''),
                  file=sys.stderr)
        raise SystemExit('tick %s in File -> Select Tables (1047) first' % a.table)
    print('table combo %s' % hex(combo))
    P.choose(combo, a.table)
    time.sleep(1)

    tree = find_tree_below(combo, a.tree)
    if tree is None:
        raise SystemExit('no visible tree for that pane')
    print('tree %s' % hex(tree))
    P.tree_select_first_leaf(tree)
    time.sleep(0.3)
    name = P.open_tree_caret(tree)
    print('double-clicked %r' % name)

    frame = P.find_top('Blitzkrieg 2 Editor', exact=False)
    for _ in range(a.timeout * 2):
        if a.table.lower() in P.text(frame).lower():
            print('open: %s' % P.text(frame))
            return 0
        time.sleep(0.5)
    raise SystemExit('not open after %ds; title %r' % (a.timeout, P.text(frame)))


if __name__ == '__main__':
    sys.exit(main())
