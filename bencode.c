
/**
 * Copyright (c) 2014, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @brief Read bencoded data
 * @author  Willem Thiart himself@willemthiart.com
 * @version 0.1
 */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "bencode.h"

bencode_t* bencode_new(
        int expected_depth,
        bencode_callbacks_t* cb,
        void* udata)
{
    bencode_t* me;

    me = calloc(1, sizeof(bencode_t));
    bencode_set_callbacks(me, cb);
    me->udata = udata;
    me->nframes = expected_depth;
    me->stk = calloc(10 + expected_depth, sizeof(bencode_frame_t));
    return me;
}

void bencode_init(bencode_t* me)
{
    memset(me,0,sizeof(bencode_t));
}

static bencode_frame_t* __push_stack(bencode_t* me)
{
    if (me->nframes <= me->d)
    {
        assert(0);
        return NULL;
    }

    me->d++;
    //memset(&me->stk[me->d], 0, sizeof(bencode_frame_t));

    bencode_frame_t* s = &me->stk[me->d];

    s->pos = 0;
    s->intval = 0;
    s->len = 0;
    s->type = 0;

    if (0 == s->sv_size)
    {
        s->sv_size = 20;
        s->strval = malloc(s->sv_size);
    }

    if (0 == s->k_size)
    {
        s->k_size = 20;
        s->key = malloc(s->k_size);
    }
    
    return &me->stk[me->d];
}

static bencode_frame_t* __pop_stack(bencode_t* me)
{
    bencode_frame_t* f;

    f = &me->stk[me->d];

    switch(f->type)
    {
        case BENCODE_TOK_LIST:
            if (me->cb.list_leave)
                me->cb.list_leave(me, f->key);
            break;
        case BENCODE_TOK_DICT:
            if (me->cb.dict_leave)
                me->cb.dict_leave(me, f->key);
            break;
    }

    if (me->d == 0)
        return NULL;

    f = &me->stk[--me->d];

    switch(f->type)
    {
        case BENCODE_TOK_LIST:
            if (me->cb.list_next)
                me->cb.list_next(me);
            break;
        case BENCODE_TOK_DICT:
            if (me->cb.dict_next)
                me->cb.dict_next(me);
            break;
    }

    return f;
}

static int __parse_digit(const int current_value, const char c)
{
    return (c - '0') + current_value * 10;
}

static void __start_int(bencode_frame_t* f)
{
    f->type = BENCODE_TOK_INT;
    f->pos = 0;
}

static bencode_frame_t* __start_dict(bencode_t* me, bencode_frame_t* f)
{
    f->type = BENCODE_TOK_DICT;
    f->pos = 0;
    if (me->cb.dict_enter)
        me->cb.dict_enter(me, f->key);

    /* key/value */
    f = __push_stack(me);
    f->type = BENCODE_TOK_DICT_KEYLEN;
    return f;
}

static void __start_list(bencode_t* me, bencode_frame_t* f)
{
    f->type = BENCODE_TOK_LIST;
    f->pos = 0;
    if (me->cb.list_enter)
        me->cb.list_enter(me, f->key);
}

static void __start_str(bencode_frame_t* f)
{
    f->type = BENCODE_TOK_STR_LEN;
    f->pos = 0;
}

