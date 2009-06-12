#!/usr/bin/env python

import gtk
import pdock
item_count = 0

def add_item(btn, dock):
    global item_count
    item_count += 1
    item = pdock.DockItem(dock, "item%s" % item_count, gtk.TextView(),
                          "Item %s" % item_count, "gtk-ok")
    dock.add_item(item)

win = gtk.Window()
box = gtk.VBox()
win.add(box)

bb = gtk.HButtonBox()
box.pack_start(bb, False, False, 0)

dock = pdock.Dock()
box.pack_start(dock, True, True)


btn = gtk.Button("Add item")
btn.connect("clicked", add_item, dock)
bb.pack_start(btn, False, False, 0)
btn = gtk.Button("Dump")
btn.connect("clicked", lambda w: dock.dump())
bb.pack_start(btn, False, False, 0)
btn = gtk.Button("Quit")
btn.connect("clicked", lambda w: gtk.main_quit())
bb.pack_start(btn, False, False, 0)

win.resize(350,350)
win.connect("delete-event", lambda w, e: gtk.main_quit())

add_item(None, dock)
add_item(None, dock)
item = pdock.DockItem(dock, "itemlo", gtk.TextView(),
                      "Locked item", "gtk-edit",
                      behavior=pdock.DOCK_ITEM_BEH_LOCKED)
dock.add_item(item)
item = pdock.DockItem(dock, "itemnf", gtk.TextView(),
                      "Never floating", "gtk-edit",
                      behavior=pdock.DOCK_ITEM_BEH_NEVER_FLOATING|pdock.DOCK_ITEM_BEH_CANT_CLOSE)
dock.add_item(item)
item = pdock.DockItem(dock, "itemnv", gtk.TextView(),
                      "Never vertical", "gtk-edit",
                      behavior=pdock.DOCK_ITEM_BEH_NEVER_VERTICAL|pdock.DOCK_ITEM_BEH_CANT_AUTOHIDE)
dock.add_item(item)
item = pdock.DockItem(dock, "itemnh", gtk.TextView(),
                      "Never horizontal", "gtk-edit",
                      behavior=pdock.DOCK_ITEM_BEH_NEVER_HORIZONTAL)
dock.add_item(item)

win.show_all()

gtk.main()