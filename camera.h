#ifndef CAMERA_H
#define CAMERA_H

typedef struct {
    int id;
    char name[33];
    char pan[32];
    char tilt[32];
    char zoom[32];
    char focus[32];
} CameraPreset;

typedef struct {
    CameraPreset *presets;
    int count;
} CameraPresetList;

// ฟังก์ชันสำหรับดึงข้อมูล preset ทั้งหมด
CameraPresetList* camera_get_presets(const char *ip, const char *user, const char *pass);

// ฟังก์ชันสำหรับคัดลอก preset
int camera_clone_preset(const char *ip, const char *user, const char *pass, int from_id, int to_id);

// ฟังก์ชันสำหรับตั้งค่า preset ใหม่
int camera_set_preset(const char *ip, const char *user, const char *pass, int id, const char *name);

// ฟังก์ชันสำหรับลบ preset
int camera_clear_preset(const char *ip, const char *user, const char *pass, int id);

// ฟังก์ชันสำหรับเรียกใช้ preset
int camera_call_preset(const char *ip, const char *user, const char *pass, int id, int speed);

// ฟังก์ชันสำหรับนำเข้า/ส่งออก preset
void camera_export_presets(const char *filename, CameraPresetList *list);
CameraPresetList* camera_import_presets(const char *filename);

// ฟังก์ชันสำหรับคืนหน่วยความจำ
void camera_free_preset_list(CameraPresetList *list);

#endif
