#include <gtk/gtk.h>

static gboolean key_event(GtkWidget *widget, GdkEventKey *event) {
    g_printerr("a %2x\n",  event->hardware_keycode);
    g_printerr("%s\n",  gdk_keyval_name(event->keyval));
    return FALSE;
}

int main(int argc, char *argv[]) {

    GtkWidget *window;
    gtk_init(&argc, &argv);
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(window, "key-release-event", G_CALLBACK(key_event), NULL);
    gtk_widget_show(window);
    gtk_main();

    return 0;
}