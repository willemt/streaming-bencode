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
    /* an integer */
    BENCODE_TOK_INT,
    /* the length of the string */
    BENCODE_TOK_STR_LEN,
    /* string */
    BENCODE_TOK_STR
}; 

typedef struct {

    /**
     * @param dict_key The dictionary key for this item.
     *        This is set to null for list entries
     * @param val The integer value
     * @return 0 on error; otherwise 1
     */
    int hit_int(bencode_t *s,
            const char *dict_key,
            const long int val);

    /**
     * @param dict_key The dictionary key for this item.
     *        This is set to null for list entries
     * @param val The integer value
     * @return 0 on error; otherwise 1
     */
    int hit_str(bencode_t *s,
        const char *dict_key,
        unsigned int val_len,
        const unsigned char* val,
        unsigned int len);

    /**
     * @param dict_key The dictionary key for this item.
     *        This is set to null for list entries
     * @param val The integer value
     * @return 0 on error; otherwise 1
     */
    int dict_enter(bencode_t *s,
            const char *dict_key);
    /**
     * @param dict_key The dictionary key for this item.
     *        This is set to null for list entries
     * @param val The integer value
     * @return 0 on error; otherwise 1
     */
    int dict_leave(bencode_t *s,
            const char *dict_key);
    /**
     * @param dict_key The dictionary key for this item.
     *        This is set to null for list entries
     * @param val The integer value
     * @return 0 on error; otherwise 1
     */
    int list_enter(bencode_t *s,
            const char *dict_key);
    /**
     * @param dict_key The dictionary key for this item.
     *        This is set to null for list entries
     * @param val The integer value
     * @return 0 on error; otherwise 1
     */
    int list_leave(bencode_t *s,
            const char *dict_key)
    /**
     * @param val The integer value
     * @return 0 on error; otherwise 1
     */
    int list_next(bencode_t *s);

} bencode_callbacks_t;

typedef struct {

    /* dict key */
    char* key;

    /* length of key buffer */
    int keylen;

    long int intval;

//    int len;

    int pos;

    /* token type */
    int type;

    /* user data for context specific to frame */
    void* udata;

} bencode_frame_t;

typedef struct {
    /* stack */
    bencode_frame_t* stk;

    /* number of frames we can push down, ie. maximum depth */
    unsigned int nframes;

    /* current depth within stack */
    unsigned int d;

    /* user data for context */
    void* udata;

    bencode_callbacks_t cb;

} bencode_t;


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
