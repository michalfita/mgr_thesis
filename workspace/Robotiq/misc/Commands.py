#!/usr/bin/env python
# -*- coding: utf-8 -*-

""" @package Commands

 This module contains mechanics of handling commands between control buttons
 and communication output.

"""
import logging
import gettext
from collections import namedtuple
from misc.OrderedDict import OrderedDict
from ui.ActionDispatcher import ActionDispatcher
from misc.Singleton import Singleton
from misc.PersistentContainer import PersistentContainer
#from ui.CommandsUserInterface import CommandsUserInterface

def _(m): return m 

#CommandDefinition = namedtuple('CommandDefinition', 'action, name, command')
class CommandDefinition(object):
    __slot__ = 'action, type, name, command'
    def __init__(self, *args, **kwargs):
        self.__log = logging.getLogger('robotiq.cmd_def')
        for n, e in kwargs.items():
            self.__dict__[n] = e
    def __getattribute__(self, name):
        if name == 'command':
            persistent_container = PersistentContainer()
            self.__log.debug('Attribute %s = %s', name, repr(object.__getattribute__(self, name)))
            return persistent_container.get('commands', self.__dict__['action'], str, object.__getattribute__(self, name))
        return object.__getattribute__(self, name)
    
    def __setattr__(self, name, value):
        if name == 'command':
            persistent_container = PersistentContainer()
            persistent_container.put('commands', self.__dict__['action'], value) 
        return object.__setattr__(self, name, value)

commands_list = [
         CommandDefinition(action = 'control-up-press',             type = 'leg', name = _('Press Control Up'),               command = 'walk forward'),
         CommandDefinition(action = 'control-up-release',           type = 'leg', name = _('Release Control Up'),             command = ''),
         CommandDefinition(action = 'control-down-press',           type = 'leg', name = _('Press Control Down'),             command = 'walk backward'),
         CommandDefinition(action = 'control-down-release',         type = 'leg', name = _('Release Control Down'),           command = ''),
         CommandDefinition(action = 'control-left-press',           type = 'leg', name = _('Press Control Left'),             command = ''),
         CommandDefinition(action = 'control-left-release',         type = 'leg', name = _('Release Control Up'),             command = ''),
         CommandDefinition(action = 'control-right-press',          type = 'leg', name = _('Press Control Right'),            command = ''),
         CommandDefinition(action = 'control-right-release',        type = 'leg', name = _('Release Control Right'),          command = ''),
         CommandDefinition(action = 'control-stop-press',           type = 'leg', name = _('Press Control Stop'),             command = 'stop'),
         CommandDefinition(action = 'control-stop-release',         type = 'leg', name = _('Release Control Stop'),           command = ''),
         CommandDefinition(action = 'control-rightarmup-press',     type = 'arm', name = _('Press Control Right Arm Up'),     command = ''),
         CommandDefinition(action = 'control-rightarmup-release',   type = 'arm', name = _('Release Control Right Arm Up'),   command = ''),
         CommandDefinition(action = 'control-rightarmdown-press',   type = 'arm', name = _('Press Control Right Arm Down'),   command = ''),
         CommandDefinition(action = 'control-rightarmdown-release', type = 'arm', name = _('Release Control Right Arm Down'), command = ''),
         CommandDefinition(action = 'control-rightarmin-press',     type = 'arm', name = _('Press Control Right Arm In'),     command = ''),
         CommandDefinition(action = 'control-rightarmin-release',   type = 'arm', name = _('Release Control Right Arm In'),   command = ''),
         CommandDefinition(action = 'control-rightarmout-press',    type = 'arm', name = _('Press Control Right Arm Out'),    command = ''),
         CommandDefinition(action = 'control-rightarmout-release',  type = 'arm', name = _('Release Control Right Arm Out'),  command = ''),
         CommandDefinition(action = 'control-tiltright-press',      type = 'hip', name = _('Press Control Tilt Right'),       command = ''),
         CommandDefinition(action = 'control-tiltright-release',    type = 'hip', name = _('Release Control Tilt Right'),     command = ''),
         CommandDefinition(action = 'control-leftarmup-press',      type = 'arm', name = _('Press Control Left Arm Up'),      command = ''),
         CommandDefinition(action = 'control-leftarmup-release',    type = 'arm', name = _('Release Control Left Arm Up'),    command = ''),
         CommandDefinition(action = 'control-leftarmdown-press',    type = 'arm', name = _('Press Control Left Arm Down'),    command = ''),
         CommandDefinition(action = 'control-leftarmdown-release',  type = 'arm', name = _('Release Control Left Arm Down'),  command = ''),
         CommandDefinition(action = 'control-leftarmin-press',      type = 'arm', name = _('Press Control Left Arm In'),      command = ''),
         CommandDefinition(action = 'control-leftarmin-release',    type = 'arm', name = _('Release Control Left Arm In'),    command = ''),
         CommandDefinition(action = 'control-leftarmout-press',     type = 'arm', name = _('Press Control Left Arm Out'),     command = ''),
         CommandDefinition(action = 'control-leftarmout-release',   type = 'arm', name = _('Release Control Left Arm Out'),   command = ''),
         CommandDefinition(action = 'control-tiltleft-press',       type = 'hip', name = _('Press Control Tilt Left'),        command = ''),
         CommandDefinition(action = 'control-tiltleft-release',     type = 'hip', name = _('Release Control Tilt Right'),     command = ''),
    ]                         

del _

class Commands(Singleton):
    
    class CallCommand(object):
        def __init__(self, parent, action):
            self.__parent = parent
            self.__action = action
            
        def __call__(self):
            action_dispatcher = ActionDispatcher()
            data = self.__parent.get_commands_dict()[self.__action].command + '\r'
            action_dispatcher['local-output-data'](data)

    def __init__(self):
        self.__log = logging.getLogger('robotiq.commands')
        self.__commands_dict = OrderedDict()
        self.__commands_types = set()
        self.__process_commands_db()
        
    def __process_commands_db(self):
        action_dispatcher = ActionDispatcher()
        for cd in commands_list:
            self.__commands_dict[cd.action] = cd
            self.__commands_types.add(cd.type)
            action_dispatcher[cd.action] += Commands.CallCommand(self, cd.action)

    def get_commands_dict(self):
        return self.__commands_dict
    
    def get_types(self):
        return self.__commands_types