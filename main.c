#include <gtk/gtk.h>
#include "camera.h"
#include <ctype.h>

typedef struct {
    GtkEntry *ip_entry;
    GtkEntry *user_entry;
    GtkEntry *pass_entry;
    GtkListStore *store;
    GtkTreeView *tree_view;
} AppData;

static void on_refresh_clicked(GtkButton *button, gpointer user_data) {
    AppData *data = (AppData *)user_data;
    const char *ip = gtk_entry_get_text(data->ip_entry);
    const char *user = gtk_entry_get_text(data->user_entry);
    const char *pass = gtk_entry_get_text(data->pass_entry);
    
    CameraPresetList *list = camera_get_presets(ip, user, pass);
    if (list) {
        gtk_list_store_clear(data->store);
        
        for (int i = 0; i < list->count; ++i) {
            GtkTreeIter iter;
            gtk_list_store_append(data->store, &iter);
            gtk_list_store_set(data->store, &iter, 
                              0, list->presets[i].id,
                              1, list->presets[i].name,
                              2, list->presets[i].pan,
                              3, list->presets[i].tilt,
                              4, list->presets[i].zoom,
                              5, list->presets[i].focus,
                              -1);
        }
        camera_free_preset_list(list);
    }
}

static void on_edit_clicked(GtkButton *button, gpointer user_data) {
    AppData *data = (AppData *)user_data;
    GtkTreeSelection *selection = gtk_tree_view_get_selection(data->tree_view);
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        int id;
        gchar *name, *pan, *tilt, *zoom, *focus;
        gtk_tree_model_get(model, &iter,
                          0, &id,
                          1, &name,
                          2, &pan,
                          3, &tilt,
                          4, &zoom,
                          5, &focus,
                          -1);
        
        // สร้าง dialog สำหรับแก้ไข
        GtkWidget *dialog = gtk_dialog_new_with_buttons("Edit Preset",
                                                      GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))),
                                                      GTK_DIALOG_MODAL,
                                                      "OK", GTK_RESPONSE_OK,
                                                      "Cancel", GTK_RESPONSE_CANCEL,
                                                      NULL);
        
        GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        GtkWidget *grid = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
        gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
        gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
        
        GtkWidget *id_entry = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(id_entry), g_strdup_printf("%d", id));
        gtk_grid_attach(GTK_GRID(grid), gtk_label_new("ID:"), 0, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), id_entry, 1, 0, 1, 1);
        
        GtkWidget *name_entry = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(name_entry), name ? name : "");
        gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Name:"), 0, 1, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), name_entry, 1, 1, 1, 1);
        
        gtk_container_add(GTK_CONTAINER(content), grid);
        gtk_widget_show_all(dialog);
        
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
            int new_id = atoi(gtk_entry_get_text(GTK_ENTRY(id_entry)));
            const char *new_name = gtk_entry_get_text(GTK_ENTRY(name_entry));
            const char *ip = gtk_entry_get_text(data->ip_entry);
            
            if (strcmp(ip, "localhost") == 0) {
                // ถ้าเป็น localhost ให้แก้ไขไฟล์ JSON โดยตรง
                CameraPresetList *list = camera_import_presets("mock_presets_localhost.json");
                if (list) {
                    // หา preset ที่ต้องการแก้ไข
                    for (int i = 0; i < list->count; i++) {
                        if (list->presets[i].id == id) {
                            list->presets[i].id = new_id;
                            strncpy(list->presets[i].name, new_name, sizeof(list->presets[i].name) - 1);
                            list->presets[i].name[sizeof(list->presets[i].name) - 1] = '\0';
                            break;
                        }
                    }
                    // บันทึกกลับไปที่ไฟล์
                    camera_export_presets("mock_presets_localhost.json", list);
                    camera_free_preset_list(list);
                    // รีเฟรชข้อมูล
                    on_refresh_clicked(button, user_data);
                }
            } else {
                // ถ้าเป็น IP จริง ให้ใช้ CGI
                if (camera_set_preset(ip,
                                   gtk_entry_get_text(data->user_entry),
                                   gtk_entry_get_text(data->pass_entry),
                                   new_id, new_name)) {
                    on_refresh_clicked(button, user_data);
                }
            }
        }
        
        g_free(name);
        g_free(pan);
        g_free(tilt);
        g_free(zoom);
        g_free(focus);
        gtk_widget_destroy(dialog);
    }
}

