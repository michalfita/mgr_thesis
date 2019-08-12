# -*- coding: utf-8 -*-

# pocdock - an experimental docking implementation
# Copyright (C) 2008 Andi Albrecht <albrecht.andi@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""An experimental docking implementation.

"""

import gtk
import gobject
import pango
try:
  from xml.etree import ElementTree as etree
except ImportError:
  from lxml import etree
from cStringIO import StringIO
import cairo

DOCK_ITEM_STATUS_DOCKED = 'docked'
DOCK_ITEM_STATUS_HIDDEN = 'hidden'
DOCK_ITEM_STATUS_FLOATING = 'floating'

DOCK_ITEM_BEH_NORMAL = 0
DOCK_ITEM_BEH_NEVER_FLOATING = 1 << 0
DOCK_ITEM_BEH_NEVER_VERTICAL = 1 << 1
DOCK_ITEM_BEH_NEVER_HORIZONTAL = 1 << 2
DOCK_ITEM_BEH_CANT_CLOSE = 1 << 3
DOCK_ITEM_BEH_CANT_AUTOHIDE = 1 << 4
DOCK_ITEM_BEH_LOCKED = 1 << 5


class Dock(gtk.VBox):
    """The docking widget."""
    
    def __init__(self):
        self.__gobject_init__()
        self.items = list()
        self.bar_top = DockBar(self, gtk.POS_TOP)
        self.bar_bottom = DockBar(self, gtk.POS_BOTTOM)
        self.bar_left = DockBar(self, gtk.POS_LEFT)
        self.bar_right = DockBar(self, gtk.POS_RIGHT)
        self.pack_start(self.bar_top, False, False, 0)
        hbox = gtk.HBox()
        hbox.pack_start(self.bar_left, False, False, 0)
        self.main = DockGroup(self, "__main__")
        hbox.pack_start(self.main, True, True, 0)
        hbox.pack_start(self.bar_right, False, False, 0)
        self.pack_start(hbox, True, True, 0)
        self.pack_start(self.bar_bottom, False, False, 0)
        
    def add_item(self, item):
        self.main.add_item(item)
        
    def find_group_by_position(self, dock_item, x, y):
        gx, gy = self.main.window.get_origin()
        if x >= gx and x <= gx+self.main.allocation.width \
        and y >= gy and y <= gy+self.main.allocation.width:
            return self.main.find_group_by_position(dock_item, x, y)
        else:
            return None, None
            
    def flatten_groups(self):
        return self.main.flatten_groups()
    
    def update_views(self):
        for group in self.flatten_groups():
            group.update_views()
                
    def save_layout(self, fp):
        dom = etree.Element("dock-layout")
        self.main.save_layout(dom)
        for item in self.items:
            if item.status == DOCK_ITEM_STATUS_FLOATING:
                win = item.get_parent()
                width = win.allocation.width
                height = win.allocation.height
                x, y = win.get_position()
                fl = etree.Element("floating")
                fl.set("x", str(x))
                fl.set("y", str(y))
                fl.set("width", str(width))
                fl.set("height", str(height))
                x = etree.Element("item")
                x.set("id", item.id)
                fl.append(x)
                dom.append(fl)
        fp.write(etree.tounicode(dom, pretty_print=True))
        
    def dump(self):
        fp = StringIO()
        self.save_layout(fp)
        print fp.getvalue()
        
        
class DockBar(gtk.HBox):
    """Container for *hidden* DockItems."""
    
    def __init__(self, dock, pos):
        gtk.HBox.__init__(self)
        self.dock = dock
        self.bar = gtk.Toolbar()
        self.pos = pos
        if pos in [gtk.POS_LEFT, gtk.POS_RIGHT]:
            self.bar.set_orientation(gtk.ORIENTATION_VERTICAL)
        else:
            self.bar.set_orientation(gtk.ORIENTATION_HORIZONTAL)
        self.pack_start(self.bar, True, True)
            
    def add_item(self, item):
        btn = gtk.ToolButton()
        btn.set_data("item", item)
        lbl = gtk.Label(item.label)
        btn.set_label_widget(lbl)
        if item.icon:
            it = gtk.icon_theme_get_default()
            pb = it.load_icon(item.icon, gtk.ICON_SIZE_MENU, gtk.ICON_LOOKUP_FORCE_SVG)
            i = gtk.Image()
            i.set_from_pixbuf(pb)
            btn.set_icon_widget(i)
        if self.pos in [gtk.POS_LEFT, gtk.POS_RIGHT]:
            lbl.set_angle(270)
        self.bar.insert(btn, -1)
        self.show_all()
        btn.connect("clicked", self.on_button_clicked)
        
    def on_button_clicked(self, btn):
        btn.get_data("item").show()
        btn.get_parent().remove(btn)
        btn.destroy()
        if not self.bar.get_n_items():
            self.hide()
            
class DockNotebook(gtk.Notebook):
    """The main view within a DockGroup"""
    
    def __init__(self):
        self.__gobject_init__()
        self.set_tab_pos(gtk.POS_BOTTOM)
        self.set_scrollable(True)
        self.popup_disable()
        self.connect("page-removed", self.on_page_count_changed)
        self.connect("page-added", self.on_page_count_changed)
        
    def on_page_count_changed(self, *args):
        self.set_show_tabs(self.get_n_pages() > 1)
        self.set_show_border(self.get_n_pages() > 1)


class DockGroup(gtk.EventBox):
    """A DockGroup holds a centered main view and up to four
    DockGroups (top, bottom, right, left)"""
    
    def __init__(self, dock, name):
        self.__gobject_init__()
        self.dock = dock
        self.grpname = name
        self.objects = list()
        self.vpaned1 = gtk.VPaned()
        self.add(self.vpaned1)
        self.vpaned2 = gtk.VPaned()
        self.vpaned1.pack2(self.vpaned2)
        self.hpaned1 = gtk.HPaned()
        self.vpaned2.pack1(self.hpaned1)
        self.hpaned2 = gtk.HPaned()
        self.hpaned1.pack2(self.hpaned2)
        self.notebook = DockNotebook()
        self.hpaned2.pack1(self.notebook)
        self.group_top = None
        self.group_bottom = None
        self.group_left = None
        self.group_right = None
        self.connect("remove", self.on_remove)
        
    def _get_subgroups(self):
        r = list()
        for item in ["top", "bottom", "left", "right"]:
            group = getattr(self, "group_%s" % item, None)
            if group:
                r.append(group)
        return r
    subgroups = property(fget=_get_subgroups)
        
    def on_remove(self, *args):
        if not self.group_bottom and not self.group_top \
        and not self.group_left and not self.group_right \
        and not self.notebook.get_n_pages() \
        and self.get_parent():
            self.get_parent().remove(self)
            self.destroy()
            return
            
    def on_group_destroyed(self, widget, group):
        gobject.idle_add(setattr, self, group, None)
        
    def flatten_groups(self):
        groups=[]
        if self.group_bottom:
            groups += self.group_bottom.flatten_groups()
        if self.group_top:
            groups += self.group_top.flatten_groups()
        if self.group_left:
            groups += self.group_left.flatten_groups()
        if self.group_right:
            groups += self.group_right.flatten_groups()
        groups.append(self)
        return groups
    
    def find_group_by_position(self, dock_item, x, y):
        for group in self.subgroups:
            gx, gy = group.window.get_origin()
            if x >= gx and x <= gx+group.allocation.width \
            and y >= gy and y <= gy+group.allocation.height:
                return group.find_group_by_position(dock_item, x, y)
        # no subgroups, if it's in notebook there's nothing to do
        if dock_item in self.notebook.get_children():
            return self, None
        gx, gy = self.window.get_origin()
        if y <= gy+self.allocation.height/3.0 \
        and dock_item.behavior & DOCK_ITEM_BEH_NEVER_VERTICAL == 0:
            pos = gtk.POS_TOP
        elif y >= gy+self.allocation.height-self.allocation.height/3.0 \
        and dock_item.behavior & DOCK_ITEM_BEH_NEVER_VERTICAL == 0:
            pos = gtk.POS_BOTTOM
        elif x <= gx+self.allocation.width/3.0 \
        and dock_item.behavior & DOCK_ITEM_BEH_NEVER_HORIZONTAL == 0:
            pos = gtk.POS_LEFT
        elif x >= gx+self.allocation.width-self.allocation.width/3.0 \
        and dock_item.behavior & DOCK_ITEM_BEH_NEVER_HORIZONTAL == 0:
            pos = gtk.POS_RIGHT
        else:
            pos = None
        return self, pos
        
    def add_item(self, item, pos=-1):
        if pos == -1:
            pos = item.initial_position
        else:
            pos = pos
        if pos == None:
            item.status = DOCK_ITEM_STATUS_DOCKED
            lbl = gtk.Label(item.label)
            lbl.set_single_line_mode(True)
            lbl.set_ellipsize(pango.ELLIPSIZE_END)
            lbl.set_max_width_chars(30)
            self.notebook.append_page(item, lbl)
            self.notebook.set_tab_reorderable(item, True)
            gobject.idle_add(self.notebook.set_current_page, self.notebook.get_n_pages()-1)
        elif pos == gtk.POS_TOP:
            if self.group_top:
                self.group_top.add_item(item, pos=None)
            else:
                self.group_top = DockGroup(self.dock, "top")
                self.group_top.add_item(item, pos=None)
                self.vpaned1.pack1(self.group_top)
                self.group_top.connect("destroy", self.on_group_destroyed, "group_top")
        elif pos == gtk.POS_BOTTOM:
            if self.group_bottom:
                self.group_bottom.add_item(item, pos=None)
            else:
                self.group_bottom = DockGroup(self.dock, "bottom")
                self.group_bottom.add_item(item, pos=None)
                self.vpaned2.pack2(self.group_bottom, False, False)
                self.group_bottom.connect("destroy", self.on_group_destroyed, "group_bottom")
        elif pos == gtk.POS_LEFT:
            if self.group_left:
                self.group_left.add_item(item, pos=None)
            else:
                self.group_left = DockGroup(self.dock, "left")
                self.group_left.add_item(item, pos=None)
                self.hpaned1.pack1(self.group_left)
                self.group_left.connect("destroy", self.on_group_destroyed, "group_left")
        elif pos == gtk.POS_RIGHT:
            if self.group_right:
                self.group_right.add_item(item, pos=None)
            else:
                self.group_right = DockGroup(self.dock, "right")
                self.group_right.add_item(item, pos=None)
                self.hpaned2.pack2(self.group_right)
                self.group_right.connect("destroy", self.on_group_destroyed, "group_right")
        self.show_all()
        self.dock.update_views()
        
    def relocate_item(self, item, pos):
        parent = item.get_parent()
        if isinstance(parent, gtk.Notebook):
            for n in range(parent.get_n_pages()):
                if parent.get_nth_page(n) == item:
                    parent.remove_page(n)
                    break
        elif item.get_parent():
            item.get_parent().remove(item)
            #item.destroy()
        self.add_item(item, pos)
        
    def has_items(self):
        if self.notebook.get_n_pages() != 0:
            return True
        if self.group_top \
        and self.group_top.has_items():
            return True
        if self.group_bottom \
        and self.group_bottom.has_items():
            return True
        if self.group_left \
        and self.group_left.has_items():
            return True
        if self.group_right \
        and self.group_right.has_items():
            return True
        return False

    def update_views(self):
        if self.group_top and not self.group_top.has_items():
            self.group_top.destroy()
        if self.group_bottom and not self.group_bottom.has_items():
            self.group_bottom.destroy()
        if self.group_left and not self.group_left.has_items():
            self.group_left.destroy()
        if self.group_right and not self.group_right.has_items():
            self.group_right.destroy()
        group_count = 0
        last_group = None
        if self.group_top:
            group_count += 1
            last_group = self.group_top
        if self.group_bottom:
            group_count += 1
            last_group = self.group_bottom
        if self.group_right:
            group_count += 1
            last_group = self.group_right
        if self.group_left:
            group_count += 1
            last_group = self.group_left
        if group_count == 1 and not self.notebook.get_n_pages():
            items = list()
            while last_group.notebook.get_n_pages():
                item = last_group.notebook.get_nth_page(0)
                last_group.notebook.remove_page(0)
                items.append(item)
            for item in items:
                self.add_item(item, pos=None)
            
    
    def show_dock_target(self, win, pos):
        x = y = width = height = None
        dx, dy = self.window.get_origin()
        if pos == gtk.POS_TOP:
            x = dx
            y = dy
            width = self.allocation.width
            height = self.allocation.height / 3
        elif pos == gtk.POS_BOTTOM:
            x = dx
            y = dy + self.allocation.height - self.allocation.height/3
            width = self.allocation.width
            height = self.allocation.height / 3
        elif pos == gtk.POS_LEFT:
            x = dx
            y = dy
            width = self.allocation.width / 3
            height = self.allocation.height
        elif pos == gtk.POS_RIGHT:
            x = dx + self.allocation.width - self.allocation.width / 3
            y = dy
            width = self.allocation.width / 3
            height = self.allocation.height
        elif pos == None:
            x = dx + self.allocation.width / 3
            y = dy + self.allocation.height / 3
            width = self.allocation.width / 3
            height = self.allocation.height / 3
        if width and height:
            win.relocate(x, y, width, height, True)
            win.show()
        else:
            win.hide()
            
    def save_layout(self, node, pos=None):
        grp = etree.Element("group")
        if pos:
            grp.set("pos", pos)
            grp.set("position", str(self.get_parent().get_position()))
        grp.set("name", self.grpname)
        node.append(grp)
        for n in range(self.notebook.get_n_pages()):
            item = self.notebook.get_nth_page(n)
            xitem = etree.Element("item")
            xitem.set("id", str(item.id))
            grp.append(xitem)
        if self.group_top:
            self.group_top.save_layout(grp, "top")
        if self.group_bottom:
            self.group_bottom.save_layout(grp, "bottom")
        if self.group_left:
            self.group_left.save_layout(grp, "left")
        if self.group_right:
            self.group_right.save_layout(grp, "right")
        
class DockItem(gtk.VBox):
    """A DockItem"""
    
    def __init__(self, dock, id, widget, label, icon=None, pos=None,
                 behavior=DOCK_ITEM_BEH_NORMAL):
        self.__gobject_init__()
        self.id = id
        self.dock = dock
        self.widget = widget
        self.label = label
        self.icon = icon
        self.behavior = behavior
        self.shadow_window = None
        self.floating_window = None
        self.status = None
        self.initial_position = pos
        self._setup_widget()
        
    def _setup_widget(self):
        self.header = gtk.EventBox()
        style = self.get_style()
        self.header.modify_bg(gtk.STATE_NORMAL, style.bg[gtk.STATE_ACTIVE])
        hbox = gtk.HBox()
        hbox.set_spacing(3)
        hbox.set_border_width(2)
        if self.icon:
            #i = gtk.image_new_from_stock(self.icon, gtk.ICON_SIZE_MENU)
            it = gtk.icon_theme_get_default()
            pb = it.load_icon(self.icon, gtk.ICON_SIZE_MENU, gtk.ICON_LOOKUP_FORCE_SVG)
            i = gtk.Image()
            i.set_from_pixbuf(pb)
            hbox.pack_start(i, False, False)
        a = gtk.Alignment(0, 0.5)
        a.add(gtk.Label(self.label))
        hbox.pack_start(a, True, True, 0)
        if self.behavior & DOCK_ITEM_BEH_LOCKED == 0:
            if self.behavior & DOCK_ITEM_BEH_CANT_AUTOHIDE == 0:
                btn = gtk.Button()
                btn.set_relief(gtk.RELIEF_NONE)
                btn.set_property("can-focus", False)
                btn.set_size_request(17, 17)
                btn.set_border_width(0)
                btn.set_image(gtk.image_new_from_stock("gtk-leave-fullscreen", gtk.ICON_SIZE_MENU))
                btn.connect("clicked", self.on_send_to_bar)
                hbox.pack_start(btn, False, False)
            if self.behavior & DOCK_ITEM_BEH_CANT_CLOSE == 0:
                btn = gtk.Button()
                btn.set_relief(gtk.RELIEF_NONE)
                btn.set_property("can-focus", False)
                btn.set_size_request(17, 17)
                btn.set_image(gtk.image_new_from_stock("gtk-close", gtk.ICON_SIZE_MENU))
                btn.connect("clicked", self.on_close)
                hbox.pack_start(btn, False, False)
        self.header.add(hbox)
        if self.behavior & DOCK_ITEM_BEH_LOCKED == 0:
            self.header.connect("enter-notify-event", self.on_header_enter)
            self.header.connect("leave-notify-event", self.on_header_leave)
            self.header.connect("button-press-event", self.on_header_button_press)
            self.header.connect("motion-notify-event", self.on_header_motion)
            self.header.connect("button-release-event", self.on_header_button_release)
        self.header.connect("realize", self.on_header_realize)
        self.pack_start(self.header, False, False)
        if self.widget.get_parent():
            self.widget.reparent(self)
            self.set_child_packing(self.widget, True, True, 0, gtk.PACK_START)
        else:
            self.pack_start(self.widget, True, True)
        self.dock.items.append(self)
        self.widget.connect("destroy", self.on_widget_destroy)
        self.connect("destroy", self.on_destroy)
            
    def on_close(self, *args):
        self.destroy()
        
    def on_destroy(self, *args):
        self.dock.items.remove(self)
        
    def on_send_to_bar(self, *args):
        self.send_to_bar()
        
    def on_header_enter(self, *args):
        self.header.set_state(gtk.STATE_PRELIGHT)
        
    def on_header_leave(self, *args):
        self.header.set_state(gtk.STATE_NORMAL)
        
    def on_header_button_press(self, widget, event):
        if event.button == 1:
            self._btn_xoffset = int(event.x)
            self._btn_yoffset = int(event.y)
            self.shadow_window = PlaceholderWindow(self.dock, self)
            self.header.window.set_cursor(gtk.gdk.Cursor(gtk.gdk.FLEUR))
            
    def on_header_button_release(self, widget, event):
        if event.button == 1:
            x, y = event.get_root_coords()
            group, pos = self.dock.find_group_by_position(self, x, y)
            if group:
                group.relocate_item(self, pos)
            else:
                x -= self._btn_xoffset
                y -= self._btn_yoffset
                self.set_floating(x, y)
                if isinstance(self.floating_window, gtk.Window):
                    gobject.idle_add(self.floating_window.show)
            self.shadow_window.destroy()
            self.shadow_window = None
            gobject.idle_add(self.dock.update_views)
            return
            
    def on_header_motion(self, widget, event):
        x, y = event.get_root_coords()
        group, pos = self.dock.find_group_by_position(self, x, y)
        if group:
            gobject.idle_add(group.show_dock_target, self.shadow_window, pos)
        elif self.behavior & DOCK_ITEM_BEH_NEVER_FLOATING == 0:
            x -= self._btn_xoffset
            y -= self._btn_yoffset
            self.shadow_window.show()
            gobject.idle_add(self.shadow_window.relocate, x, y, self.allocation.width, self.allocation.height, True)
            if isinstance(self.floating_window, gtk.Window):
                #gobject.idle_add(self.floating_window.hide)
                pass
        return
            
    def on_header_realize(self, header, *args):
        header.window.set_cursor(gtk.gdk.Cursor(gtk.gdk.HAND2))
        
    def on_widget_destroy(self, *args):
        gobject.idle_add(self.destroy)
        
    def send_to_bar(self):
        self.hide()
        parent = self.get_parent()
        topgroup = None
        while parent:
            if isinstance(parent, DockGroup) \
            and parent.grpname != "__main__":
                topgroup = parent
            parent = parent.get_parent()
        if not topgroup or topgroup.grpname == "top":
            bar = self.dock.bar_top
        elif topgroup.grpname == "left":
            bar = self.dock.bar_left
        elif topgroup.grpname == "right":
            bar = self.dock.bar_right
        else:
            bar = self.dock.bar_bottom
        bar.add_item(self)
        
    def set_floating(self, x, y):
        width = self.allocation.width
        height = self.allocation.height
        self.floating_window = FloatingWindow(self)
        self.floating_window.resize(width, height)
        self.floating_window.move(int(x), int(y))
        self.floating_window.show_all()
        self.status = DOCK_ITEM_STATUS_FLOATING

# Parts of the PlaceholderWindow are ported from MonoDevelop's
# docking implementation.       
class PlaceholderWindow(gtk.Window):
    """A *dummy* window which is displayed when dragging around DockItems"""
    redgc = None
    anim = 0
    rx = 0
    ry = 0
    rw = 0
    rh = 0
    allow_docking = True
    
    with_preview = False
    
    def __init__(self, dock, dockitem, xoffset = 0, yoffset = 0):
        gtk.Window.__init__(self, gtk.WINDOW_POPUP)
        self._xoffset = int(xoffset)
        self._yoffset = int(yoffset)
        self.use_alpha = False
        self.set_skip_pager_hint(True)
        self.set_decorated(False)
        self.set_transient_for(dock.get_toplevel())
        self.set_type_hint(gtk.gdk.WINDOW_TYPE_HINT_UTILITY)
        self._set_colormap()
        if self.with_preview:
            pb = gtk.gdk.pixbuf_get_from_drawable(None,
                        dockitem.window, dockitem.window.get_colormap(),
                        0, 0, 0, 0, dockitem.allocation.width, dockitem.allocation.height)
            f1 = 150.0/dockitem.allocation.width
            f2 = 250.0/dockitem.allocation.height
            if f1 < f2:
                w = dockitem.allocation.width*f1
                h = dockitem.allocation.height*f1
            else:
                w = dockitem.allocation.width*f2
                h = dockitem.allocation.height*f2
            pb = pb.scale_simple(int(w), int(h), gtk.gdk.INTERP_NEAREST)
            img = gtk.Image()
            img.set_from_pixbuf(pb)
            a = gtk.Alignment(0, 0)
            a.add(img)
            self.add(a)
            self.show_all()
        self.set_app_paintable(True)
        self.realize()
        if self.is_composited():
            self.connect("expose-event", self.expose)
        else:
            self.connect("expose-event", self.expose_simple)
        
    def _set_colormap(self):
        screen = self.get_screen()
        colormap = screen.get_rgba_colormap()
        self.use_alpha = True
        if not colormap:
            colormap = screen.get_rgb_colormap()
            self.use_alpha = False
        self.set_colormap(colormap)

    def relocate(self, x, y, w, h, animate):
        x = int(x)
        y = int(y)
        if x != self.rx or y != self.ry or w != self.rw or h != self.rh:
            self.move(x, y)
            self.resize(w, h)
            
            self.rx = x
            self.ry = y
            self.rw = w
            self.rh = h
            
            if self.anim != 0:
                gobject.source_remove(self.anim)
                self.anim = 0
            if animate and w < 150 and h < 150:
                sa = 7
                self.move(self.rx-sa, self.ry-sa)
                self.resize(self.rw+sa*2, self.rh+sa*2)
                self.anim = gobject.timeout_add(10, self.run_animation)
                

    def run_animation(self):
        cw, ch = self.get_size()
        cx, cy = self.get_position()
        if cx != self.rx:
            cx += 1
            cy += 1
            ch -= 2
            cw -= 2
            self.move(cx, cy)
            self.resize(cw, ch)
            return True
        self.anim = 0
        return False 
    
    def draw(self, widget, cr):
        rect = widget.get_allocation()
        draw_rb(cr, rect.x, rect.y, rect.width, rect.height, 5)
        cr.set_operator(cairo.OPERATOR_OVER)
        cr.set_source_rgba(0, 0, 0, .2)
        cr.fill_preserve()
            
    def expose(self, widget, event):
        cr = widget.window.cairo_create()
        cr.rectangle(event.area.x, event.area.y,
                     event.area.width, event.area.height)
        cr.clip()
        cr.set_source_rgba(0, 0, 0, .2)
        cr.set_operator( cairo.OPERATOR_SOURCE )
        cr.paint()
        self.draw(widget, cr)
        print("Expose")
        return False
    
    def expose_simple(self, widget, event):
        cr = widget.window.cairo_create()
        cr.rectangle(event.area.x, event.area.y,
                     event.area.width, event.area.height)
        cr.clip()
        cr.set_operator( cairo.OPERATOR_SOURCE )
        cr.set_source_rgb(0, 0, 0)
        width = self.allocation.width
        height = self.allocation.height
        cr.move_to(0, 0)
        cr.rel_line_to(0, height)
        cr.rel_line_to(width, 0)
        cr.rel_line_to(0, -height)
        cr.rel_line_to(-width, 0)
        cr.stroke()

# draw_cb is ported from GNOME Launch Box 
def draw_rb(cr, x, y, width, height, radius):
    import math
    width -= 2 * radius
    height -= 2 * radius

    cr.move_to(x + radius, y)

    cr.rel_line_to(width, 0.0)

    cx, cy = cr.get_current_point()
    cr.arc (cx, cy + radius, radius, 3.0 * math.pi/2, 0)

    cr.rel_line_to (0.0, height)

    cx, cy = cr.get_current_point ()
    cr.arc (cx - radius, cy, radius, 0, math.pi/2)

    cr.rel_line_to (- width, 0.0)

    cx, cy = cr.get_current_point()
    cr.arc (cx, cy - radius, radius, math.pi/2, math.pi)

    cr.rel_line_to (0.0, -height)

    cx, cy = cr.get_current_point ()
    cr.arc (cx + radius, cy, radius, math.pi, 3.0 * math.pi/2)

    cr.close_path()
    
class FloatingWindow(gtk.Window):
    """A window for floating DockItems"""
    
    def __init__(self, item):
        gtk.Window.__init__(self)
        self.item = item
        self.set_title(item.label)
        self.set_type_hint(gtk.gdk.WINDOW_TYPE_HINT_UTILITY)
        #self.window.set_decorations(gtk.gdk.DECOR_BORDER)
        self.item.reparent(self)
        self.connect("remove", self.on_remove)
        self.connect("show", self.on_show)
        
    def on_remove(self, *args):
        if not self.get_child():
            self.destroy()
    
    def on_show(self, *args):       
        self.window.set_decorations(gtk.gdk.DECOR_BORDER | gtk.gdk.DECOR_RESIZEH)