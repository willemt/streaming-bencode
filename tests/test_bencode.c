
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

#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "CuTest.h"

#include "bencode.h"

#if WIN32
int asprintf(char **resultp, const char *format, ...)
{
    char buf[1024];
    va_list args;

    va_start(args, format);
    vsprintf(buf, format, args);
    *resultp = strdup(buf);
    va_end(args);
    return 1;
}

char* strndup(const char* str, const unsigned int len)
{
    char* new;

    new = malloc(len+1);
    strncpy(new,str,len);
    new[len] = '\0';
    return new;
}
#endif

enum {
    BENCODE_TYPE_NONE,
    BENCODE_TYPE_INT,
    BENCODE_TYPE_STR,
    BENCODE_TYPE_DICT,
    BENCODE_TYPE_DICTKEY,
    BENCODE_TYPE_LIST,
};

typedef struct node_s node_t;

struct node_s {
    int type;
    int intval;
    char* strval;
    int sv_len;
    char* dictkey;
    node_t* child;
    node_t* parent;
    node_t* next;
    node_t* prev;

    /* for testing (list|dict)_next() callback */
    int list_next_called;
    int dict_next_called;
    int dict_leave_called;
    int list_leave_called;
};

static node_t* __find_sibling_slot(node_t* n, int depth)
{
    assert(n);
    if (0 == depth)
    {
        if (BENCODE_TYPE_NONE == n->type)
            return n;

        int c = 0;
        while (n->next)
        {
            c++;
            n = n->next;
        }
        n->next = calloc(1, sizeof(node_t));
        n->next->prev = n;
        return n->next;
    }
    else
    {
        if (!n->child)
        {
            n->child = calloc(1, sizeof(node_t));
            n->child->parent = n;
        }
        return __find_sibling_slot(n->child, depth - 1);
    }
}

static node_t* __find_child_slot(node_t* n, int depth)
{
    if (0 == depth)
    {
        if (BENCODE_TYPE_NONE == n->type) return n;

        /* find empty child slot */
        while (n->next)
            n = n->next;
        n->next = calloc(1, sizeof(node_t));
        n->next->parent = n;
        return n->next;
    }
    else
    {
        if (!n->child)
        {
            n->child = calloc(1, sizeof(node_t));
            n->child->parent = n;
        }
        return __find_child_slot(n->child, depth - 1);
    }
}

int __int(bencode_t *s,
        const char *dict_key,
        const long int val)
{
    node_t* n = __find_sibling_slot(s->udata, s->d);
    n->intval = val;
    n->dictkey = dict_key ? strdup(dict_key) : NULL;
    n->type = BENCODE_TYPE_INT;
    return 1;
}

int __str(bencode_t *s,
        const char *dict_key,
        unsigned int v_total_len __attribute__((__unused__)),
        const unsigned char* val,
        unsigned int v_len) 
{
    node_t* n = __find_sibling_slot(s->udata, s->d);
    assert(n);
    //printf("string val:%.*s key:%s\n", v_len, val, dict_key);
    n->strval = malloc(v_len);
    n->sv_len = v_len;
    memcpy(n->strval, val, v_len);
    n->dictkey = !dict_key ? NULL : strdup(dict_key);
    n->type = BENCODE_TYPE_STR;
    return 1;
}

int __dict_enter(bencode_t *s,
        const char *dict_key __attribute__((__unused__)))
{
    node_t* n = __find_child_slot(s->udata, s->d);
    n->type = BENCODE_TYPE_DICT;
    n->child = __find_child_slot(s->udata, s->d + 1);
    n->child->parent = n;
    return 1;
}

int __dict_leave(bencode_t *s, const char *dict_key __attribute__((__unused__)))
{
    node_t* n = s->udata;
    n->dict_leave_called += 1;
    return 1;
}

int __list_enter(bencode_t *s, const char *dict_key)
{
    node_t* n = __find_child_slot(s->udata, s->d);
    n->type = BENCODE_TYPE_LIST;
    n->child = __find_child_slot(s->udata, s->d + 1);
    n->dictkey = !dict_key ? NULL : strdup(dict_key);
    assert(n->child);
    n->child->parent = n;
    return 1;
}

int __list_leave(bencode_t *s, const char *dict_key __attribute__((__unused__)))
{
    node_t* n = s->udata;
    n->list_leave_called += 1;
    return 1;
}

