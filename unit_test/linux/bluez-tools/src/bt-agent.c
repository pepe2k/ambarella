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

static gboolean need_unregister = TRUE;
static GMainLoop *mainloop = NULL;

static void sigterm_handler(int sig)
{
	g_message("%s received", sig == SIGTERM ? "SIGTERM" : "SIGINT");

	if (g_main_loop_is_running(mainloop))
		g_main_loop_quit(mainloop);
}

static void agent_released(Agent *agent, gpointer data)
{
	g_assert(data != NULL);
	GMainLoop *mainloop = data;

	need_unregister = FALSE;

	if (g_main_loop_is_running(mainloop))
		g_main_loop_quit(mainloop);
}

static gchar *adapter_arg = NULL;

static GOptionEntry entries[] = {
	{"adapter", 'a', 0, G_OPTION_ARG_STRING, &adapter_arg, "Adapter Name or MAC", "<name|mac>"},
	{NULL}
};

int main(int argc, char *argv[])
{
	GError *error = NULL;
	GOptionContext *context;

	/* Query current locale */
	setlocale(LC_CTYPE, "");
#if GLIB_VERSION_MIN_REQUIRED < GLIB_VERSION_2_36
	g_type_init();
#endif
	dbus_init();

	context = g_option_context_new(" - a bluetooth agent");
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_summary(context, "Version "PACKAGE_VERSION);
	g_option_context_set_description(context,
			//"Report bugs to <"PACKAGE_BUGREPORT">."
			"Project home page <"PACKAGE_URL">."
			);

	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("%s: %s\n", g_get_prgname(), error->message);
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

	mainloop = g_main_loop_new(NULL, FALSE);

	Manager *manager = g_object_new(MANAGER_TYPE, NULL);

	Adapter *adapter = find_adapter(adapter_arg, &error);
	exit_if_error(error);

	Agent *agent = g_object_new(AGENT_TYPE, NULL);

	adapter_register_agent(adapter, AGENT_DBUS_PATH, "DisplayYesNo", &error);
	exit_if_error(error);

	g_signal_connect(agent, "AgentReleased", G_CALLBACK(agent_released), mainloop);

	/* Add SIGTERM && SIGINT handlers */
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigterm_handler;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);

	g_main_loop_run(mainloop);

	if (need_unregister) {
		adapter_unregister_agent(adapter, AGENT_DBUS_PATH, &error);
		exit_if_error(error);

		/* Waiting for AgentReleased signal */
		g_main_loop_run(mainloop);
	}

	g_main_loop_unref(mainloop);

	g_object_unref(agent);
	g_object_unref(adapter);
	g_object_unref(manager);

	dbus_disconnect();

	exit(EXIT_SUCCESS);
}

