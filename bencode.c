
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
    me->stk = calloc(1, sizeof(bencode_frame_t) * expected_depth);
    return me;
}

void bencode_init(bencode_t* me)
{
    memset(me,0,sizeof(bencode_t));
}

static bencode_frame_t* __push_stack(bencode_t* me)
{
    me->d++;
    if (me->nframes <= me->d)
        return NULL;
    memset(&me->stk[me->d], 0, sizeof(bencode_frame_t));
    return &me->stk[me->d];
}

static bencode_frame_t* __pop_stack(bencode_t* me)
{
    if (me->d <= 0)
        return NULL;
    me->d--;
    return &me->stk[me->d];
}

static int __parse_digit(const char c, int pos)
{
    return (c - '0') * pow(10, pos);
}

int __process_tok(
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
        /* end of list */
        case 'e':
            __pop_stack(me);
            break;
        }
    case BENCODE_TOK_NONE:
        switch (**buf)
        {
        case 'i':
            f = __push_stack(me);
            f->type = BENCODE_TOK_INT;
            f->pos = 0;
            break;
        case 'd':
            f = __push_stack(me);
            f->type = BENCODE_TOK_DICT_KEYLEN;
            f->pos = 0;
            break;
        case 'l':
            f = __push_stack(me);
            f->type = BENCODE_TOK_LIST;
            f->pos = 0;
            break;

        case 'e':
            f = __push_stack(me);
            f->type = BENCODE_TOK_DICT_KEYLEN;
            f->pos = 0;
            break;
        default:
            if (isdigit(**buf))
            {
                f = __push_stack(me);
                f->type = BENCODE_TOK_STR_LEN;
                f->pos = 0;
                f->len += __parse_digit(**buf, f->pos++);
            }
            else
            {
                printf("%c\n", **buf);
                assert(0);
            }
            break;
        }
        break;

    case BENCODE_TOK_INT:
        if ('e' == **buf)
        {
            // OUTPUT INT
            // POP stack
            f->type = BENCODE_TOK_NONE;
        }
        else if (isdigit(**buf))
        {
            f->intval += __parse_digit(**buf, f->pos++);
        }
        else
        {
            assert(0);
        }
        break;

    case BENCODE_TOK_STR_LEN:
        if (':' == **buf)
        {
            f->type = BENCODE_TOK_STR;
            f->pos = 0;
        }
        else if (isdigit(**buf))
        {
            f->len += __parse_digit(**buf, f->pos++);
        }
        else
        {
            assert(0);
        }

        break;

    case BENCODE_TOK_STR:
        if (f->len == f->pos)
        {

        }
        if (':' == **buf)
        {
            f->type = BENCODE_TOK_STR;
            f->pos = 0;
        }
        else if (isdigit(**buf))
        {
            f->len += __parse_digit(**buf, f->pos++);
        }
        else
        {
            assert(0);
        }

        break;
    case BENCODE_TOK_DICT_KEYLEN:
        if (':' == **buf)
        {
            f->type = BENCODE_TOK_DICT_KEY;
            f->pos = 0;
        }
        else if (isdigit(**buf))
        {
            f->len += __parse_digit(**buf, f->pos++);
        }
        else if ('e' == **buf)
        {
            f = __pop_stack(me);
        }
        else
        {
            assert(0);
        }

        break;
    case BENCODE_TOK_DICT_KEY:
        if (f->pos == f->len)
        {
            f->key[f->pos] = '\0';
            printf("END DICT KEY %s\n", f->key);
            f = __push_stack(me);
        }
        else
        {
            f->key[f->pos] = **buf; 
        }
        break;

    default: assert(0); break;
    }

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
        __process_tok(me,&buf,&len);

    return 0;
}

void bencode_set_callbacks(
        bencode_t* me,
        bencode_callbacks_t* cb)
{
     memcpy(&me->cb,cb,sizeof(bencode_callbacks_t));
}

