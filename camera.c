#include "camera.h"
#include "http_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <json.h>
#include <glib.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif

static char* get_app_path() {
    char *path = NULL;
#ifdef _WIN32
    char exe_path[MAX_PATH];
    GetModuleFileName(NULL, exe_path, MAX_PATH);
    char *last_slash = strrchr(exe_path, '\\');
    if (last_slash) {
        *last_slash = '\0';
        path = g_strdup(exe_path);
    }
#else
    path = g_get_current_dir();
#endif
    return path;
}

CameraPresetList* camera_get_presets(const char *ip, const char *user, const char *pass) {
    if (strcmp(ip, "localhost") == 0) {
        char *app_path = get_app_path();
        if (!app_path) return NULL;

        char *json_path;
#ifdef __APPLE__
        json_path = g_build_filename(app_path, "camera_app.app", "Contents", "Resources", "mock_presets_localhost.json", NULL);
#else
        json_path = g_build_filename(app_path, "mock_presets_localhost.json", NULL);
#endif

        CameraPresetList *list = camera_import_presets(json_path);
        g_free(json_path);
        g_free(app_path);
        return list;
    }

    char url[256];
    snprintf(url, sizeof(url), "http://%s/command/inquiry.cgi?inq=presetposition", ip);

    char *response = http_get(url, user, pass);
    if (!response) return NULL;

    // Parse response ตามรูปแบบ PresetPos=Value11,Value21,Value31,Value41,Value51,Value61
    CameraPresetList *list = malloc(sizeof(CameraPresetList));
    list->presets = calloc(10, sizeof(CameraPreset));
    list->count = 0;

    char *token = strtok(response, "=");
    if (token && strcmp(token, "PresetPos") == 0) {
        token = strtok(NULL, ",");
        while (token && list->count < 10) {
            list->presets[list->count].id = atoi(token);
            
            token = strtok(NULL, ",");
            if (token) strncpy(list->presets[list->count].name, token, 32);
            
            token = strtok(NULL, ",");
            if (token) strncpy(list->presets[list->count].pan, token, 32);
            
            token = strtok(NULL, ",");
            if (token) strncpy(list->presets[list->count].tilt, token, 32);
            
            token = strtok(NULL, ",");
            if (token) strncpy(list->presets[list->count].zoom, token, 32);
            
            token = strtok(NULL, ",");
            if (token) strncpy(list->presets[list->count].focus, token, 32);
            
            list->count++;
            token = strtok(NULL, ",");
        }
    }

    free(response);
    return list;
}

int camera_clone_preset(const char *ip, const char *user, const char *pass, int from_id, int to_id) {
    char url[256];
    snprintf(url, sizeof(url), "http://%s/command/presetposition.cgi?PresetSet=%d,%s,off", 
             ip, to_id, "Copy of Preset");
    
    char *response = http_get(url, user, pass);
    if (!response) return 0;
    
    free(response);
    return 1;
} 

int camera_set_preset(const char *ip, const char *user, const char *pass, int id, const char *name,
                     const char *pan, const char *tilt, const char *zoom, const char *focus) {
    char url[256];
    snprintf(url, sizeof(url), "http://%s/command/presetposition.cgi?PresetSet=%d,%s,%s,%s,%s,%s,off", 
             ip, id, name, pan, tilt, zoom, focus);
    
    char *response = http_get(url, user, pass);
    if (!response) return 0;
    
    free(response);
    return 1;
}

int camera_clear_preset(const char *ip, const char *user, const char *pass, int id) {
    char url[256];
    snprintf(url, sizeof(url), "http://%s/command/presetposition.cgi?PresetClear=%d", ip, id);
    
    char *response = http_get(url, user, pass);
    if (!response) return 0;
    
    free(response);
    return 1;
}

int camera_call_preset(const char *ip, const char *user, const char *pass, int id, int speed) {
    char url[256];
    snprintf(url, sizeof(url), "http://%s/command/presetposition.cgi?PresetCall=%d,%d", 
             ip, id, speed);
    
    char *response = http_get(url, user, pass);
    if (!response) return 0;
    
    free(response);
    return 1;
}

void camera_export_presets(const char *filename, CameraPresetList *list) {
    json_object *jarray = json_object_new_array();
    for (int i = 0; i < list->count; i++) {
        json_object *jpreset = json_object_new_object();
        
        // เพิ่มข้อมูลแบบตรงๆ ไม่ต้องแปลงหรือเข้ารหัส
        json_object_object_add(jpreset, "id", json_object_new_int(list->presets[i].id));
        json_object_object_add(jpreset, "name", json_object_new_string(list->presets[i].name));
        json_object_object_add(jpreset, "pan", json_object_new_string(list->presets[i].pan));
        json_object_object_add(jpreset, "tilt", json_object_new_string(list->presets[i].tilt));
        json_object_object_add(jpreset, "zoom", json_object_new_string(list->presets[i].zoom));
        json_object_object_add(jpreset, "focus", json_object_new_string(list->presets[i].focus));
        
        json_object_array_add(jarray, jpreset);
    }
    
    FILE *f = fopen(filename, "w");
    if (f) {
        fputs(json_object_to_json_string_ext(jarray, JSON_C_TO_STRING_PRETTY), f);
        fclose(f);
    }
    json_object_put(jarray);
}

CameraPresetList* camera_import_presets(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buffer = malloc(len + 1);
    fread(buffer, 1, len, f);
    buffer[len] = '\0';
    fclose(f);

    json_object *jarray = json_tokener_parse(buffer);
    free(buffer);

    if (!jarray || !json_object_is_type(jarray, json_type_array)) return NULL;

    int count = json_object_array_length(jarray);
    CameraPresetList *list = malloc(sizeof(CameraPresetList));
    list->presets = calloc(count, sizeof(CameraPreset));
    list->count = count;

    for (int i = 0; i < count; ++i) {
        json_object *jpreset = json_object_array_get_idx(jarray, i);
        
        // อ่านข้อมูลแบบตรงๆ
        list->presets[i].id = json_object_get_int(json_object_object_get(jpreset, "id"));
        strncpy(list->presets[i].name, json_object_get_string(json_object_object_get(jpreset, "name")), sizeof(list->presets[i].name) - 1);
        strncpy(list->presets[i].pan, json_object_get_string(json_object_object_get(jpreset, "pan")), sizeof(list->presets[i].pan) - 1);
        strncpy(list->presets[i].tilt, json_object_get_string(json_object_object_get(jpreset, "tilt")), sizeof(list->presets[i].tilt) - 1);
        strncpy(list->presets[i].zoom, json_object_get_string(json_object_object_get(jpreset, "zoom")), sizeof(list->presets[i].zoom) - 1);
        strncpy(list->presets[i].focus, json_object_get_string(json_object_object_get(jpreset, "focus")), sizeof(list->presets[i].focus) - 1);
        
        // รับรองว่าทุก string จบด้วย null
        list->presets[i].name[sizeof(list->presets[i].name) - 1] = '\0';
        list->presets[i].pan[sizeof(list->presets[i].pan) - 1] = '\0';
        list->presets[i].tilt[sizeof(list->presets[i].tilt) - 1] = '\0';
        list->presets[i].zoom[sizeof(list->presets[i].zoom) - 1] = '\0';
        list->presets[i].focus[sizeof(list->presets[i].focus) - 1] = '\0';
    }
    
    json_object_put(jarray);
    return list;
}

void camera_free_preset_list(CameraPresetList *list) {
    free(list->presets);
    free(list);
}
