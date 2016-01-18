/*
 *
 *  bluez-tools - a set of tools to manage bluetooth devices for linux
 *
 *  Copyright (C) 2010  Alexander Orlenko <zxteam@gmail.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <glib.h>

#include "lib/dbus-common.h"
#include "lib/helpers.h"
#include "lib/bluez-api.h"

static GMainLoop *mainloop = NULL;

static void sigterm_handler(int sig)
{
	g_message("%s received", sig == SIGTERM ? "SIGTERM" : "SIGINT");
	g_main_loop_quit(mainloop);
}

static void trap_signals()
{
	/* Add SIGTERM && SIGINT handlers */
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigterm_handler;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
}

static void network_property_changed(Network *network, const gchar *name, const GValue *value, gpointer data)
{
	g_assert(data != NULL);
	GMainLoop *mainloop = data;

	if (g_strcmp0(name, "Connected") == 0) {
		if (g_value_get_boolean(value) == TRUE) {
			g_print("Network service is connected\n");
		} else {
			g_print("Network service is disconnected\n");
			g_main_loop_quit(mainloop);
		}
	} else if (g_strcmp0(name, "Interface") == 0) {
		g_print("Interface: %s\n", g_value_get_string(value));
	} else if (g_strcmp0(name, "UUID") == 0) {
		g_print("UUID: %s (%s)\n", uuid2name(g_value_get_string(value)), g_value_get_string(value));
	}
}

static gchar *adapter_arg = NULL;
static gboolean connect_arg = FALSE;
static gchar *connect_device_arg = NULL;
static gchar *connect_uuid_arg = NULL;
static gboolean server_arg = FALSE;
static gchar *server_uuid_arg = NULL;
static gchar *server_brige_arg = NULL;

static GOptionEntry entries[] = {
	{"adapter", 'a', 0, G_OPTION_ARG_STRING, &adapter_arg, "Adapter Name or MAC", "<name|mac>"},
	{"connect", 'c', 0, G_OPTION_ARG_NONE, &connect_arg, "Connect to the network device", NULL},
	{"server", 's', 0, G_OPTION_ARG_NONE, &server_arg, "Start GN/PANU/NAP server", NULL},
	{NULL}
};

