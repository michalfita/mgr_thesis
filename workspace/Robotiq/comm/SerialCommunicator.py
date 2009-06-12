#!/usr/bin/env python
# -*- coding: utf-8 -*-

import serial
import threading
import gettext
import gtk
from ui.MainWindow import MainWindow
from ui.ActionDispatcher import ActionDispatcher

"""@package comm.SerialCommunicator
This module contains class to communicate through serial port.
"""

class SerialCommunicator(object):
    """
    This class provides facilitation for serial communication.
    
    The role of this class is to provide full facilitation for the serial
    communication in the application. It registers actions releated to making
    connection and disconnection, provides fields for configuration manager.
    The main operation is done in the thread working when application is in
    connection state.
    """
    def __init__(self):
        self.__state = 'initial'
        self.__serial = serial.Serial()
        self.__serial.timeout = 2 # particular value :-)
        self.__feed_uimanager()
        action_dispatcher = ActionDispatcher()
        action_dispatcher['connect'] += self.connect
        action_dispatcher['disconnect'] += self.disconnect
    
    def __feed_uimanager(self):
        main_window = MainWindow()
        self.__uimanager = main_window.uimanager
        self.__actiongroup = gtk.ActionGroup('SerialCommActionGroup')
        action_dispatcher = ActionDispatcher()
        self.__actiongroup.add_toggle_actions([
                ('serial-select', None, _('_Serial Communication'), None, _('Use serial communication to interact with robot.'), action_dispatcher['serial-select'], True)
            ])
        self.__actiongroup.add_actions([
                ('connect', gtk.STOCK_CONNECT, _('_Connect'), 'F5', _('Connect to the robot.'), action_dispatcher['connect']),
                ('disconnect', gtk.STOCK_DISCONNECT, _('_Disconnect'), None, _('Disconnect the connection.'), action_dispatcher['disconnect']),
                ('connection-setup', gtk.STOCK_PROPERTIES, _('Confi_guration'), None, _('Configure all communication channels.'), action_dispatcher['connection-setup'])
            ])
        
        #id = self.__uimanager.new_merge_id()
        id = self.__uimanager.add_ui_from_string('''
        <ui>
          <menubar name="MenuBar">
            <menu name="ConnectionMenu" action="connection-menu">
              <placeholder name="ConnectionPlaceholder">
                <menuitem name="SerialCommunication" action="serial-select" />
              </placeholder>
            </menu>
          </menubar>
        </ui>
        ''')
        print '#ID=',id
        #self.__uimanager.add_ui(id,
        #    '/menubar/ConnectionMenu/ConnectionPlaceholder',
        #    'SerialCommunication',
        #    'serial-select', gtk.UI_MANAGER_MENUITEM, True
        #    )
        self.__uimanager.insert_action_group(self.__actiongroup, 1)
        self.__uimanager.ensure_update()
        self.__actiongroup.get_action('connect').set_sensitive(True)
        self.__actiongroup.get_action('disconnect').set_sensitive(False)
        #main_window.update_bars()
    
    def __update_uimanager(self):
        """
        This private method updates the uimanager and controls.
        @todo GUI has to be handled in higher level class with commonized
              interface for adding protocols similarly as plugins.
        """
        main_window = MainWindow()
        action_dispatcher = ActionDispatcher()
        if self.__state == 'connected':
            self.__actiongroup.get_action('connect').set_sensitive(False)
            self.__actiongroup.get_action('disconnect').set_sensitive(True)
            self.__actiongroup.get_action('serial-select').set_sensitive(False)
            #if action is not None:
            #  self.__actiongroup.remove_action(action)
            #self.__actiongroup.add_actions([
            #        ('disconnect', gtk.STOCK_DISCONNECT, _('_Disconnect'), None, _('_Disconnect the connection.'), action_dispatcher['disconnect']),
            #    ])
        else:
            action = self.__actiongroup.get_action('disconnect')
            self.__actiongroup.get_action('connect').set_sensitive(True)
            self.__actiongroup.get_action('disconnect').set_sensitive(False)
            self.__actiongroup.get_action('serial-select').set_sensitive(True)
            #if action is not None:
            #  self.__actiongroup.remove_action(action)
            #self.__actiongroup.add_actions([
            #        ('connect', gtk.STOCK_CONNECT, _('_Connect'), 'F5', _('Connect to the robot.'), action_dispatcher['connect']),
            #    ])
        self.__uimanager.ensure_update()
        #main_window.update_bars() 
    
    def setup(self, parameters):
        """
        This method sets the serial objects up.
      
        This method sets the parameters of the serial object when no open
        connection has been made.
        @todo Consider exception in case of calling in wrong place
        """
        if self.__state != 'connected':
            if isinstance(parameters, dict):
                for n,p in parameters.items():
                    if isinstance(p, dict):
                      p = p['value']
                    setattr(self.__serial, n, p)
            self.__state != 'disconnected'
        else:
            pass # consider exception
            
    
    def get_parameters(self):
        parameters = {}
        parameters['port'] = {
            'position' : 1,
            'name' : _('Serial Port'),
            'options' : None,
          }
        parameters['9600'] = {
            'position' : 2,
            'name' : _('Baud Rate'),
            'options' : self.__serial.BAUDRATES
          }
        parameters['EIGHTBITS'] = {
            'position' : 3,
            'name' : _('Number of data bits'),
            'options' : self.__serial.BYTESIZES
          }
        
        parameters['parity'] = {
            'position' : 4,
            'name' : _('Parity check mode'),
            'options' : self.__serial.PARITIES
          }
        parameters['stopbits'] = {
            'position' : 5,
            'name' : _('Stop bits'),
            'options' : self.__serial.STOPBITS
          }
        
        parameters['xonxoff'] = {
            'position' : 6,
            'name' : _('XON/XOFF flow control'),
            'logic' : [0 , 1]
          }
        parameters['rtscts'] = {
            'position' : 7,
            'name' : _('RTS/CTS flow control'),
            'logic' : [0, 1]
          }
        return parameters
    
    def connect(self, p = None):
        """
        This method makes a connection.
        """
        if self.__state != 'connected':
            self.__state = 'connected'

        self.__update_uimanager()
    
    def disconnect(self, p = None):
        """
        This method disconnects.
        """
        if self.__state != 'disconnected':
            self.__state = 'disconnected'

        self.__update_uimanager()
    
    def __working_thread(self):
        pass    