int __list_next(bencode_t *s __attribute__((__unused__)))
{
    node_t* n = s->udata;
    n->list_next_called += 1;
    return 1;
}

int __dict_next(bencode_t *s __attribute__((__unused__)))
{
    node_t* n = s->udata;
    n->dict_next_called += 1;
    return 1;
}


static bencode_callbacks_t __cb = {
    .hit_int = __int,
    .hit_str = __str,
    .dict_enter = __dict_enter,
    .dict_leave = __dict_leave,
    .list_enter = __list_enter,
    .list_leave = __list_leave,
    .list_next = __list_next,
    .dict_next = __dict_next
};

void TestBencodeTest_add_sibling_adds_sibling(
    CuTest * tc
)
{
    node_t* dom = calloc(1,sizeof(node_t));
    node_t* n;

    n = __find_sibling_slot(dom, 0);
    n->type = BENCODE_TYPE_INT;
    CuAssertTrue(tc, n == dom);

    n = __find_sibling_slot(dom, 0);
    CuAssertTrue(tc, n != dom);
    CuAssertTrue(tc, dom->next == n);
    CuAssertTrue(tc, dom == n->prev);
    CuAssertTrue(tc, !dom->child);
    CuAssertTrue(tc, !dom->next->parent);
}

void TestBencodeTest_add_child_adds_child(
    CuTest * tc
)
{
    node_t* dom = calloc(1,sizeof(node_t));
    node_t* n;

    n = __find_sibling_slot(dom, 0);
    n->type = BENCODE_TYPE_INT;
    CuAssertTrue(tc, n == dom);

    n = __find_child_slot(dom, 1);
    CuAssertTrue(tc, n != dom);
    CuAssertTrue(tc, !dom->next);
    CuAssertTrue(tc, dom == n->parent);
    CuAssertTrue(tc, dom->child == n);
    CuAssertTrue(tc, !dom->next);
    CuAssertTrue(tc, !dom->prev);
    CuAssertTrue(tc, !n->next);
    CuAssertTrue(tc, !n->prev);
}

void TestBencode_fail_if_depth_not_sufficient(
    CuTest * tc
)
{
    bencode_t* s;
    char* str = "4:test";

    s = bencode_new(0, &__cb, NULL);
    CuAssertTrue(tc, 0 == bencode_dispatch_from_buffer(s, str, strlen(str)));
}

void TestBencodeIntValue(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "i123e";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(2, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_INT);
    CuAssertTrue(tc, 123 == dom->intval);
}

void TestBencodeIntValue2(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "i102030e";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(2, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_INT);
    CuAssertTrue(tc, 102030 == dom->intval);
}

void TestBencodeIntValueLarge(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "i252875232e";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(2, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_INT);
}

void TestBencodeIsIntEmpty(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = " ";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(2, &__cb, dom);
    CuAssertTrue(tc, 0 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_NONE);
}

void TestBencodeStringValue(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "4:test";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(2, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strncmp(dom->strval, "test", 4));
}

void TestBencodeStringValue2(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "12:flyinganimal";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(2, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strncmp(dom->strval, "flyinganimal", 12));
}

void TestBencodeStringHandlesNonAscii0(
    CuTest * tc
)
{
    bencode_t* s;
    node_t* dom = calloc(1,sizeof(node_t));
    char *str = strdup("6:xxxxxx");

    /*  127.0.0.1:80 */
    str[2] = 127;
    str[3] = 0;
    str[4] = 0;
    str[5] = 1;
    str[6] = 0;
    str[7] = 80;

    s = bencode_new(2, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, 8));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, dom->strval[0] == 127);
    CuAssertTrue(tc, dom->strval[1] == 0);
    CuAssertTrue(tc, dom->strval[2] == 0);
    CuAssertTrue(tc, dom->strval[3] == 1);
    CuAssertTrue(tc, dom->strval[4] == 0);
    CuAssertTrue(tc, dom->strval[5] == 80);
}

void TestBencodeStringValueWithColon(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "13:flying:animal";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(2, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strncmp(dom->strval, "flying:animal",13));
}


void TestBencodeIsList(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "l4:test3:fooe";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(2, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_LIST);
}

#if 0 /* debugging */
static void print_dom(node_t* n, int d)
{
    int i;
    printf("t:%d v:(%.*s %d) k: %s\n",
            n->type, n->sv_len, n->strval, n->intval, n->dictkey);

    if (n->child)
    {
        for (i=0; i<d; i++) printf("\t");
        printf("[c]");
        print_dom(n->child,d+1);
    }

    if (n->next)
    {
        for (i=0; i<d; i++) printf("\t");
        printf("[s]");
        print_dom(n->next,d);
    }
}
#endif