int main(int argc, char *argv[])
{
	GError *error = NULL;
	GOptionContext *context;

	/* Query current locale */
	setlocale(LC_CTYPE, "");

	g_type_init();
	dbus_init();

	context = g_option_context_new("- a bluetooth network manager");
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_summary(context, "Version "PACKAGE_VERSION);
	g_option_context_set_description(context,
			"Connect Options:\n"
			"  -c, --connect <name|mac> <uuid>\n"
			"  Where\n"
			"    `name|mac` is a device Name or MAC\n"
			"    `uuid` is:\n"
			"       Profile short name: gn, panu or nap\n\n"
			"Server Options:\n"
			"  -s, --server <gn|panu|nap> <brige>\n"
			"  Every new connection to this server will be added the `bridge` interface\n\n"
			//"Report bugs to <"PACKAGE_BUGREPORT">."
			"Project home page <"PACKAGE_URL">."
			);

	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("%s: %s\n", g_get_prgname(), error->message);
		g_print("Try `%s --help` for more information.\n", g_get_prgname());
		exit(EXIT_FAILURE);
	} else if (!connect_arg && !server_arg) {
		g_print("%s", g_option_context_get_help(context, FALSE, NULL));
		exit(EXIT_FAILURE);
	} else if (connect_arg && (argc != 3 || strlen(argv[1]) == 0 || strlen(argv[2]) == 0)) {
		g_print("%s: Invalid arguments for --connect\n", g_get_prgname());
		g_print("Try `%s --help` for more information.\n", g_get_prgname());
		exit(EXIT_FAILURE);
	} else if (server_arg && (argc != 3 || strlen(argv[1]) == 0 || strlen(argv[2]) == 0)) {
		g_print("%s: Invalid arguments for --server\n", g_get_prgname());
		g_print("Try `%s --help` for more information.\n", g_get_prgname());
		exit(EXIT_FAILURE);
	}

	g_option_context_free(context);

	if (!dbus_system_connect(&error)) {
		g_printerr("Couldn't connect to DBus system bus: %s\n", error->message);
		exit(EXIT_FAILURE);
	}

	/* Check, that bluetooth daemon is running */
	if (!intf_supported(BLUEZ_DBUS_NAME, MANAGER_DBUS_PATH, MANAGER_DBUS_INTERFACE)) {
		g_printerr("%s: bluez service is not found\n", g_get_prgname());
		g_printerr("Did you forget to run bluetoothd?\n");
		exit(EXIT_FAILURE);
	}

	Adapter *adapter = find_adapter(adapter_arg, &error);
	exit_if_error(error);

	if (connect_arg) {
		connect_device_arg = argv[1];
		connect_uuid_arg = argv[2];

		Device *device = find_device(adapter, connect_device_arg, &error);
		exit_if_error(error);

		if (!intf_supported(BLUEZ_DBUS_NAME, device_get_dbus_object_path(device), NETWORK_DBUS_INTERFACE)) {
			g_printerr("Network service is not supported by this device\n");
			exit(EXIT_FAILURE);
		}

		mainloop = g_main_loop_new(NULL, FALSE);

		Network *network = g_object_new(NETWORK_TYPE, "DBusObjectPath", device_get_dbus_object_path(device), NULL);
		g_signal_connect(network, "PropertyChanged", G_CALLBACK(network_property_changed), mainloop);

		if (network_get_connected(network) == TRUE) {
			g_print("Network service is already connected\n");
		} else {
			gchar *intf = network_connect(network, connect_uuid_arg, &error);
			exit_if_error(error);
			trap_signals();
			g_main_loop_run(mainloop);

			/* Force disconnect the network device */
			if (network_get_connected(network) == TRUE) {
				network_disconnect(network, NULL);
			}
			g_free(intf);
		}

		g_main_loop_unref(mainloop);
		g_object_unref(network);
		g_object_unref(device);
	} else if (server_arg) {
		server_uuid_arg = argv[1];
		server_brige_arg = argv[2];

		if (g_ascii_strcasecmp(server_uuid_arg, "gn") != 0 && g_ascii_strcasecmp(server_uuid_arg, "panu") != 0 && g_ascii_strcasecmp(server_uuid_arg, "nap") != 0) {
			g_print("%s: Invalid server UUID: %s\n", g_get_prgname(), server_uuid_arg);
			g_print("Try `%s --help` for more information.\n", g_get_prgname());
			exit(EXIT_FAILURE);
		}

		if (!intf_supported(BLUEZ_DBUS_NAME, adapter_get_dbus_object_path(adapter), NETWORK_SERVER_DBUS_INTERFACE)) {
			g_printerr("Network server is not supported by this adapter\n");
			exit(EXIT_FAILURE);
		}

		gchar *server_uuid_upper = g_ascii_strup(server_uuid_arg, -1);

		NetworkServer *network_server = g_object_new(NETWORK_SERVER_TYPE, "DBusObjectPath", adapter_get_dbus_object_path(adapter), NULL);
		network_server_register(network_server, server_uuid_arg, server_brige_arg, &error);
		exit_if_error(error);
		g_print("%s server registered\n", server_uuid_upper);

		mainloop = g_main_loop_new(NULL, FALSE);

		trap_signals();

		g_main_loop_run(mainloop);

		network_server_unregister(network_server, server_uuid_arg, &error);
		exit_if_error(error);
		g_print("%s server unregistered\n", server_uuid_upper);

		g_main_loop_unref(mainloop);
		g_free(server_uuid_upper);
		g_object_unref(network_server);
	}

	g_object_unref(adapter);
	dbus_disconnect();

	exit(EXIT_SUCCESS);
}

