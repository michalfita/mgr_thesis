#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""@package ui.MainMenu
Implementation of application's main menu.

@deprecated There is no plans to use it further!
"""

import pygtk
pygtk.require('2.0')
import gtk
import gobject
import thread
import traceback
import sys
import os.path
import gettext
from Singleton import Singleton
import MainWindow

class MainMenu(gtk.MenuBar):
    #__metaclass__ = Singleton
    def __init__(self):
        super(MainMenu, self).__init__()
        
        self.aggregate = gtk.AccelGroup()
        self.connect('parent-set', self.on_parent_set)
        
        
        self.file_menu = gtk.Menu()
        
        self.file_menuitem = gtk.MenuItem(_("_File"))
        self.file_menuitem.set_submenu(self.file_menu)
        self.append(self.file_menuitem)
        self.file_menuitem.show()
        
        self.quit_item = gtk.ImageMenuItem(gtk.STOCK_QUIT, self.aggregate)
        key, mod = gtk.accelerator_parse("Q")
        self.quit_item.add_accelerator("activate", self.aggregate, key, mod, gtk.ACCEL_VISIBLE)
        self.file_menu.append(self.quit_item)
        self.quit_item.show()
        
        self.edit_menu = gtk.Menu()
        
        self.edit_menuitem = gtk.MenuItem(_("_Edit"))
        self.edit_menuitem.set_submenu(self.edit_menu)
        self.append(self.edit_menuitem)
        self.edit_menuitem.show()
        
    def on_parent_set(self, widget, old_parent):
        parent = self
        while not isinstance(parent, gtk.Window) and parent is not None:
            parent = parent.get_parent()
        if parent is not None:
            parent.add_accel_group(self.aggregate)
            self.quit_item.connect("activate", parent.on_req_exit)
        return True