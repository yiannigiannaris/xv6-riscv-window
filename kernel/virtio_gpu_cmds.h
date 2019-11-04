//Commands to interact with virtio gpu as specified in virtio documentation
enum virtio_gpu_ctrl_type {

        /* 2d commands */
        VIRTIO_GPU_CMD_GET_DISPLAY_INFO = 0x0100,
        VIRTIO_GPU_CMD_RESOURCE_CREATE_2D,
        VIRTIO_GPU_CMD_RESOURCE_UNREF,
        VIRTIO_GPU_CMD_SET_SCANOUT,
        VIRTIO_GPU_CMD_RESOURCE_FLUSH,
        VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D,
        VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING,
        VIRTIO_GPU_CMD_RESOURCE_DETACH_BACKING,
        VIRTIO_GPU_CMD_GET_CAPSET_INFO,
        VIRTIO_GPU_CMD_GET_CAPSET,
        VIRTIO_GPU_CMD_GET_EDID,

        /* cursor commands */
        VIRTIO_GPU_CMD_UPDATE_CURSOR = 0x0300,
        VIRTIO_GPU_CMD_MOVE_CURSOR,

        /* success responses */
        VIRTIO_GPU_RESP_OK_NODATA = 0x1100,
        VIRTIO_GPU_RESP_OK_DISPLAY_INFO,
        VIRTIO_GPU_RESP_OK_CAPSET_INFO,
        VIRTIO_GPU_RESP_OK_CAPSET,
        VIRTIO_GPU_RESP_OK_EDID,

        /* error responses */
        VIRTIO_GPU_RESP_ERR_UNSPEC = 0x1200,
        VIRTIO_GPU_RESP_ERR_OUT_OF_MEMORY,
        VIRTIO_GPU_RESP_ERR_INVALID_SCANOUT_ID,
        VIRTIO_GPU_RESP_ERR_INVALID_RESOURCE_ID,
        VIRTIO_GPU_RESP_ERR_INVALID_CONTEXT_ID,
        VIRTIO_GPU_RESP_ERR_INVALID_PARAMETER,
};

#define VIRTIO_GPU_FLAG_FENCE (1 << 0)

struct virtio_gpu_ctrl_hdr {
        uint32 type;
        uint32 flags;
        uint64 fence_id;
        uint32 ctx_id;
        uint32 padding;
};

//VIRTIO_GPU_CMD_GET_DISPLAY_INFO
//use blank virtio_gpu_ctrl_hdr for request
//Response: VIRTIO_GPU_RESP_OK_DISPLAY_INFO
#define VIRTIO_GPU_MAX_SCANOUTS 16 
 
struct virtio_gpu_rect { 
        uint32 x; 
        uint32 y; 
        uint32 width; 
        uint32 height; 
}; 
 
struct virtio_gpu_resp_display_info { 
        struct virtio_gpu_ctrl_hdr hdr; 
        struct virtio_gpu_display_one { 
                struct virtio_gpu_rect r; 
                uint32 enabled; 
                uint32 flags; 
        } pmodes[VIRTIO_GPU_MAX_SCANOUTS]; 
};

//VIRTIO_GPU_CMD_GET_EDID
//Response: VIRTIO_GPU_RESP_OK_EDID
struct virtio_gpu_get_edid { 
        struct virtio_gpu_ctrl_hdr hdr; 
        uint32 scanout; 
        uint32 padding; 
}; 

struct virtio_gpu_resp_edid { 
        struct virtio_gpu_ctrl_hdr hdr; 
        uint32 size; 
        uint32 padding; 
        uint8 edid[1024]; 
};

//VIRTIO_GPU_CMD_RESOURCE_CREATE_2D
//Response: VIRTIO_GPU_RESP_OK_NODATA
enum virtio_gpu_formats { 
        VIRTIO_GPU_FORMAT_B8G8R8A8_UNORM  = 1, 
        VIRTIO_GPU_FORMAT_B8G8R8X8_UNORM  = 2, 
        VIRTIO_GPU_FORMAT_A8R8G8B8_UNORM  = 3, 
        VIRTIO_GPU_FORMAT_X8R8G8B8_UNORM  = 4, 
 
        VIRTIO_GPU_FORMAT_R8G8B8A8_UNORM  = 67, 
        VIRTIO_GPU_FORMAT_X8B8G8R8_UNORM  = 68, 
 
        VIRTIO_GPU_FORMAT_A8B8G8R8_UNORM  = 121, 
        VIRTIO_GPU_FORMAT_R8G8B8X8_UNORM  = 134, 
}; 
 
struct virtio_gpu_resource_create_2d { 
        struct virtio_gpu_ctrl_hdr hdr; 
        uint32 resource_id; 
        uint32 format; 
        uint32 width; 
        uint32 height; 
}; 

//VIRTIO_GPU_CMD_RESOURCE_UNREF
//Response: VIRTIO_GPU_RESP_OK_NODATA
struct virtio_gpu_resource_unref { 
        struct virtio_gpu_ctrl_hdr hdr; 
        uint32 resource_id; 
        uint32 padding; 
};

//VIRTIO_GPU_CMD_SET_SCANOUT
//Response: VIRTIO_GPU_RESP_OK_NODATA
struct virtio_gpu_set_scanout { 
        struct virtio_gpu_ctrl_hdr hdr; 
        struct virtio_gpu_rect r; 
        uint32 scanout_id; 
        uint32 resource_id; 
};

//VIRTIO_GPU_CMD_RESOURCE_FLUSH
//Response: VIRTIO_GPU_RESP_OK_NODATA
struct virtio_gpu_resource_flush { 
        struct virtio_gpu_ctrl_hdr hdr; 
        struct virtio_gpu_rect r; 
        uint32 resource_id; 
        uint32 padding; 
};

//VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D
//Response: VIRTIO_GPU_RESP_OK_NODATA
struct virtio_gpu_transfer_to_host_2d {
        struct virtio_gpu_ctrl_hdr hdr;
        struct virtio_gpu_rect r;
        uint64 offset;
        uint32 resource_id;
        uint32 padding;
};

//VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING
//Response: VIRTIO_GPU_RESP_OK_NODATA
struct virtio_gpu_resource_attach_backing { 
        struct virtio_gpu_ctrl_hdr hdr; 
        uint32 resource_id; 
        uint32 nr_entries; 
}; 
 
struct virtio_gpu_mem_entry { 
        uint64 addr; 
        uint32 length; 
        uint32 padding; 
};

//VIRTIO_GPU_CMD_RESOURCE_DETACH_BACKING
//Response: VIRTIO_GPU_RESP_OK_NODATA
struct virtio_gpu_resource_detach_backing { 
        struct virtio_gpu_ctrl_hdr hdr; 
        uint32 resource_id; 
        uint32 padding; 
};

//VIRTIO_GPU_CMD_UPDATE_CURSOR
//VIRTIO_GPU_CMD_MOVE_CURSOR
//Response: VIRTIO_GPU_RESP_OK_NODATA
struct virtio_gpu_cursor_pos { 
        uint32 scanout_id; 
        uint32 x; 
        uint32 y; 
        uint32 padding; 
}; 
 
struct virtio_gpu_update_cursor { 
        struct virtio_gpu_ctrl_hdr hdr; 
        struct virtio_gpu_cursor_pos pos; 
        uint32 resource_id; 
        uint32 hot_x; 
        uint32 hot_y; 
        uint32 padding; 
};
