#include "usb_mx_id.h"

#include "XLinkPublicDefines.h"

#if (defined(_WIN32) || defined(_WIN64) )
#include "win_time.h"
#else
#include "time.h"
#endif

#include "string.h"

static double steady_seconds()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static double seconds()
{
    static double s;
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    if (!s)
        s = ts.tv_sec + ts.tv_nsec * 1e-9;
    return ts.tv_sec + ts.tv_nsec * 1e-9 - s;
}



#define ADDRESS_BUFF_SIZE 35

// Commands and executable to read serial number from unbooted MX device
static const uint8_t mxid_read_cmd[] = {
    0x4d, 0x41, 0x32, 0x78, 0x8a, 0x00, 0x00, 0x20, 0x70, 0x48, 0x00, 0x00, 0x0c, 0x08, 0x03, 0x64,
    0x61, 0x10, 0x84, 0x6c, 0x61, 0x10, 0x82, 0x00, 0x80, 0x00, 0xc4, 0x00, 0x40, 0x00, 0xc6, 0x03,
    0xa0, 0x08, 0x84, 0x00, 0x40, 0x00, 0xc6, 0x03, 0x80, 0x88, 0x80, 0xfe, 0xff, 0xbf, 0x12, 0x00,
    0x00, 0x00, 0x01, 0x08, 0xe0, 0xc3, 0x81, 0x00, 0x00, 0x00, 0x01, 0xa0, 0xbf, 0xe3, 0x9d, 0x00,
    0x0c, 0x08, 0x3b, 0x68, 0x61, 0x17, 0xb8, 0x00, 0x00, 0x27, 0xc0, 0xf0, 0xff, 0xff, 0x7f, 0x00,
    0x00, 0x00, 0x01, 0x70, 0x61, 0x17, 0x82, 0x64, 0x61, 0x17, 0xba, 0x00, 0x40, 0x00, 0xc2, 0x28,
    0x00, 0x00, 0x03, 0x00, 0x00, 0x27, 0xc2, 0xe9, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x40, 0x27, 0xc0, 0x08, 0xe0, 0xc7, 0x81, 0x00, 0x00, 0xe8, 0x81, 0xa0, 0xbf, 0xe3, 0x9d, 0x00,
    0x0c, 0x08, 0x3b, 0x64, 0x61, 0x17, 0x82, 0x68, 0x61, 0x17, 0xb8, 0x03, 0x20, 0x10, 0x84, 0x04,
    0x00, 0x00, 0x37, 0x00, 0x40, 0x20, 0xc4, 0xdd, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x27, 0xf6, 0xda, 0xff, 0xff, 0x7f, 0x10, 0xe0, 0x16, 0xb6, 0x00, 0x00, 0x27, 0xf6, 0xd7,
    0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x01, 0x08, 0x20, 0x10, 0x82, 0x00, 0x00, 0x27, 0xc2, 0xd3,
    0xff, 0xff, 0x7f, 0x70, 0x61, 0x17, 0xba, 0x00, 0x40, 0x07, 0xc2, 0x00, 0x00, 0x26, 0xc2, 0x09,
    0x20, 0x10, 0x82, 0x00, 0x00, 0x27, 0xc2, 0xcd, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x40, 0x07, 0xc2, 0x04, 0x20, 0x26, 0xc2, 0x0a, 0x20, 0x10, 0x82, 0x00, 0x00, 0x27, 0xc2, 0xc7,
    0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x01, 0x00, 0x40, 0x07, 0xc2, 0x08, 0x20, 0x26, 0xc2, 0xcf,
    0xff, 0xff, 0x7f, 0x00, 0x00, 0xe8, 0x81, 0xa0, 0xbf, 0xe3, 0x9d, 0x00, 0x00, 0x1c, 0x11, 0xdb,
    0xff, 0xff, 0x7f, 0x00, 0x20, 0x10, 0xb0, 0x08, 0xe0, 0xc7, 0x81, 0x00, 0x00, 0xe8, 0x81, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xba, 0xfc, 0x00, 0x20, 0x70,
    0x84, 0x00, 0x00, 0x00, 0x70, 0x09, 0x00,
};

const uint8_t* usb_mx_id_get_payload() {
    return mxid_read_cmd;
}

int usb_mx_id_get_payload_size() {
    return sizeof(mxid_read_cmd);
}


// list of devices for which the mx_id was already resolved (with small timeout)
// Some consts
#define MX_ID_LIST_SIZE 16
const double LIST_ENTRY_TIMEOUT_SEC = 0.5;
typedef struct {
    char mx_id[XLINK_MAX_MX_ID_SIZE];
    char compat_name[ADDRESS_BUFF_SIZE];
    double timestamp;
} MxIdListEntry;
static MxIdListEntry list_mx_id[MX_ID_LIST_SIZE] = { 0 };
static bool list_initialized = false;

static bool list_mx_id_is_entry_valid(MxIdListEntry* entry) {
    if (entry == NULL) return false;
    if (entry->compat_name[0] == 0 || steady_seconds() - entry->timestamp >= LIST_ENTRY_TIMEOUT_SEC) return false;

    // otherwise entry ok
    return true;
}

void usb_mx_id_cache_init() {
    // initialize list
    if (!list_initialized) {
        for (int i = 0; i < MX_ID_LIST_SIZE; i++) {
            list_mx_id[i].timestamp = 0;
            list_mx_id[i].mx_id[0] = 0;
            list_mx_id[i].compat_name[0] = 0;
        }
        list_initialized = true;
    }
}

int usb_mx_id_cache_store_entry(const char* mx_id, const char* compat_addr) {
    for (int i = 0; i < MX_ID_LIST_SIZE; i++) {
        // If entry an invalid (timedout - default)
        if (!list_mx_id_is_entry_valid(&list_mx_id[i])) {
            strncpy(list_mx_id[i].mx_id, mx_id, sizeof(list_mx_id[i].mx_id));
            strncpy(list_mx_id[i].compat_name, compat_addr, ADDRESS_BUFF_SIZE);
            list_mx_id[i].timestamp = steady_seconds();
            return i;
        }
    }
    return -1;
}

bool usb_mx_id_cache_get_entry(const char* compat_addr, char* mx_id) {
    for (int i = 0; i < MX_ID_LIST_SIZE; i++) {
        // If entry still valid
        if (list_mx_id_is_entry_valid(&list_mx_id[i])) {
            // if entry compat name matches
            if (strncmp(compat_addr, list_mx_id[i].compat_name, ADDRESS_BUFF_SIZE) == 0) {
                // copy stored mx_id 
                strncpy(mx_id, list_mx_id[i].mx_id, sizeof(list_mx_id[i].mx_id));
                return true;
            }
        }
    }
    return false;
}

