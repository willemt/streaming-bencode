
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
    char* dictkey;
    node_t* child;
    node_t* parent;
    node_t* next;
    node_t* prev;
};

static node_t* __find_sibling_slot(node_t* n, int depth)
{
    if (0 == depth)
    {
        while (n->next)
            n = n->next;
        n->next = calloc(1, sizeof(node_t));
        n->prev = n;
        return n->next;
    }
    else
    {
        return __find_sibling_slot(n->child, depth - 1);
    }
}

static node_t* __find_child_slot(node_t* n, int depth)
{
    if (0 == depth)
    {
        while (n->next)
            n = n->next;
        n->child = calloc(1, sizeof(node_t));
        n->parent = n;
        return n->child;
    }
    else
    {
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
        unsigned int val_len,
        const unsigned char* val,
        unsigned int len)
{
    node_t* n = __find_sibling_slot(s->udata, s->d);
    n->strval = strdup(val);
    n->dictkey = dict_key ? strdup(dict_key) : NULL;
    n->type = BENCODE_TYPE_STR;
    return 1;
}

int __dict_enter(bencode_t *s,
        const char *dict_key)
{
    node_t* n = __find_child_slot(s->udata, s->d);
    n->type = BENCODE_TYPE_DICT;
    return 1;
}

int __dict_leave(bencode_t *s,
        const char *dict_key)
{
    return 1;
}

int __list_enter(bencode_t *s,
        const char *dict_key)
{
    node_t* n = __find_child_slot(s->udata, s->d);
    n->type = BENCODE_TYPE_LIST;
    return 1;
}

int __list_leave(bencode_t *s,
        const char *dict_key)
{
    return 1;
}

int __list_next(bencode_t *s,
        const char *dict_key)
{
    return 1;
}

static bencode_callbacks_t __cb = {
    .hit_int = __int,
    .hit_str = __str,
    .dict_enter = __dict_enter,
    .dict_leave = __dict_leave,
    .list_enter = __list_enter,
    .list_leave = __list_leave,
    .list_next = __list_next
};

void TestBencode_fail_if_depth_not_sufficient(
    CuTest * tc
)
{
    bencode_t* s;
    char* str = "4:test";

    s = bencode_new(0, &__cb, NULL);
    CuAssertTrue(tc, 0 == bencode_dispatch_from_buffer(s, str, strlen(str)));
}

void TestBencode_string(
    CuTest * tc
)
{

}

void TestBencodeIntValue(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "i777e";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(2, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_INT);
    CuAssertTrue(tc, 777 == dom->intval);
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
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
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
    CuAssertTrue(tc, 0 == strcmp(dom->strval, "test"));
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
    CuAssertTrue(tc, 0 == strcmp(dom->strval, "12:flyinganimal"));
}

#if 0
/**
 * The string value function errors when the string is of insufficient length
 * */
void T_estBencodeStringInvalid(
    CuTest * tc
)
{
    bencode_t ben;

    char *str = strdup("5:test");

    bencode_t* s;
    char *str = "5:flyinganimal";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(2, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strcmp(dom->strval, "12:flyinganimal"));
}
#endif

void TestBencodeStringHandlesNonAscii0(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "6:xxxxxx";
    node_t* dom = calloc(1,sizeof(node_t));

    /*  127.0.0.1:80 */
    str[2] = 127;
    str[3] = 0;
    str[4] = 0;
    str[5] = 1;
    str[6] = 0;
    str[7] = 80;

    s = bencode_new(2, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, dom->strval[0] == 127);
    CuAssertTrue(tc, dom->strval[1] == 0);
    CuAssertTrue(tc, dom->strval[2] == 0);
    CuAssertTrue(tc, dom->strval[3] == 1);
    CuAssertTrue(tc, dom->strval[4] == 0);
    CuAssertTrue(tc, dom->strval[5] == 80);
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

#if 0
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
    CuAssertTrue(tc, dom->child != NULL);
    CuAssertTrue(tc, dom->child->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strcmp(dom->child->strval,"foo"));
    CuAssertTrue(tc, 0 == strcmp(dom->child->next->strval,"bar"));
}

void TestBencodeListInListWithValue(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "ll3:fooee";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(2, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_LIST);
    CuAssertTrue(tc, dom->child != NULL);
    CuAssertTrue(tc, dom->child->type == BENCODE_TYPE_LIST);
    CuAssertTrue(tc, dom->child->child != NULL);
    CuAssertTrue(tc, dom->child->child->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strcmp(dom->child->child->strval,"foo"));
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
    CuAssertTrue(tc, dom->child == NULL);
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
    CuAssertTrue(tc, dom->child != NULL);
    CuAssertTrue(tc, dom->child->type == BENCODE_TYPE_LIST);
    CuAssertTrue(tc, dom->child->child == NULL);
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
    CuAssertTrue(tc, dom->child != NULL);
    CuAssertTrue(tc, dom->child->type == BENCODE_TYPE_LIST);
    CuAssertTrue(tc, dom->child->child != NULL);
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
    CuAssertTrue(tc, dom->child != NULL);
    CuAssertTrue(tc, dom->child->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strcmp(dom->child->strval,"test"));
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
    CuAssertTrue(tc, dom->child != NULL);
    CuAssertTrue(tc, dom->child->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strcmp(dom->child->strval,"test"));
}

void TestBencodeListGetNextAtInvalidEnd(
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
    CuAssertTrue(tc, dom->child != NULL);
    CuAssertTrue(tc, dom->child->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strcmp(dom->child->strval,"bar"));
    CuAssertTrue(tc, 0 == strcmp(dom->child->dictkey,"foo"));
}

void TestBencodeDictWontGetNextIfEmpty(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "de";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(2, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_DICT);
    CuAssertTrue(tc, dom->child == NULL);
}

void TestBencodeDictGetNextTwice(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "d4:test3:egg3:foo3:hame";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(2, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_DICT);
    CuAssertTrue(tc, dom->child != NULL);
    CuAssertTrue(tc, dom->child->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strcmp(dom->child->strval,"egg"));
    CuAssertTrue(tc, 0 == strcmp(dom->child->dictkey,"test"));
    CuAssertTrue(tc, dom->child->next != NULL);
    CuAssertTrue(tc, dom->child->next->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strcmp(dom->child->next->strval,"foo"));
    CuAssertTrue(tc, 0 == strcmp(dom->child->next->dictkey,"ham"));
}

void TestBencodeDictGetNextTwiceOnlyIfSecondKeyValid(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "d4:test3:egg2:foo3:hame";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(2, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_DICT);
    CuAssertTrue(tc, dom->child != NULL);
    CuAssertTrue(tc, dom->child->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strcmp(dom->child->strval,"egg"));
    CuAssertTrue(tc, 0 == strcmp(dom->child->dictkey,"test"));
    CuAssertTrue(tc, dom->child->next == NULL);
}

void TestBencodeDictGetNextInnerList(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "d3:keyl4:test3:fooee";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(2, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_DICT);
    CuAssertTrue(tc, dom->child != NULL);
    CuAssertTrue(tc, dom->child->type == BENCODE_TYPE_LIST);
    CuAssertTrue(tc, 0 == strcmp(dom->child->dictkey,"key"));
    CuAssertTrue(tc, dom->child->child != NULL);
    CuAssertTrue(tc, dom->child->child->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strcmp(dom->child->child->strval,"test"));
    CuAssertTrue(tc, dom->child->child->next != NULL);
    CuAssertTrue(tc, dom->child->child->next->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strcmp(dom->child->child->next->strval,"test"));
}

void TestBencodeDictInnerList(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "d3:keyl4:test3:fooe3:foo3:bare";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(2, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_DICT);

    CuAssertTrue(tc, dom->child != NULL);
    CuAssertTrue(tc, dom->child->type == BENCODE_TYPE_LIST);
    CuAssertTrue(tc, 0 == strcmp(dom->child->dictkey,"key"));

    CuAssertTrue(tc, dom->child->child != NULL);
    CuAssertTrue(tc, dom->child->child->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strcmp(dom->child->child->strval,"test"));

    CuAssertTrue(tc, dom->child->child->next != NULL);
    CuAssertTrue(tc, dom->child->child->next->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strcmp(dom->child->child->next->strval,"foo"));

    CuAssertTrue(tc, dom->child->next != NULL);
    CuAssertTrue(tc, dom->child->next->type == BENCODE_TYPE_LIST);
    CuAssertTrue(tc, 0 == strcmp(dom->child->next->dictkey,"foo"));
    CuAssertTrue(tc, 0 == strcmp(dom->child->next->strval,"bar"));
}

void TestBencodeStringValueIsZeroLength(
    CuTest * tc
)
{
    bencode_t* s;
    char *str = "d8:intervali1800e5:peers0:e";
    node_t* dom = calloc(1,sizeof(node_t));

    s = bencode_new(2, &__cb, dom);
    CuAssertTrue(tc, 1 == bencode_dispatch_from_buffer(s, str, strlen(str)));
    CuAssertTrue(tc, dom->type == BENCODE_TYPE_DICT);

    CuAssertTrue(tc, dom->child != NULL);
    CuAssertTrue(tc, dom->child->type == BENCODE_TYPE_LIST);
    CuAssertTrue(tc, 0 == strcmp(dom->child->dictkey,"interval"));
    CuAssertTrue(tc, 1800 == dom->child->intval);

    CuAssertTrue(tc, dom->child->next != NULL);
    CuAssertTrue(tc, dom->child->next->type == BENCODE_TYPE_STR);
    CuAssertTrue(tc, 0 == strcmp(dom->child->next->strval,""));
    CuAssertTrue(tc, 0 == strcmp(dom->child->next->dictkey,""));
}
#endif