void TestBencodeListGetNext(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "l3:foo3:bare";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(2, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_LIST);
    CuAssertPtrNotNull(tc, dom->child);
    CuAssertTrue(tc, dom->child->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strncmp(dom->child->strval,"foo",3));
    CuAssertPtrNotNull(tc, dom->child->next);
    CuAssertTrue(tc, dom->child->next->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strncmp(dom->child->next->strval,"bar",3));
}

void TestBencodeListInListWithValue(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "ll3:fooee";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(3, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_LIST);
    CuAssertPtrNotNull(tc, dom->child);
    CuAssertTrue(tc, dom->child->type == BENCODE_TYPE_LIST);
    CuAssertPtrNotNull(tc, dom->child->child);
    CuAssertTrue(tc, dom->child->child->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strncmp(dom->child->child->strval,"foo",3));
}

void TestBencodeListDoesntHasNextWhenEmpty(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "le";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(2, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_LIST);
//    CuAssertTrue(tc, dom->child == NULL);
}

void TestBencodeEmptyListInListWontGetNextIfEmpty(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "llee";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(2, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_LIST);
    CuAssertPtrNotNull(tc, dom->child);
    CuAssertTrue(tc, dom->child->type == BENCODE_TYPE_LIST);
//    CuAssertTrue(tc, dom->child->child == NULL);
}

void TestBencodeEmptyListInListWontGetNextTwiceIfEmpty(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "llelee";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(2, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_LIST);
    CuAssertPtrNotNull(tc, dom->child);
    CuAssertTrue(tc, dom->child->type == BENCODE_TYPE_LIST);
    CuAssertPtrNotNull(tc, dom->child->child);
    CuAssertTrue(tc, dom->child->next->type == BENCODE_TYPE_LIST);
}

void TestBencodeListGetNextTwiceWhereOnlyOneAvailable(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "l4:teste";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(2, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_LIST);
    CuAssertPtrNotNull(tc, dom->child);
    CuAssertTrue(tc, dom->child->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strncmp(dom->child->strval,"test",4));
}

void TestBencodeListGetNextTwice(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "l4:test3:fooe";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(2, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_LIST);
    CuAssertPtrNotNull(tc, dom->child);
    CuAssertTrue(tc, dom->child->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strncmp(dom->child->strval,"test",4));
}

#if 0
void T_estBencodeListGetNextAtInvalidEnd(
    CuTest * tc
)
{
    bencode_t ben, ben2;

    char *str = strdup("l4:testg");

    bencode_init(&ben, str, strlen(str));

    CuAssertTrue(tc, 1 == bencode_list_get_next(&ben, &ben2));
    CuAssertTrue(tc, -1 == bencode_list_get_next(&ben, &ben2));

    free(str);
}
#endif

void TestBencodeDictGetNext(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "d3:foo3:bare";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(2, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_DICT);
    CuAssertPtrNotNull(tc, dom->child);
    CuAssertTrue(tc, dom->child->type == BENCODE_TYPE_STR);
    CuAssertPtrNotNull(tc, dom->child->strval);
    CuAssertTrue(tc, 0 == strncmp(dom->child->strval,"bar",3));
    CuAssertPtrNotNull(tc, dom->child->dictkey);
    CuAssertTrue(tc, 0 == strncmp(dom->child->dictkey,"foo",3));
}

void TestBencodeDictWontGetNextIfEmpty(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "de";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(16, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_DICT);
//    CuAssertTrue(tc, dom->child == NULL);
}

void TestBencodeDictGetNextTwice(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "d4:test3:egg3:foo3:hame";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(12, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_DICT);

    CuAssertPtrNotNull(tc, dom->child);
    CuAssertTrue(tc, dom->child->type == BENCODE_TYPE_STR);
    CuAssertPtrNotNull(tc, dom->child->dictkey);
    CuAssertTrue(tc, 0 == strncmp(dom->child->dictkey,"test",4));
    CuAssertPtrNotNull(tc, dom->child->strval);
    CuAssertTrue(tc, 0 == strncmp(dom->child->strval,"egg",3));

    CuAssertPtrNotNull(tc, dom->child->next);
    CuAssertTrue(tc, dom->child->next->type == BENCODE_TYPE_STR);
    CuAssertPtrNotNull(tc, dom->child->next->dictkey);
    CuAssertTrue(tc, 0 == strncmp(dom->child->next->dictkey,"foo",3));
    CuAssertPtrNotNull(tc, dom->child->next->strval);
    CuAssertTrue(tc, 0 == strncmp(dom->child->next->strval,"ham",3));
}

