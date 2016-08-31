#!/usr/bin/env python

import gobject
import dbus
import dbus.service
import dbus.mainloop.glib
import obmc.dbuslib.propertycacher as PropertyCacher
from obmc.dbuslib.bindings import get_dbus, DbusProperties, DbusObjectManager

try:
    import obmc_system_config as System
    have_system = True
except ImportError:
    have_system = False

INTF_NAME = 'org.openbmc.InventoryItem'
DBUS_NAME = 'org.openbmc.Inventory'

if have_system:
    FRUS = System.FRU_INSTANCES
else:
    FRUS = {}


class Inventory(DbusProperties, DbusObjectManager):
    def __init__(self, bus, name):
        DbusProperties.__init__(self)
        DbusObjectManager.__init__(self)
        dbus.service.Object.__init__(self, bus, name)


class InventoryItem(DbusProperties):
    def __init__(self, bus, name, data):
        DbusProperties.__init__(self)
        dbus.service.Object.__init__(self, bus, name)

        self.name = name

        if 'present' not in data:
            data['present'] = 'False'
        if 'fault' not in data:
            data['fault'] = 'False'
        if 'version' not in data:
            data['version'] = ''

        self.SetMultiple(INTF_NAME, data)

        ## this will load properties from cache
        PropertyCacher.load(name, INTF_NAME, self.properties)

    @dbus.service.method(
        INTF_NAME, in_signature='a{sv}', out_signature='')
    def update(self, data):
        self.SetMultiple(INTF_NAME, data)
        PropertyCacher.save(self.name, INTF_NAME, self.properties)

    @dbus.service.method(
        INTF_NAME, in_signature='s', out_signature='')
    def setPresent(self, present):
        self.Set(INTF_NAME, 'present', present)
        PropertyCacher.save(self.name, INTF_NAME, self.properties)

    @dbus.service.method(
        INTF_NAME, in_signature='s', out_signature='')
    def setFault(self, fault):
        self.Set(INTF_NAME, 'fault', fault)
        PropertyCacher.save(self.name, INTF_NAME, self.properties)


def getVersion():
    version = "Error"
    with open('/etc/os-release', 'r') as f:
        for line in f:
            p = line.rstrip('\n')
            parts = line.rstrip('\n').split('=')
            if (parts[0] == "VERSION_ID"):
                version = parts[1]
                version = version.strip('"')
    return version


if __name__ == '__main__':
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
    bus = get_dbus()
    mainloop = gobject.MainLoop()
    obj_parent = Inventory(bus, '/org/openbmc/inventory')

    for f in FRUS.keys():
        obj_path = f.replace("<inventory_root>", System.INVENTORY_ROOT)
        obj = InventoryItem(bus, obj_path, FRUS[f])
        obj_parent.add(obj_path, obj)

        ## TODO:  this is a hack to update bmc inventory item with version
        ## should be done by flash object
        if (FRUS[f]['fru_type'] == "BMC"):
            version = getVersion()
            obj.update({'version': version})

    obj_parent.unmask_signals()
    name = dbus.service.BusName(DBUS_NAME, bus)
    print "Running Inventory Manager"
    mainloop.run()
