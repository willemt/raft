
typedef struct
{
    uint32_t piece_idx;
    uint32_t block_byte_offset;
    uint32_t block_len;
} bt_block_t;

typedef void *(*func_getpiece_f)( void *udata, unsigned int piece);

typedef void (*func_write_block_to_stream_f)(
    void* pce, bt_block_t * blk, unsigned char ** msg);

#ifndef HAVE_FUNC_LOG
#define HAVE_FUNC_LOG
typedef void (
    *func_log_f
)    (
    void *udata,
    void *src,
//    bt_peer_t * peer,
    const char *buf,
    ...
);
#endif

typedef int (
    *func_pollblock_f
)   (
    void *udata,
    void * peers_bitfield,
    bt_block_t * blk
);

typedef int (
    *func_pushblock_f
)   (
    void *udata,
    void * peer,
    bt_block_t * block,
    const void *data
);

typedef int (
    *func_send_f
)   (
    void *udata,
    const void * peer,
    const void *send_data,
    const int len
);

typedef int (
    *func_disconnect_f
)   (
    void *udata,
    void * peer,
    char *reason
);

#ifndef HAVE_FUNC_GET_INT
#define HAVE_FUNC_GET_INT
typedef int (
    *func_get_int_f
)   (
    void *,
    void *pr
);
#endif

#define PC_NONE ((unsigned int)0)
#define PC_HANDSHAKE_SENT ((unsigned int)1<<0)
#define PC_HANDSHAKE_RECEIVED ((unsigned int)1<<1)
#define PC_DISCONNECTED ((unsigned int)1<<2)
#define PC_BITFIELD_RECEIVED ((unsigned int)1<<3)
/*  connected to peer */
#define PC_CONNECTED ((unsigned int)1<<4)
/*  we can't communicate with the peer */
#define PC_UNCONTACTABLE_PEER ((unsigned int)1<<5)
#define PC_IM_CHOKING ((unsigned int)1<<6)
#define PC_IM_INTERESTED ((unsigned int)1<<7)
#define PC_PEER_CHOKING ((unsigned int)1<<8)
#define PC_PEER_INTERESTED ((unsigned int)1<<9)

typedef enum
{
    PWP_MSGTYPE_CHOKE = 0,
    PWP_MSGTYPE_UNCHOKE = 1,
    PWP_MSGTYPE_INTERESTED = 2,
    PWP_MSGTYPE_UNINTERESTED = 3,
    PWP_MSGTYPE_HAVE = 4,
    PWP_MSGTYPE_BITFIELD = 5,
    PWP_MSGTYPE_REQUEST = 6,
    PWP_MSGTYPE_PIECE = 7,
    PWP_MSGTYPE_CANCEL = 8,
} pwp_msg_type_e;

/*  peer wire protocol configuration */
typedef struct
{
    int max_pending_requests;
} bt_pwp_cfg_t;

void *pwp_conn_get_peer(void *pco);

void *pwp_conn_new();

void pwp_conn_set_active(void *pco, int opt);

int pwp_conn_peer_is_interested(void *pco);

int pwp_conn_is_active(void *pco);

void pwp_conn_set_my_peer_id(void *pco, const char *peer_id);

void pwp_conn_set_their_peer_id(void *pco, const char *peer_id);

void pwp_conn_set_infohash(void *pco, const char *infohash);

void pwp_conn_set_peer(void *pco, void * peer);

int pwp_conn_peer_is_interested(void *pco);

int pwp_conn_peer_is_choked(void *pco);

int pwp_conn_im_choked(void *pco);

int pwp_conn_im_interested(void *pco);

void pwp_conn_choke(void * pc);

void pwp_conn_unchoke(void * pco);

int pwp_conn_get_download_rate(const void * pco);

int pwp_conn_get_upload_rate(const void * pco);

int pwp_conn_send_statechange(void * pco, const unsigned char msg_type);

void pwp_conn_send_piece(void *pco, bt_block_t * req);

int pwp_conn_send_have(void *pco, const int piece_idx);

void pwp_conn_send_request(void *pco, const bt_block_t * request);

void pwp_conn_send_cancel(void *pco, bt_block_t * cancel);

void pwp_conn_send_bitfield(void *pco);

int pwp_conn_recv_handshake(void *pco, const char *info_hash);

int pwp_conn_send_handshake(void *pco);

void pwp_conn_set_piece_info(void *pco, int num_pieces, int piece_len);

void pwp_conn_set_state(void *pco, const int state);

int pwp_conn_get_state(void *pco);

int pwp_conn_mark_peer_has_piece(void *pco, const int piece_idx);

int pwp_conn_process_request(void * pco, bt_block_t * request);

int pwp_conn_process_msg(void *pco);

int pwp_conn_get_npending_requests(const void * pco);

int pwp_conn_get_npending_peer_requests(const void* pco);

void pwp_conn_request_block_from_peer(void * pco, bt_block_t * blk);

void pwp_conn_step(void *pco);

int pwp_conn_peer_has_piece(void *pco, const int piece_idx);

typedef struct {
    /** send data to peer */
    func_send_f send;

    /* drop the connect.
     * Most likely because we detected an error with the peer's processing */
    func_disconnect_f disconnect;

    /* manage piece related operations */
    func_write_block_to_stream_f write_block_to_stream;
    func_get_int_f piece_is_complete;
    func_getpiece_f getpiece;

    /**
     * Ask our caller if they have an idea of what block they would like.
     * We're able to request a block from the peer now.
     *
     * @return 0 on success; otherwise -1 on failure*/
    func_pollblock_f pollblock;

    /* We've just downloaded the block and want to allocate it. */
    func_pushblock_f pushblock;

    /* logging */
    func_log_f log;
} pwp_connection_functions_t;

typedef struct {
    bt_block_t block;
    const void* data;
} msg_piece_t;

typedef struct {
    uint32_t piece_idx;
} msg_have_t;

typedef struct {
   bitfield_t bf;
} msg_bitfield_t;

void pwp_conn_choke_peer(void * pco);

void pwp_conn_unchoke_peer(void * pco);

void pwp_conn_keepalive(void* pco);

void pwp_conn_choke(void* pco);

void pwp_conn_unchoke(void* pco);

void pwp_conn_interested(void* pco);

void pwp_conn_uninterested(void* pco);

void pwp_conn_have(void* pco, msg_have_t* have);

void pwp_conn_bitfield(void* pco, msg_bitfield_t* bitfield);

int pwp_conn_request(void* pco, bt_block_t *request);

void pwp_conn_cancel(void* pco, bt_block_t *cancel);

int pwp_conn_piece(void* pco, msg_piece_t *piece);

void pwp_conn_set_functions(void *pco, pwp_connection_functions_t* funcs, void* caller);

int pwp_conn_flag_is_set(void *pco, const int flag);

void pwp_conn_connected(void* pco);

void pwp_conn_connect_failed(void *pco);