void TestBencodeDictGetNextTwiceOnlyIfSecondKeyValid(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "d4:test3:egg2:foo3:hame";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(10, &__cb, dom);
    CuAssertTrue(tc, 0 == bencode_dispatch_from_buffer(s, str, strlen(str)));
#if 0
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_DICT);
    CuAssertPtrNotNull(tc, dom->child);
    CuAssertTrue(tc, dom->child->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strncmp(dom->child->strval,"egg",3));
    CuAssertTrue(tc, 0 == strncmp(dom->child->dictkey,"test",4));
    CuAssertTrue(tc, dom->child->next == NULL);
#endif
}

void TestBencodeDictGetNextInnerList(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "d3:keyl4:test3:fooee";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(16, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_DICT);

    CuAssertPtrNotNull(tc, dom->child);
    CuAssertTrue(tc, dom->child->type == BENCODE_TYPE_LIST);
    CuAssertPtrNotNull(tc, dom->child->dictkey);
    CuAssertTrue(tc, 0 == strncmp(dom->child->dictkey,"key",3));

    CuAssertPtrNotNull(tc, dom->child->child);
    CuAssertTrue(tc, dom->child->child->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strncmp(dom->child->child->strval,"test",4));

    CuAssertPtrNotNull(tc, dom->child->child->next);
    CuAssertTrue(tc, dom->child->child->next->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strncmp(dom->child->child->next->strval,"foo",4));
}

void TestBencodeDictInnerList(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "d3:keyl4:test3:fooe3:foo3:bare";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(10, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_DICT);

    CuAssertPtrNotNull(tc, dom->child);
    CuAssertTrue(tc, dom->child->type == BENCODE_TYPE_LIST);
    CuAssertPtrNotNull(tc, dom->child->dictkey);
    CuAssertTrue(tc, 0 == strncmp(dom->child->dictkey,"key",3));

    CuAssertPtrNotNull(tc, dom->child->child);
    CuAssertTrue(tc, dom->child->child->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strncmp(dom->child->child->strval,"test",4));

    CuAssertPtrNotNull(tc, dom->child->child->next);
    CuAssertTrue(tc, dom->child->child->next->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strncmp(dom->child->child->next->strval,"foo",3));

    CuAssertPtrNotNull(tc, dom->child->next);
    CuAssertTrue(tc, dom->child->next->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strncmp(dom->child->next->dictkey,"foo",3));
    CuAssertTrue(tc, 0 == strncmp(dom->child->next->strval,"bar",3));
}

void TestBencodeStringValueIsZeroLength(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "d8:intervali1800e5:peers0:e";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(10, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_DICT);

    CuAssertPtrNotNull(tc, dom->child);
    CuAssertTrue(tc, dom->child->type == BENCODE_TYPE_INT);
    CuAssertPtrNotNull(tc, dom->child->dictkey);
    CuAssertTrue(tc, 0 == strncmp(dom->child->dictkey,"interval",8));
    CuAssertTrue(tc, 1800 == dom->child->intval);

    CuAssertPtrNotNull(tc, dom->child->next);
    CuAssertTrue(tc, dom->child->next->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strncmp(dom->child->next->dictkey,"peers",5));
    CuAssertPtrNotNull(tc, dom->child->next->strval);
}

void TestBencodeListNextGetsCalled(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "l4:test3:fooe";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(10, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->list_next_called != 0);
    CuAssertTrue(tc, dom->list_next_called != 1);
    CuAssertTrue(tc, dom->list_next_called == 2);
}

void TestBencodeDictNextGetsCalled(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "d3:keyl4:test3:fooe4:testi999ee";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(10, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->dict_next_called != 0);
    CuAssertTrue(tc, dom->dict_next_called != 1);
    CuAssertTrue(tc, dom->dict_next_called == 2);
}

void TestBencodeListLeaveGetsCalled(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "l4:test3:fooe";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(10, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->list_leave_called == 1);
}

void TestBencodeDictLeaveGetsCalled(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "d3:key4:test3:food3:keyi999eee";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(10, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->dict_leave_called == 2);
}

