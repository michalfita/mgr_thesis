#!/usr/bin/env python
# -*- coding: utf-8 -*-

import serial
import threading
import gettext
import gtk
import gobject
import glib
import time
from comm.CommunicationManager import MetaCommunicator
from ui.MainWindow import MainWindow
from ui.ActionDispatcher import ActionDispatcher
from misc.PersistentContainer import PersistentContainer

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
        self.__serial.timeout = 0 # particular value :-)
        self.__available_ports = self.__scan()
        self.__thread_enabled = False
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
            persistent = PersistentContainer()
            if isinstance(parameters, dict):
                for n,p in parameters.items():
                    if isinstance(p, dict):
                      p = p['value']
                    print "serial param '", n, "' = ", str(p)
                    setattr(self.__serial, n, p)
                    setattr(persistent.serialcommunicator, n, p)
            self.__state != 'disconnected'
        else:
            pass # consider exception
            
    @property
    def parameters(self):
        parameters = {}
        persistent = PersistentContainer()
        parameters['port'] = {
            'position' : 1,
            'name' : _('Serial Port'),
            'options' : [x[1] for x in self.__available_ports],
            'value' : persistent.serialcommunicator.port or self.__serial.port
          }
        parameters['baudrate'] = {
            'position' : 2,
            'name' : _('Baud Rate'),
            'options' : self.__serial.BAUDRATES,
            'value' : int(persistent.serialcommunicator.baudrate or self.__serial.baudrate)
          }
        parameters['bytesize'] = {
            'position' : 3,
            'name' : _('Number of data bits'),
            'options' : self.__serial.BYTESIZES,
            'value' : int(persistent.serialcommunicator.bytesize or self.__serial.bytesize)
          }
        
        parameters['parity'] = {
            'position' : 4,
            'name' : _('Parity check mode'),
            'options' : self.__serial.PARITIES,
            'value' : persistent.serialcommunicator.parity or self.__serial.parity
          }
        parameters['stopbits'] = {
            'position' : 5,
            'name' : _('Stop bits'),
            'options' : self.__serial.STOPBITS,
            'value' : int(persistent.serialcommunicator.stopbits or self.__serial.stopbits)
          }
        
        parameters['xonxoff'] = {
            'position' : 6,
            'name' : _('XON/XOFF flow control'),
            'logic' : [0 , 1],
            'value' : int(persistent.serialcommunicator.xonxoff or self.__serial.xonxoff)
          }
        parameters['rtscts'] = {
            'position' : 7,
            'name' : _('RTS/CTS flow control'),
            'logic' : [0, 1],
            'value' : int(persistent.serialcommunicator.rtscts or self.__serial.rtscts)
          }
        return parameters
    
    def connect(self, p = None):
        """
        This method makes a connection.
        """
        if self.__state != 'connected':
            self.__serial.open()
            self.__thread_start()
            self.__state = 'connected'
            return True
        return False
    
    def disconnect(self, p = None):
        """
        This method disconnects.
        """
        if self.__state != 'disconnected':
            self.__thread_stop()
            self.__serial.close()
            self.__state = 'disconnected'
            return True
        return False
    
    def __scan(self):
        """
        Scan for available ports. return a list of tuples (num, name)
        """
        available = []
        for i in range(256):
            try:
                s = serial.Serial(i)
                available.append((i, s.portstr))
                s.close()   #explicit close 'cause of delayed GC in java
            except serial.SerialException:
                pass
        return available

    def write(self, data):
        if self.__state == 'connected':
            self.__serial.write(data) #was '\r\n'
            print "write('",data,"')"

    def __thread_start(self):
        #self.__thread = threading.Thread(None, self.__working_thread, 'SerialThread')
        self.__thread_enabled = True
        #self.__thread.daemon = True
        #self.__thread.start()
        #gobject.idle_add(self.__idle_write)
        glib.idle_add(self.__idle_write, priority=gobject.PRIORITY_DEFAULT_IDLE+1)
    
    def __thread_stop(self):
        self.__thread_enabled = False
        #self.__thread.join(10)
    
    def __idle_write(self, data = None):
        if  self.__thread_enabled and self.__serial.inWaiting() > 0:
            action_dispatcher = ActionDispatcher()
            data = self.__serial.read(4096)
            gtk.gdk.threads_enter()
            print "read('", data, "')"
            action_dispatcher['remote-input-data'](data)
            gtk.gdk.threads_leave()
        return self.__thread_enabled # False
    
    def __working_thread(self):
        while(self.__thread_enabled):
            data = self.__serial.read(4096)
            gtk.gdk.threads_enter()
            gobject.idle_add(self.__idle_write, data)
            gtk.gdk.threads_leave()

                
