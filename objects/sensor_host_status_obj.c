#include "interfaces/sensor.h"
#include "openbmc.h"


/* ---------------------------------------------------------------------------------------------------- */

static const gchar* dbus_object_path = "/org/openbmc/sensors/HostStatus";
static const gchar* dbus_name        = "org.openbmc.sensors.HostStatus";
static const guint poll_interval = 3000;
static guint heartbeat = 0;

static GDBusObjectManagerServer *manager = NULL;
/*
static gboolean
on_get_units    (SensorValue  *sen,
                GDBusMethodInvocation  *invocation,
                gpointer                user_data)
{
  const gchar* val = sensor_value_get_units(sen);
  sensor_value_complete_get_units(sen,invocation,val);
  return TRUE;
}

static gboolean
on_get (SensorValue                 *sen,
                GDBusMethodInvocation  *invocation,
                gpointer                user_data)
{
  guint reading = sensor_value_get_value(sen);
  sensor_value_complete_get_value(sen,invocation,reading);
  return TRUE;
}
static gboolean
on_set (SensorValue                 *sen,		
                GDBusMethodInvocation  *invocation,
		guint                   value,
                gpointer                user_data)
{
	GVariant* v = NEW_VARIANT_U(value);
	sensor_value_set_value(sen,v);
	sensor_value_emit_changed(sen,v,sensor_value_get_units(sen));
	sensor_value_complete_set_value(sen,invocation);
	return TRUE;
}
*/
static void 
on_bus_acquired (GDBusConnection *connection,
                 const gchar     *name,
                 gpointer         user_data)
{
  	g_print ("Acquired a message bus connection: %s\n",name);

  	cmdline *cmd = user_data;
	if (cmd->argc < 2)
	{
		g_print("No objects created.  Put object name(s) on command line\n");
		return;
	}	
  	manager = g_dbus_object_manager_server_new (dbus_object_path);
  	int i=0;
  	for (i=1;i<cmd->argc;i++)
  	{
		gchar *s;
 		s = g_strdup_printf ("%s/%s",dbus_object_path,cmd->argv[i]);
		ObjectSkeleton *object = object_skeleton_new (s);
		g_free (s);

		SensorValue *sensor = sensor_value_skeleton_new ();
  		object_skeleton_set_sensor_value (object, sensor);
  		g_object_unref (sensor);
	
		//must init variant
		GVariant* v = NEW_VARIANT_U(0);
		sensor_value_set_value(sensor,v);
	
  		// set units
  		sensor_value_set_units(sensor,"");
  		//define method callbacks here
  		//g_signal_connect (sensor,
                //    "handle-get-value",
                //    G_CALLBACK (on_get),
                //    NULL); /* user_data */
  		//g_signal_connect (sensor,
                //    "handle-get-units",
                //    G_CALLBACK (on_get_units),
                //    NULL); /* user_data */
  		//g_signal_connect (sensor,
                //    "handle-set-value",
                //    G_CALLBACK (on_set),
                //    NULL); /* user_data */

  		/* Export the object (@manager takes its own reference to @object) */
  		g_dbus_object_manager_server_export (manager, G_DBUS_OBJECT_SKELETON (object));
  		g_object_unref (object);
	}

  /* Export all objects */
  g_dbus_object_manager_server_set_connection (manager, connection);
}

static void
on_name_acquired (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
  g_print ("Acquired the name %s\n", name);
}

static void
on_name_lost (GDBusConnection *connection,
              const gchar     *name,
              gpointer         user_data)
{
  g_print ("Lost the name %s\n", name);
}


gint
main (gint argc, gchar *argv[])
{
  GMainLoop *loop;
  cmdline cmd;
  cmd.argc = argc;
  cmd.argv = argv;
  guint id;
  loop = g_main_loop_new (NULL, FALSE);

  id = g_bus_own_name (G_BUS_TYPE_SESSION,
                       dbus_name,
                       G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
                       G_BUS_NAME_OWNER_FLAGS_REPLACE,
                       on_bus_acquired,
                       on_name_acquired,
                       on_name_lost,
                       &cmd,
                       NULL);

  g_main_loop_run (loop);
  
  g_bus_unown_name (id);
  g_main_loop_unref (loop);
  return 0;
}
