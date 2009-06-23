#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""@package comm.CommunicationManager

This module is responsible for communication modules management.

This is the container for all communication modules available and the manager
for all communication operations. It abstracts the communicaiton operations from
detailed operations on channel specific classes.

Relations between classes in this module are really crazy.
"""

import serial
import threading
import gettext
import gtk
from ui.ActionDispatcher import ActionDispatcher
from misc.Singleton import Singleton

class MetaCommunicator(type):
    def __new__(cls, name, bases, dct):
        print "Create %s communication class." % name
        class_object = type.__new__(cls, name, bases, dct)
        CommunicationManager.register_channel_class(name, class_object)
        return class_object
    
    def __call__(cls, *args, **kwargs):
        instance = CommunicationManager.get_channel_instance(cls.__name__)
        if instance is None:
            instance = cls.__new__(cls, *args, **kwargs)
            print "Created %s instance." % instance.__class__.__name__
            instance.__init__(*args, **kwargs)
            CommunicationManager.register_channel_instance(cls.__name__, instance)
        return instance
        

class AbstractCommunicator(object):
    def __init__(self):
        if self.__class__.__name__ == "AbstractCommunicator":
            raise TypeError('Abstract class cannot be used directly.')
    
class CommunicationManager(Singleton):

    ## Private container for class objects of channel modules
    __channels_class_dict = {}
    
    ## Private container for object of channel classes
    __channels_obj_dict = {}
    
    def __init__(self):
        super(self.__class__, self).__init__()
        # Make sure all communicators are instantiated
        for name, cls in self.__class__.__channels_class_dict.items():
            obj = cls()
            # consider default setup here
    
    @property
    def state(self):
        return 'disconnected'
    
    @classmethod    
    def register_channel_class(cls, typename, typeobject):
        assert typename not in cls.__channels_class_dict
        cls.__channels_class_dict[typename] = typeobject
    
    @classmethod
    def register_channel_instance(cls, typename, instance):
        assert typename not in cls.__channels_obj_dict
        cls.__channels_obj_dict[typename] = instance
    
    @classmethod
    def get_channel_instance(cls, typename):
        if typename in cls.__channels_obj_dict:
            return cls.__channels_obj_dict[typename]
        return None
        
    def get_channels_data(self):
        """This method returns channel data.
        
        This method prepares and returns data about registered channels for the
        GUI, they are then used to display proper options in menu and in
        configuration dialog.
        """
        channels_data = []
        for name, obj in self.__class__.__channels_obj_dict.items():
            print '#Name',name
            chan_data = {
                'name' : name,
                'action' : 'comm-%s-select' % str(hash(obj)),
                'menu' : obj.menu_text,
                'tooltip' : obj.menu_tooltip,
                'parameters' : obj.parameters,
                'setup_method' : obj.setup
            }
            channels_data.append(chan_data)
        return channels_data