static void on_import_clicked(GtkButton *button, gpointer user_data) {
    AppData *data = (AppData *)user_data;
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Import Presets",
                                                   GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))),
                                                   GTK_FILE_CHOOSER_ACTION_OPEN,
                                                   "Cancel", GTK_RESPONSE_CANCEL,
                                                   "Open", GTK_RESPONSE_ACCEPT,
                                                   NULL);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        CameraPresetList *list = camera_import_presets(filename);
        if (list) {
            gtk_list_store_clear(data->store);
            
            for (int i = 0; i < list->count; ++i) {
                GtkTreeIter iter;
                gtk_list_store_append(data->store, &iter);
                gtk_list_store_set(data->store, &iter, 
                                  0, list->presets[i].id,
                                  1, list->presets[i].name,
                                  2, list->presets[i].pan,
                                  3, list->presets[i].tilt,
                                  4, list->presets[i].zoom,
                                  5, list->presets[i].focus,
                                  -1);
            }
            camera_free_preset_list(list);
        }
        g_free(filename);
    }
    
    gtk_widget_destroy(dialog);
}

static void on_export_clicked(GtkButton *button, gpointer user_data) {
    AppData *data = (AppData *)user_data;
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Export Presets",
                                                   GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))),
                                                   GTK_FILE_CHOOSER_ACTION_SAVE,
                                                   "Cancel", GTK_RESPONSE_CANCEL,
                                                   "Save", GTK_RESPONSE_ACCEPT,
                                                   NULL);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        
        // สร้าง CameraPresetList จากข้อมูลใน tree view
        CameraPresetList *list = malloc(sizeof(CameraPresetList));
        list->count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(data->store), NULL);
        list->presets = calloc(list->count, sizeof(CameraPreset));
        
        GtkTreeIter iter;
        gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(data->store), &iter);
        int i = 0;
        
        while (valid && i < list->count) {
            gchar *name, *pan, *tilt, *zoom, *focus;
            gtk_tree_model_get(GTK_TREE_MODEL(data->store), &iter,
                              0, &list->presets[i].id,
                              1, &name,
                              2, &pan,
                              3, &tilt,
                              4, &zoom,
                              5, &focus,
                              -1);
            
            // คัดลอกข้อมูลอย่างปลอดภัย
            if (name) {
                strncpy(list->presets[i].name, name, sizeof(list->presets[i].name) - 1);
                list->presets[i].name[sizeof(list->presets[i].name) - 1] = '\0';
                g_free(name);
            } else {
                list->presets[i].name[0] = '\0';
            }
            
            if (pan) {
                strncpy(list->presets[i].pan, pan, sizeof(list->presets[i].pan) - 1);
                list->presets[i].pan[sizeof(list->presets[i].pan) - 1] = '\0';
                g_free(pan);
            } else {
                list->presets[i].pan[0] = '\0';
            }
            
            if (tilt) {
                strncpy(list->presets[i].tilt, tilt, sizeof(list->presets[i].tilt) - 1);
                list->presets[i].tilt[sizeof(list->presets[i].tilt) - 1] = '\0';
                g_free(tilt);
            } else {
                list->presets[i].tilt[0] = '\0';
            }
            
            if (zoom) {
                strncpy(list->presets[i].zoom, zoom, sizeof(list->presets[i].zoom) - 1);
                list->presets[i].zoom[sizeof(list->presets[i].zoom) - 1] = '\0';
                g_free(zoom);
            } else {
                list->presets[i].zoom[0] = '\0';
            }
            
            if (focus) {
                strncpy(list->presets[i].focus, focus, sizeof(list->presets[i].focus) - 1);
                list->presets[i].focus[sizeof(list->presets[i].focus) - 1] = '\0';
                g_free(focus);
            } else {
                list->presets[i].focus[0] = '\0';
            }
            
            valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(data->store), &iter);
            i++;
        }
        
        camera_export_presets(filename, list);
        camera_free_preset_list(list);
        g_free(filename);
    }
    
    gtk_widget_destroy(dialog);
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Camera Preset Controller");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    // Set window icon
    GError *error = NULL;
    GdkPixbuf *icon = gdk_pixbuf_new_from_file("phila-app-icon_256.png", &error);
    if (icon) {
        gtk_window_set_icon(GTK_WINDOW(window), icon);
        g_object_unref(icon);
    } else if (error) {
        g_warning("Could not load icon: %s", error->message);
        g_error_free(error);
    }

    // Main container
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), main_box);
    gtk_container_set_border_width(GTK_CONTAINER(main_box), 10);

    // Login container
    GtkWidget *login_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(main_box), login_box, FALSE, FALSE, 0);

    GtkWidget *ip_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(ip_entry), "Camera IP");
    gtk_box_pack_start(GTK_BOX(login_box), ip_entry, TRUE, TRUE, 0);

    GtkWidget *user_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(user_entry), "Username");
    gtk_box_pack_start(GTK_BOX(login_box), user_entry, TRUE, TRUE, 0);

    GtkWidget *pass_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(pass_entry), "Password");
    gtk_entry_set_visibility(GTK_ENTRY(pass_entry), FALSE);
    gtk_box_pack_start(GTK_BOX(login_box), pass_entry, TRUE, TRUE, 0);

    GtkWidget *refresh_btn = gtk_button_new_with_label("Connect");
    gtk_box_pack_start(GTK_BOX(login_box), refresh_btn, FALSE, FALSE, 0);

    // Action buttons container
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(main_box), button_box, FALSE, FALSE, 0);

    GtkWidget *edit_btn = gtk_button_new_with_label("Edit");
    GtkWidget *import_btn = gtk_button_new_with_label("Import");
    GtkWidget *export_btn = gtk_button_new_with_label("Export");
    gtk_box_pack_start(GTK_BOX(button_box), edit_btn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), import_btn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), export_btn, TRUE, TRUE, 0);

    // Create list store and tree view
    GtkListStore *store = gtk_list_store_new(6, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, 
                                            G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    GtkWidget *tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    
    // Add columns
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("ID", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Pan", renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Tilt", renderer, "text", 3, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Zoom", renderer, "text", 4, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Focus", renderer, "text", 5, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

    // Add tree view to scrolled window
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), tree_view);
    gtk_box_pack_start(GTK_BOX(main_box), scrolled_window, TRUE, TRUE, 0);

    // Create and initialize AppData
    AppData *app_data = g_new(AppData, 1);
    app_data->ip_entry = GTK_ENTRY(ip_entry);
    app_data->user_entry = GTK_ENTRY(user_entry);
    app_data->pass_entry = GTK_ENTRY(pass_entry);
    app_data->store = store;
    app_data->tree_view = GTK_TREE_VIEW(tree_view);

    // Connect signals
    g_signal_connect(refresh_btn, "clicked", G_CALLBACK(on_refresh_clicked), app_data);
    g_signal_connect(edit_btn, "clicked", G_CALLBACK(on_edit_clicked), app_data);
    g_signal_connect(import_btn, "clicked", G_CALLBACK(on_import_clicked), app_data);
    g_signal_connect(export_btn, "clicked", G_CALLBACK(on_export_clicked), app_data);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);
    gtk_main();

    // Free AppData
    g_free(app_data);
    return 0;
}