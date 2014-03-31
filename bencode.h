#ifndef BENCODE_H
#define BENCODE_H

enum {
    /* init state */
    BENCODE_TOK_NONE,
    /* a list */
    BENCODE_TOK_LIST,
    /* the length of a dictionary key */
    BENCODE_TOK_DICT_KEYLEN,
    /* a dictionary key */
    BENCODE_TOK_DICT_KEY,
    /* a dictionary value */
    BENCODE_TOK_DICT_VAL,
    /* an integer */
    BENCODE_TOK_INT,
    /* the length of the string */
    BENCODE_TOK_STR_LEN,
    /* string */
    BENCODE_TOK_STR,
    BENCODE_TOK_DICT
}; 

typedef struct bencode_s bencode_t;

typedef struct {

    /**
     * @param dict_key The dictionary key for this item.
     *        This is set to null for list entries
     * @param val The integer value
     * @return 0 on error; otherwise 1
     */
    int (*hit_int)(bencode_t *s,
            const char *dict_key,
            const long int val);

    /**
     * Call when there is some string for us to read.
     * This callback could fire multiple times for large strings
     *
     * @param dict_key The dictionary key for this item.
     *        This is set to null for list entries
     * @param val The string value
     * @param v_total_len The total length of the string
     * @param v_len The length of the string we're currently emitting
     * @return 0 on error; otherwise 1
     */
    int (*hit_str)(bencode_t *s,
        const char *dict_key,
        unsigned int v_total_len,
        const unsigned char* val,
        unsigned int v_len);

    /**
     * @param dict_key The dictionary key for this item.
     *        This is set to null for list entries
     * @param val The integer value
     * @return 0 on error; otherwise 1
     */
    int (*dict_enter)(bencode_t *s,
            const char *dict_key);
    /**
     * Called when we have finished processing a dictionary
     * @param dict_key The dictionary key for this item.
     *        This is set to null for list entries
     * @param val The integer value
     * @return 0 on error; otherwise 1
     */
    int (*dict_leave)(bencode_t *s,
            const char *dict_key);
    /**
     * @param dict_key The dictionary key for this item.
     *        This is set to null for list entries
     * @param val The integer value
     * @return 0 on error; otherwise 1
     */
    int (*list_enter)(bencode_t *s,
            const char *dict_key);
    /**
     * @param dict_key The dictionary key for this item.
     *        This is set to null for list entries
     * @param val The integer value
     * @return 0 on error; otherwise 1
     */
    int (*list_leave)(bencode_t *s,
            const char *dict_key);
    /**
     * Called when we have just finished processing a list item
     * @return 0 on error; otherwise 1
     */
    int (*list_next)(bencode_t *s);

    /**
     * Called when we have just finished processing a dict item
     * @return 0 on error; otherwise 1
     */
    int (*dict_next)(bencode_t *s);

} bencode_callbacks_t;

typedef struct {

    /* dict key */
    char* key;
    int k_size;

    char* strval;
    int sv_size;
    /* length of key buffer */
//    int keylen;

    long int intval;

    int len;

    int pos;

    /* token type */
    int type;

    /* user data for context specific to frame */
    void* udata;

} bencode_frame_t;

struct bencode_s {
    /* stack */
    bencode_frame_t* stk;

    /* number of frames we can push down, ie. maximum depth */
    unsigned int nframes;

    /* current depth within stack */
    unsigned int d;

    /* user data for context */
    void* udata;

    bencode_callbacks_t cb;
};


/**
 * @param expected_depth The expected depth of the bencode
 * @param cb The callbacks we need to parse the bencode
 * @return new memory for a bencode sax parser
 */
bencode_t* bencode_new(
        int expected_depth,
        bencode_callbacks_t* cb,
        void* udata);

/**
 * Initialise reader
 */
void bencode_init(bencode_t*);

/**
 * @param buf The buffer to read new input from
 * @param len The size of the buffer
 * @return 0 on error; otherwise 1
 */
int bencode_dispatch_from_buffer(
        bencode_t*,
        const char* buf,
        unsigned int len);
/**
 * @param cb The callbacks we need to parse the bencode
 */
void bencode_set_callbacks(
        bencode_t*,
        bencode_callbacks_t* cb);


#endif /* BENCODE_H */
