#ifndef BENCODE_SAX_H
#define BENCODE_SAX_H

typedef struct {

    /**
     * @param dict_key The dictionary key for this item.
     *        This is set to null for list entries
     * @param val The integer value
     * @return 0 on error; otherwise 1
     */
    int hit_int(bencode_sax_t *s,
            const char *dict_key,
            const long int val);

    /**
     * @param dict_key The dictionary key for this item.
     *        This is set to null for list entries
     * @param val The integer value
     * @return 0 on error; otherwise 1
     */
    int hit_str(bencode_sax_t *s,
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
    int dict_enter(bencode_sax_t *s,
            const char *dict_key);
    /**
     * @param dict_key The dictionary key for this item.
     *        This is set to null for list entries
     * @param val The integer value
     * @return 0 on error; otherwise 1
     */
    int dict_leave(bencode_sax_t *s,
            const char *dict_key);
    /**
     * @param dict_key The dictionary key for this item.
     *        This is set to null for list entries
     * @param val The integer value
     * @return 0 on error; otherwise 1
     */
    int list_enter(bencode_sax_t *s,
            const char *dict_key);
    /**
     * @param dict_key The dictionary key for this item.
     *        This is set to null for list entries
     * @param val The integer value
     * @return 0 on error; otherwise 1
     */
    int list_leave(bencode_sax_t *s,
            const char *dict_key)
    /**
     * @param val The integer value
     * @return 0 on error; otherwise 1
     */
    int list_next(bencode_sax_t *s);

} bencode_sax_callbacks_t;

typedef struct {
    char* dictkey;

    /* user data for context specific to frame */
    void* udata;
} bencode_sax_frame_t;

typedef struct {
    /* stack */
    bencode_sax_frame_t* stk;

    /* number of frames we can push down, ie. maximum depth */
    unsigned int nframes;

    /* current depth within stack */
    unsigned int d;

    /* user data for context */
    void* udata;
} bencode_sax_t;


/**
 * @param expected_depth The expected depth of the bencode
 * @param cb The callbacks we need to parse the bencode
 * @return new memory for a bencode sax parser
 */
bencode_sax_t* bencode_sax_new(
        int expected_depth,
        bencode_sax_callbacks_t* cb);

/**
 * Initialise reader
 */
void bencode_sax_init(bencode_sax_t*);

/**
 * @param buf The buffer to read new input from
 * @param len The size of the buffer
 * @return 0 on error; otherwise 1
 */
int bencode_sax_dispatch_from_buffer(
        bencode_sax_t*,
        const char* buf,
        unsigned int len);
/**
 * @param cb The callbacks we need to parse the bencode
 */
void bencode_sax_set_callbacks(
        bencode_sax_t*,
        bencode_sax_callbacks_t* cb);


#endif /* BENCODE_SAX_H */
