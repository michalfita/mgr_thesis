#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""@package ui.ActionDispatcher
Event handlers dispatcher module.

This module contains classes related to dispatching event handlers, it is
somehow implementaiton of the observer design pattern.
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

class ActionDelegate:
    '''Handles a list of methods and functions.
    
    Instances of this class are used by ActionDispatcher instances for calling
    registered methods, functions or other callable object for particular event
    name.
    @see ActionDispatcher
    
    Usage:\verbatim
        d = ActionDelegate()
        d += function    # Add function to end of delegate list
        d(*args, **kw)   # Call all functions, returns a list of results
        d -= function    # Removes last matching function from list
        d -= object      # Removes all methods of object from list
    \endverbatim
    '''
    def __init__(self):
        """Object constructor.
        """
        self.__delegates = []

    def __iadd__(self, callback):
        """The + operator.
        
        Allows addition of new callback callable object, that will be called for
        event refered by name in ActionDispatcher instance. Named keys refer to
        instances of this class keeping sequences of methods to be called.
        @param callback A callable object to assign.
        @return The same instance of this class.
        """
        self.__delegates.append(callback)
        return self

    def __isub__(self, callback):
        """The - operator.
        
        Allows substraction of callback callable object, that was called for
        event refered by name in ActionDispatcher instance. Named keys refer to
        instances of this class keeping sequences of methods to be called.
        @param callback A callable object to unassign.
        @return The same instance of this class.
        """
        # If callback is a class instance,
        # remove all callbacks for that instance
        self.__delegates = [ cb
            for cb in self.__delegates
                if getattr(cb, 'im_self', None) != callback]

        # If callback is callable, remove the last
        # matching callback
        if callable(callback):
            for i in range(len(self.__delegates)-1, -1, -1):
                if self.__delegates[i] == callback:
                    del self.__delegates[i]
                    return self
        return self
        

    def __call__(self, *args, **kw):
        """Callable method.
        
        This method allows instance of this object to be treated like a
        function. It calls all callbacks stored.
        @param *args All arguments passed here are forwarded to stored callbacks.
        @param **kw @see args
        @return List of results from all callbacks.
        """
        return [ callback(*args, **kw)
            for callback in self.__delegates ]


class ActionDispatcher(Singleton):
    """Collection of named events.
    
    Stores named events associated with ActionDelegate instances available
    as collection elements, which may be called to call registered methods,
    functions or other callable objects.
    
    This frees programmer from remember which methods have to be called in case
    of particural event. It additionally simplifies extendability of the code by
    limiting additions of new handlers to locally used ActionDispatcher instance.
    
    \note This is a singleton.
    """
    def __init__(self):
        super(ActionDispatcher, self).__init__()
        self.__action_delegate_map = {}
        
    def _add_action(self, name, method): # TODO: Probably won't needed anymore
        if not isinstance(name, str):
            raise TypeError(_('New action has to have name string, other types are not allowed.'))
        if not callable(method):
            raise TypeError(_('New action has to have callable object as second argument.'))
        self.__action_delegate_map[name] = method    
        return
    
    def get_delegate(self, name):
        """Gets ActionDelegate instance.
        
        This method gets ActionDelegate instance stored in the collection under
        given name.
        @see __getitem__
        @param name Name of the ActionDeletagte instance in the collection.
        @return Instance of ActionDelegate refered by name.
        """
        if not isinstance(name, str):
            raise TypeError(_('New action has to have name string, other types are not allowed.'))
        if name not in self.__action_delegate_map:
            delegate = ActionDelegate()
            self.__action_delegate_map[name] = delegate
            
        return self.__action_delegate_map[name]
        
    def remove_delegate(self, name):
        """Remove ActionDelegate instance.
        
        This method removes instance of ActionDelegate class hold in the
        collection under \p name.
        @see remove_delegate
        @param name Name of the ActionDeletagte instance in the collection.
        """
        if not isinstance(name, str):
            raise TypeError(_('New action has to have name string, other types are not allowed.'))
        if name not in self.__action_delegate_map:
            raise AtributeError(_('No such action as "%s" found.') % name)
        del self.__action_delegate_map[name]
        
    def __setitem__(self, name, delegate):
        """Stub for -= and += operation.
        
        Existence of this method allows operations such as \p -= and \p += on
        instances of ActionDispatcher class. These operations allows additions
        and removes of methods in ActionDelegate instances held by instances of
        ActionDispatcher.
        @see ActionDispatcher::__iadd__
        @see ActionDispatcher::__isub__
        @param name Key of passed from \p [key] operator.
        @param delegate The instance of ActionDelegate class.
        """
        if name not in self.__action_delegate_map:
            return
        if self.__action_delegate_map[name] is not delegate:
            raise Exception(_('You failed, sucker! You cannot dispatch item, only add to and remove from action.')) # FIXME: new kind of exception?
        return
        
    def __getitem__(self, name):
        """Gets ActionDelegate instance.
        
        This method gets ActionDelegate instance stored in the collection under
        given name, but it is indirectly used by \p [key] operation on Python
        sequences.
        @see get_delegate
        @param name Name of the ActionDeletagte instance in the collection.
        @return Instance of ActionDelegate refered by \p name.
        """
        return self.get_delegate(name)
        
    def __delitem__(self, name):
        """Remove ActionDelegate instance.
        
        This method removes instance of ActionDelegate class hold in the
        collection under \p name, but it is indirectly used by \p [key] operaion
        on Python sequences.
        @see remove_delegate
        @param name Name of the ActionDeletagte instance in the collection.
        """
        return self.remove_delegate(name)
    
    def __contains__(self, name):
        """Check for ActionDelegate instance.
        
        Implementation of one from abstract Python sequence methods.
        @param name Name of the ActionDeletagte instance in the collection.
        @return Boolean value whether sequence has element under \p name.
        """
        if name in self.__action_delegate_map:
            return True
        return False
        
    def __iter__(self):
        """Return iterator.
        @return Iterator over sequence.
        @warning Consider taking iterators as not safest method, as it currently
                 returns iterator over ActionDispatcher's internal sequence.
        """
        return self.__action_delegate_map.__iter__() # FIXME: Shouldn't we protect it better?
        