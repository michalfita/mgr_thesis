#!/usr/bin/env python
# -*- coding: utf-8 -*-

import serial
import threading
import gettext
import gtk
from ActionDispatcher import ActionDispatcher
from misc.Singleton import Singleton

"""@package ui.CommunicationUserInterface

This module contains the class responsible for common GUI elements releated
to connection in the application.
"""

class CommunicationUserInterface(Singleton):
    """
    The CommunicationInterface class for UI manipulation.
    
    This class connects the GUI with the Communication Manager responsible for
    different communication channels.
    """
    def __init__(self):
        self.__comm_mgr = None
        self.__feed_uimanager()
        action_dispatcher = ActionDispatcher()
        action_dispatcher['connect'] += self.connect
        action_dispatcher['disconnect'] += self.disconnect
        
    def __feed_uimanager(self):
        from ui.MainWindow import MainWindow
        main_window = MainWindow()
        self.__uimanager = main_window.uimanager
        self.__main_actiongroup = gtk.ActionGroup('CommUserIfaceActionGroup')
        self.__chan_actiongroup = gtk.ActionGroup('CommChannelsActionGroup')
        action_dispatcher = ActionDispatcher()
        self.__main_actiongroup.add_actions([
                ('connect', gtk.STOCK_CONNECT, _('_Connect'), 'F5', _('Connect to the robot.'), action_dispatcher['connect']),
                ('disconnect', gtk.STOCK_DISCONNECT, _('_Disconnect'), None, _('Disconnect the connection.'), action_dispatcher['disconnect']),
                ('connection-setup', gtk.STOCK_PROPERTIES, _('Confi_guration'), None, _('Configure all communication channels.'), action_dispatcher['connection-setup'])
            ])
        
        self.__uimanager.insert_action_group(self.__main_actiongroup, 1)
        self.__uimanager.insert_action_group(self.__chan_actiongroup, 2)
        self.__uimanager.ensure_update()
        self.__main_actiongroup.get_action('connect').set_sensitive(True)
        self.__main_actiongroup.get_action('disconnect').set_sensitive(False)
        #main_window.update_bars()
    
    def __add_channels(self):
        # Roll through all channels...
        name = 'SerialCommunication'
        action = 'serial-select'
        actions_list = [
                (action, None, _('_Serial Communication'), None, _('Use serial communication to interact with robot.'), 0)
            ]
        items_string = '<menuitem name="%s" action="%s" />\n' % (name, action)
        #self.__chan_actiongroup.add_toggle_actions(actions_list)
        self.__chan_actiongroup.add_radio_actions(actions_list)
        
        # Feed uimanager with additional menu items.
        id = self.__uimanager.add_ui_from_string('''
        <ui>
          <menubar name="MenuBar">
            <menu name="ConnectionMenu" action="connection-menu">
              <placeholder name="ConnectionPlaceholder">
                <menuitem name="SerialCommunication" action="serial-select" />
                %s
              </placeholder>
            </menu>
          </menubar>
        </ui>
        ''' % items_string)
        return
    
    def __update_uimanager(self):
        """
        This private method updates the uimanager and controls.
        @todo GUI has to be handled in higher level class with commonized
              interface for adding protocols similarly as plugins.
        """
        from ui.MainWindow import MainWindow
        main_window = MainWindow()
        action_dispatcher = ActionDispatcher()
        if self.__comm_mgr is not None:
          if self.__comm_mgr.state == 'connected':
              self.__main_actiongroup.get_action('connect').set_sensitive(False)
              self.__main_actiongroup.get_action('disconnect').set_sensitive(True)
              self.__chan_actiongroup.set_sensitive(False)
          else:
              action = self.__actiongroup.get_action('disconnect')
              self.__main_actiongroup.get_action('connect').set_sensitive(True)
              self.__main_actiongroup.get_action('disconnect').set_sensitive(False)
              self.__chan_actiongroup.set_sensitive(True)
          self.__uimanager.ensure_update()
        return 
    
    def select(self):
        pass
    
    def connect(self, p = None):
        self.__update_uimanager()
        
    def disconnect(self, p = None):
        self.__update_uimanager()
        
