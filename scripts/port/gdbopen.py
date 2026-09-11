"""Open an object from the Game Database window, the way double-clicking it does.

    python gdbopen.py BuildingRPGStats            # the first leaf of that table's tree
    python gdbopen.py BuildingRPGStats --tree 400

The Game Database window has a combo box of tables (control id 135) and a tree
per table. Choosing the table and double-clicking a leaf opens the object in
its editor -- a building in the building editor, and so on -- which is how the
state-driven palettes of editors other than the map editor are reached.

The table has to be one the editor shows: tick it in File -> Select Tables
(1047) first. A clean exit then saves that choice into UserData.xml, so
bracket the run with editorstate.py.
"""
import argparse
import sys
import time

import uiprobe as P

TABLE_COMBO_ID = 135


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('table')
    ap.add_argument('--tree', type=int, default=400, help='the tree control id that shows the table')
    ap.add_argument('--desktop', default='bk2probe')
    ap.add_argument('--timeout', type=int, default=60)
    a = ap.parse_args()
    P.attach(a.desktop)
    combos = P.find(class_is='ComboBox', ctrl_id=TABLE_COMBO_ID)
    if not combos:
        raise SystemExit('no Game Database window')
    items = P.combo_items(combos[0])
    if a.table not in items:
        raise SystemExit('%s is not among the shown tables %s; tick it in Select Tables (1047)' % (a.table, items))
    P.choose(combos[0], a.table)
    time.sleep(1)
    trees = P.find(class_is='SysTreeView32', ctrl_id=a.tree, visible=True)
    if not trees:
        raise SystemExit('no visible tree with id %d' % a.tree)
    P.tree_select_first_leaf(trees[0])
    time.sleep(0.3)
    name = P.open_tree_caret(trees[0])
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
