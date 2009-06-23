#!/usr/bin/env python
# -*- coding: utf-8 -*-

import serial
import threading
import gettext
import gtk
from comm.CommunicationManager import MetaCommunicator
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
    __metaclass__ = MetaCommunicator
    
    def __init__(self):
        super(SerialCommunicator, self).__init__()
        self.__state = 'initial'
        self.__serial = serial.Serial()
        self.__serial.timeout = 2 # particular value :-)
        action_dispatcher = ActionDispatcher()
        action_dispatcher['connect'] += self.connect
        action_dispatcher['disconnect'] += self.disconnect
        print self.__class__.__name__, "initialized..."
    
    @property
    def menu_text(self):
        return _('_Serial Communication')
    
    @property 
    def menu_tooltip(self):
        return _('Use serial communication to interact with robot.')
    
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
            
    @property
    def parameters(self):
        parameters = {}
        parameters['port'] = {
            'position' : 1,
            'name' : _('Serial Port'),
            'options' : None,
          }
        parameters['baudrate'] = {
            'position' : 2,
            'name' : _('Baud Rate'),
            'options' : self.__serial.BAUDRATES
          }
        parameters['bytesize'] = {
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
            self.__serial.open()
            self.__state = 'connected'
    
    def disconnect(self, p = None):
        """
        This method disconnects.
        """
        if self.__state != 'disconnected':
            self.__serial.close()
            self.__state = 'disconnected'
    
    def __working_thread(self):
        pass    