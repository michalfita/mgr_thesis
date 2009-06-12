#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""@package ui.MainWindow
Implementation of main window.

This module puts all elements of the user interface in main window's container
and displays it. It handles some events releated with the window itself.
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
from misc.Singleton import Singleton
from AboutDialog import AboutDialog
from ActionDispatcher import ActionDispatcher
from CommandEntry import CommandEntry
from CommandOutput import CommandOutput
from ControlButtons import ControlButtons
from CommunicationUserInterface import CommunicationUserInterface

class MainWindow(Singleton, gtk.Window):
    """Main window implementation.
    
    This class contains implementation of base elements creation in the main
    application window. Menu and toolbar are taken from UI manager, which loads
    structure from \p mainui.xml file.
     
    @note This class inherits from Singleton to implement the singleton pattern.
    """
    # FIXME: How to make gtk singleton?
    def __init__(self):
        """Object constructor.
        
        Here main user interface elements comes into life. It divides the window
        and put all the widgets in appriopriate places.
        
        @note Dual inheritance requires to not use of super().
        @todo Add proper docking mechanisms in future.
        """
        Singleton.__init__(self, type=gtk.WINDOW_TOPLEVEL)
        gtk.Window.__init__(self, type=gtk.WINDOW_TOPLEVEL)
        
        self.set_title('Robotiq')
        self.prepare_icons()
        #gobject.type_register(MainWindow)
        self.uimanager = gtk.UIManager()
        
        self.actiongroup = gtk.ActionGroup('MainActionGroup')
        self._prepare_actions(self.actiongroup)
        
        self.uimanager.insert_action_group(self.actiongroup, 0)
        self.uimanager.add_ui_from_file('ui/mainui.xml')
        
        accelgroup = self.uimanager.get_accel_group()
        self.add_accel_group(accelgroup)
        
        self.main_menu = self.uimanager.get_widget('/MenuBar')
        #self.main_menu.show()
        
        self.toolbar = self.uimanager.get_widget('/Toolbar')
        #self.toolbar.show()
        
        # TODO: Temporary elements in window
        self.command_output = CommandOutput(None) # FIXME: provide access to data by dispatcher insetead of object
        #self.command_output.show()
        self.set_geometry_hints(self.command_output, 150, 200, -1, -1, 150, 200, 1, 1, 0.75, 10)
        self.frame_cmd_output = gtk.Frame()
        self.frame_cmd_output.set_shadow_type(gtk.SHADOW_IN)
        #self.frame_cmd_output.show()
        self.frame_cmd_output.add(self.command_output)
        
        self.command_entry = CommandEntry(None, self.command_output) # FIXME: provide access to console by dispatcher instead of object
        #self.command_entry.show()
        
        # TODO: going to be dockable
        self.right_vpaned = gtk.VPaned()
        dupa1 = gtk.Label(_('Placeholder'))
        self.control_buttons = ControlButtons()
        #self.control_buttons.show()
        self.frame_ctrl_buttons = gtk.Frame()
        #self.frame_ctrl_buttons.show()
        self.frame_ctrl_buttons.add(self.control_buttons)
        
        dupa1.show()
        
        self.right_vpaned.pack1(dupa1)
        self.right_vpaned.pack2(self.frame_ctrl_buttons)
        self.right_vpaned.show()
        
        self.top_hpaned = gtk.HPaned()
        self.top_hpaned.pack1(self.frame_cmd_output, True, False)
        self.top_hpaned.pack2(self.right_vpaned, True)
        #self.top_hpaned.show()
        
        self.statusbar = gtk.Statusbar()
        #self.statusbar.show()
        
        self.vbox = gtk.VBox(False, 2)
        self.add(self.vbox)
        self.vbox.pack_start(self.main_menu, False, False, 0)
        self.vbox.pack_start(self.toolbar, False, False, 0)
        self.vbox.pack_end(self.statusbar, False, False, 0)
        self.vbox.pack_end(self.command_entry, False, False, 0)
        self.vbox.pack_end(self.top_hpaned, True, True, 0)
        self.vbox.show()
        
        self._prepare_keys_table()
        self._prepare_dispatchions()
        self._prepare_signals()

        self._comm_ui = CommunicationUserInterface()
        return
    
    def show(self):
        gtk.Window.show(self)
        self.main_menu.show()
        self.toolbar.show()
        self.command_output.show()
        self.command_entry.show()
        self.frame_cmd_output.show()
        self.control_buttons.show()
        self.frame_ctrl_buttons.show()
        self.top_hpaned.show()
        self.statusbar.show()
    
    def update_bars(self):
        self.vbox.remove(self.main_menu)
        self.vbox.remove(self.toolbar)
        self.main_menu = self.uimanager.get_widget('/MenuBar')
        self.toolbar = self.uimanager.get_widget('/Toolbar')
        self.vbox.pack_start(self.main_menu, False, False, 0)
        self.vbox.pack_start(self.toolbar, False, False, 0)
        self.main_menu.show()
        self.toolbar.show() 
              
    def prepare_icons(self):
        """Preparation of application icons.
        """
        icon_names = ['robot2-16x16', 'robot2-32x32', 'robot2-48x48', 'robot2-128x128']
        icon_list = []
        for icon_name in icon_names:
            icon_list.append(gtk.gdk.pixbuf_new_from_file(os.path.abspath('./icons/' + icon_name + '.png')))
        icons_string = ''
        for i in range(0, len(icon_list)):
            icons_string += 'icon_list[%d], ' % i
        exec 'gtk.window_set_default_icon_list(%s)' % icons_string
    
    def _prepare_signals(self):
        action_dispatcher = ActionDispatcher()
        self.connect("destroy", action_dispatcher['application-quit'])
        self.connect("delete-event", self.h_delete_event)
        self.connect("key-press-event", self.h_keypress_event)
        self.connect("key-release-event", self.h_keyrelease_event)
    
    def _prepare_keys_table(self):
        dispatcher = ActionDispatcher()
        self._keys_table = {
          gtk.accelerator_parse('<Control>Left') : dispatcher['control-left-press'],
          gtk.accelerator_parse('<Control><Release>Left') : dispatcher['control-left-release'],
          gtk.accelerator_parse('<Control>Right') : dispatcher['control-right-press'],
          gtk.accelerator_parse('<Control><Release>Right') : dispatcher['control-right-release'],
          gtk.accelerator_parse('<Control>Up') : dispatcher['control-up-press'],
          gtk.accelerator_parse('<Control><Release>Up') : dispatcher['control-up-release'],
          gtk.accelerator_parse('<Control>Down') : dispatcher['control-down-press'],
          gtk.accelerator_parse('<Control><Release>Down') : dispatcher['control-down-release'],
          gtk.accelerator_parse('<Control>space') : dispatcher['control-stop-press'],
          gtk.accelerator_parse('<Control><Release>space') : dispatcher['control-stop-release'],
        }
        self._pressed_keys = set()
    
    def _prepare_dispatchions(self):
        action_dispatcher = ActionDispatcher()
        action_dispatcher['application_about'] += self.on_req_about
    
    def _prepare_actions(self, actiongroup):
        action_dispatcher = ActionDispatcher()
        actiongroup.add_actions([
            ('file-menu', None, _('_File')),
            ('edit-menu', None, _('_Edit')),
            ('connection-menu', None, _('_Connection')),
            ('help-menu', None, _('_Help')),
            ('clipboard-cut', gtk.STOCK_CUT, _('Cu_t'), '<control>x', _('Put selected content into the clipboard and remove from context.')),
            ('clipboard-copy', gtk.STOCK_COPY, _('_Copy'), '<control>c', _('Copy seleted content into the clipboard.')),
            ('clipboard-paste', gtk.STOCK_PASTE, _('_Paste'), '<control>v', _('Paste the content of clipboard in active context.')),
            ('selection-all', gtk.STOCK_SELECT_ALL, _('Select _All'), '<control>a', _('Select all in active context.')),
            ('application-about', gtk.STOCK_ABOUT, _('_About'), None, _('Displays information about application and its license.'), self.on_req_about),
            ('application-quit', gtk.STOCK_QUIT, _('_Quit'), None, _('Leave the application.'), action_dispatcher['application-quit']),
          ])
        return    
        
    def on_req_about(self, param):
        """Event handler for about request.
        """
        about_dialog = AboutDialog()
        about_dialog.run()
    
    def h_keypress_event(self, widget, event):
        #print "#KEY PRESS   (%s) %s" % (event.state, gtk.gdk.keyval_name(event.keyval))
        try:
            if (event.keyval, event.state) not in self._pressed_keys:
                self._pressed_keys.add((event.keyval, event.state)) 
                self._keys_table[(event.keyval, event.state)]()
            return True
        except KeyError:
            return False
    
    def h_keyrelease_event(self, widget, event):
        #print "#KEY RELEASE (%s) %s" % (event.state, gtk.gdk.keyval_name(event.keyval))
        try:
            if (event.keyval, event.state) in self._pressed_keys:
                self._pressed_keys.discard((event.keyval, event.state))
            self._keys_table[(event.keyval, event.state | gtk.gdk.RELEASE_MASK)]()
            return True
        except KeyError:
            return False  
        
    def h_delete_event(self, widget, event, data=None):
        """Event handler for window delete.
        """
        (width, height) = self.get_size()
        (x, y) = self.get_position()
        #p = PersistentState.get_instance()
        #p.put('Window', 'width', width)
        #p.put('Window', 'height', height)
        #p.put('Window', 'x', x)
        #p.put('Window', 'y', y)
        return False