static int __process_tok(
        bencode_t* me,
        const char** buf,
        unsigned int *len)
{
    bencode_frame_t* f = &me->stk[me->d];

    switch (f->type)
    {
    case BENCODE_TOK_LIST:
        switch (**buf)
        {
        /* end of list/dict */
        case 'e':
            f = __pop_stack(me);
            break;
        case 'i':
            f = __push_stack(me);
            __start_int(f);
            break;
        case 'd':
            f = __push_stack(me);
            f = __start_dict(me,f);
            break;
        case 'l':
            f = __push_stack(me);
            __start_list(me,f);
            break;
        /* calculating length of string */
        default:
            if (isdigit(**buf))
            {
                f = __push_stack(me);
                __start_str(f);
                f->len = __parse_digit(f->len, **buf);
            }
            else
            {
                return 0;
            }
            break;
        }
        break;

    case BENCODE_TOK_DICT_VAL:
        /* drop through */
    case BENCODE_TOK_NONE:
        switch (**buf)
        {
        case 'i':
            __start_int(f);
            break;
        case 'd':
            f = __start_dict(me,f);
            break;
        case 'l':
            __start_list(me,f);
            break;
        /* calculating length of string */
        default:
            if (isdigit(**buf))
            {
                __start_str(f);
                f->len = __parse_digit(f->len, **buf);
            }
            else
            {
                return 0;
            }
            break;

        }
        break;

    case BENCODE_TOK_INT:
        if ('e' == **buf)
        {
            me->cb.hit_int(me, f->key, f->intval);
            f = __pop_stack(me);
        }
        else if (isdigit(**buf))
        {
            f->intval = __parse_digit(f->intval, **buf);
        }
        else
        {
            assert(0);
        }
        break;

    case BENCODE_TOK_STR_LEN:
        if (':' == **buf)
        {
            if (0 == f->len)
            {
                me->cb.hit_str(me, f->key, 0, NULL, 0);
                f = __pop_stack(me);
            }
            else
            {
                f->type = BENCODE_TOK_STR;
                f->pos = 0;
            }
        }
        else if (isdigit(**buf))
        {
            f->len = __parse_digit(f->len, **buf);
        }
        else
        {
            assert(0);
        }

        break;
    case BENCODE_TOK_STR:

        /* resize string
         * +1 incase we also need to count for '\0' terminator */
        if (f->sv_size <= f->pos + 1)
        {
            f->sv_size = 4 + f->sv_size * 2;
            f->strval = realloc(f->strval,f->sv_size);
        }
        f->strval[f->pos++] = **buf;

        if (f->len == f->pos)
        {
            f->strval[f->pos] = 0;
            me->cb.hit_str(me, f->key, f->len,
                    (const unsigned char*)f->strval, f->len);
            f = __pop_stack(me);
        }

        break;
    case BENCODE_TOK_DICT:
        if ('e' == **buf)
        {
            f = __pop_stack(me);
            goto done;
        }

        f = __push_stack(me);
        f->type = BENCODE_TOK_DICT_KEYLEN;
        f->pos = 0;
        /* drop through */
    case BENCODE_TOK_DICT_KEYLEN:
        if (':' == **buf)
        {
            f->type = BENCODE_TOK_DICT_KEY;
            f->pos = 0;
        }
        else if (isdigit(**buf))
        {
            f->len = __parse_digit(f->len, **buf);
        }
        /* end of dictionary */
        else if ('e' == **buf)
        {
            //return 0;
            f = __pop_stack(me);
        }
        else
        {
            return 0;
        }

        break;
    case BENCODE_TOK_DICT_KEY:
        if (f->k_size <= f->pos + 1)
        {
            f->k_size = f->k_size * 2 + 4;
            f->key = realloc(f->key, f->k_size);
        }

        f->key[f->pos++] = **buf; 

        if (f->pos == f->len)
        {
            f->key[f->pos] = '\0';
            //f = __push_stack(me);
            f->type = BENCODE_TOK_DICT_VAL;
            f->pos = 0;
            f->len = 0;
        }
        break;

    default:
        assert(0); break;
    }

done:
    (*buf)++;
    *len -= 1;
    return 1;
}

int bencode_dispatch_from_buffer(
        bencode_t* me,
        const char* buf,
        unsigned int len)
{
    if (me->nframes <= me->d)
        return 0;

    while (0 < len)
    {
        switch(__process_tok(me, &buf, &len))
        {
            case 0:
                return 0;
                break;
        }
    }

    return 1;
}

void bencode_set_callbacks(
        bencode_t* me,
        bencode_callbacks_t* cb)
{
     memcpy(&me->cb,cb,sizeof(bencode_callbacks_t));
}

