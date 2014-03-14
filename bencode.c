
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
        break;
    case BENCODE_TOK_NONE:
        switch (**buf)
        {

        /* starting an integer */
        case 'i':
            f = __push_stack(me);
            f->type = BENCODE_TOK_INT;
            f->pos = 0;
            break;

        /* starting a dictionary */
        case 'd':
            f = __push_stack(me);
            f->type = BENCODE_TOK_DICT_KEYLEN;
            f->pos = 0;
            if (me->cb.dict_enter)
                me->cb.dict_enter(me, NULL);
            break;

        /* starting a list */
        case 'l':
            f = __push_stack(me);
            f->type = BENCODE_TOK_LIST;
            f->pos = 0;
            if (me->cb.list_enter)
                me->cb.list_enter(me, NULL);
            break;

        case 'e':
            f = __push_stack(me);
            f->type = BENCODE_TOK_DICT_KEYLEN;
            f->pos = 0;
            break;

        /* calculating length of string */
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
                printf("bad string\n");
                return 0;
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
            printf("integer: %d\n", (int)f->intval);
            me->cb.hit_int(me, NULL, f->intval);
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
            printf("showing\n");
            me->cb.hit_str(me, NULL, f->len, (const unsigned char*)f->strval, 0);
        }
        else
        {
            /* resize string */
            if (f->sv_size < f->pos)
            {
                f->sv_size *= 2;
                f->strval = realloc(f->strval,f->sv_size);
            }
            f->strval[f->pos] = **buf;
            f->pos += 1;
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
        if (f->k_size < f->pos)
        {
            f->k_size *= 2;
            f->key = realloc(f->key,f->k_size);
        }

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
    {
        switch(__process_tok(me, &buf, &len))
        {
            case 0:
                return 0;
                break;
        }
    }

    return 0;
}

void bencode_set_callbacks(
        bencode_t* me,
        bencode_callbacks_t* cb)
{
     memcpy(&me->cb,cb,sizeof(bencode_callbacks_t));
}

