#!/usr/bin/env python

import gtk
import gtk.glade
import gobject

from os.path import abspath, join, dirname

import pdock

class TestFactory(object):
    
    def __init__(self):
        self.item_count = 0
        self.xml = gtk.glade.XML(abspath(join(dirname(__file__), "test_factory.glade")))
        self.win = self.xml.get_widget("window1")
        self.xml.signal_autoconnect(self)
        self._setup_dock()
        
    def _setup_dock(self):
        box = self.xml.get_widget("vbox1")
        self.dock = pdock.Dock()
        box.pack_start(self.dock, True, True)
        self.xml.get_widget("combo_pos").set_active(0)
        
    def on_quit(self, *args):
        gtk.main_quit()
        
    def on_add_item(self, *args):
        self.item_count += 1
        label = self.xml.get_widget("entry_label").get_text() or "Unknown"
        p = self.xml.get_widget("combo_pos").get_active_text()
        pos = dict(top=gtk.POS_TOP, bottom=gtk.POS_BOTTOM,
                   left=gtk.POS_LEFT, right=gtk.POS_RIGHT).get(p.lower(), None)
        beh = pdock.DOCK_ITEM_BEH_NORMAL
        if self.xml.get_widget("chk_nfloating").get_active():
            beh = beh | pdock.DOCK_ITEM_BEH_NEVER_FLOATING
        item = pdock.DockItem(self.dock, "item%s" % self.item_count,
                              gtk.TextView(),
                              label,
                              "gtk-ok",
                              pos,
                              beh
                              )
        self.dock.add_item(item)
        
    def run(self):
        self.win.show_all()
        gtk.main()
        
        
if __name__ == "__main__":
    f = TestFactory()
    f.run()