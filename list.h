/*-
 * Copyright (c) 1991, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *  @(#)queue.h 8.5 (Berkeley) 8/20/94
 * $FreeBSD$
 */
/*
 * modify:
 *   - all macro name is lower case
 *   - modify origin ``STAILQ`` to ``single-linked list``
 *   - modify origin ``TAILQ`` to ``double-linked list``
 *   - remove origin ``SLIST`` and ``LIST``
 *   - rename origin file name from ``sysqueue.h`` to ``list.h``
 **/
#ifndef __LIST_H__
#define __LIST_H__

/*
 * This file defines four types of data structures: singly-linked lists
 * and double-linked lists.
 *
 * A singly-linked list is headed by a pair of pointers, one to the
 * head of the list and the other to the tail of the list. The elements are
 * singly linked for minimum space and pointer manipulation overhead at the
 * expense of O(n) removal for arbitrary elements. New elements can be added
 * to the list after an existing element, at the head of the list, or at the
 * end of the list. Elements being removed from the head of the tail queue
 * should use the explicit macro for this purpose for optimum efficiency.
 * A singly-linked tail queue may only be traversed in the forward direction.
 * Singly-linked tail queues are ideal for applications with large datasets
 * and few or no removals or for implementing a FIFO queue.
 *
 * A double-linked list is headed by a pair of pointers, one to the head of the
 * list and the other to the tail of the list. The elements are doubly
 * linked so that an arbitrary element can be removed without a need to
 * traverse the list. New elements can be added to the list before or
 * after an existing element, at the head of the list, or at the end of
 * the list. A tail queue may be traversed in either direction.
 *
 *                             slist     list
 * _head                       +         +
 * _head_initializer           +         +
 * _entry                      +         +
 * _init                       +         +
 * _empty                      +         +
 * _first                      +         +
 * _next                       +         +
 * _prev                       -         +
 * _last                       +         +
 * _foreach                    +         +
 * _foreach_from               +         +
 * _foreach_safe               +         +
 * _foreach_from_safe          +         +
 * _foreach_reverse            -         +
 * _foreach_reverse_from       -         +
 * _foreach_reverse_safe       -         +
 * _foreach_reverse_from_safe  -         +
 * _insert_head                +         +
 * _insert_before              -         +
 * _insert_after               +         +
 * _insert_tail                +         +
 * _concat                     +         +
 * _remove_after               +         -
 * _remove_head                +         -
 * _remove                     +         +
 * _swap                       +         +
 *
 */
#ifndef container_of
#include <stddef.h>
#define container_of(ptr, type, member) \
   ((type *) ((char *) (ptr) - offsetof(type, member)))
#endif
/*
 * Singly-linked list declarations.
 */
#define slist_head(name, type)                                     \
   struct name {                                                   \
      struct type *slh_first;/* first element */                   \
      struct type **slh_last;/* addr of last next element */       \
   }

#define slist_head_initializer(head)                               \
    { NULL, &(head).slh_first }

#define slist_entry(type)                                          \
   struct {                                                        \
      struct type *sle_next; /* next element */                    \
   }

/*
 * Singly-linked list functions.
 */
#define slist_concat(head1, head2)                                 \
   do {                                                            \
      if (!slist_empty((head2))) {                                 \
         *(head1)->slh_last = (head2)->slh_first;                  \
         (head1)->slh_last = (head2)->slh_last;                    \
         slist_init((head2));                                      \
      }                                                            \
   } while (0)

#define slist_empty(head)  ((head)->slh_first == NULL)

#define slist_first(head)  ((head)->slh_first)

#define slist_foreach(var, head, field)                            \
   for((var) = slist_first((head));                                \
       (var);                                                      \
       (var) = slist_next((var), field))

#define slist_foreach_from(var, head, field)                       \
   for ((var) = ((var) ? (var) : slist_first((head)));             \
        (var);                                                     \
        (var) = slist_next((var), field))

#define slist_foreach_safe(var, head, field, tvar)                 \
   for ((var) = slist_first((head));                               \
        (var) && ((tvar) = slist_next((var), field), 1);           \
        (var) = (tvar))

#define slist_foreach_from_safe(var, head, field, tvar)            \
   for ((var) = ((var) ? (var) : slist_first((head)));             \
        (var) && ((tvar) = slist_next((var), field), 1);           \
        (var) = (tvar))

#define slist_init(head)                                           \
   do {                                                            \
      slist_first((head)) = NULL;                                  \
      (head)->slh_last = &slist_first((head));                     \
   } while (0)

#define slist_insert_after(head, tqelm, elm, field)                \
   do {                                                            \
      if ((slist_next((elm), field) = slist_next((tqelm), field)) == NULL) \
         (head)->slh_last = &slist_next((elm), field);             \
      slist_next((tqelm), field) = (elm);                          \
   } while (0)

#define slist_insert_head(head, elm, field)                        \
   do {                                                            \
      if ((slist_next((elm), field) = slist_first((head))) == NULL) \
         (head)->slh_last = &slist_next((elm), field);             \
      slist_first((head)) = (elm);                                 \
   } while (0)

#define slist_insert_tail(head, elm, field)                        \
   do {                                                            \
      slist_next((elm), field) = NULL;                             \
      *(head)->slh_last = (elm);                                   \
      (head)->slh_last = &slist_next((elm), field);                \
   } while (0)

#define slist_last(head, type, field)                              \
   (slist_empty((head)) ? NULL :                                   \
    container_of((head)->slh_last, struct type, field.sle_next))

#define slist_next(elm, field) ((elm)->field.sle_next)

#define slist_remove(head, elm, type, field)                       \
   do {                                                            \
      if (slist_first((head)) == (elm)) {                          \
         slist_remove_head((head), field);                         \
      }                                                            \
      else {                                                       \
         struct type *curelm = slist_first((head));                \
         while (slist_next(curelm, field) != (elm))                \
            curelm = slist_next(curelm, field);                    \
         slist_remove_after(head, curelm, field);                  \
      }                                                            \
   } while (0)

#define slist_remove_after(head, elm, field)                       \
   do {                                                            \
      if ((slist_next(elm, field) =                                \
           slist_next(slist_next(elm, field), field)) == NULL)     \
         (head)->slh_last = &slist_next((elm), field);             \
   } while (0)

#define slist_remove_head(head, field)                             \
   do {                                                            \
      if ((slist_first((head)) =                                   \
           slist_next(slist_first((head)), field)) == NULL)        \
         (head)->slh_last = &slist_first((head));                  \
   } while (0)

#define slist_swap(head1, head2, type)                             \
   do {                                                            \
      struct type *swap_first = slist_first(head1);                \
      struct type **swap_last = (head1)->slh_last;                 \
      slist_first(head1) = slist_first(head2);                     \
      (head1)->slh_last = (head2)->slh_last;                       \
      slist_first(head2) = swap_first;                             \
      (head2)->slh_last = swap_last;                               \
      if (slist_empty(head1))                                      \
         (head1)->slh_last = &slist_first(head1);                  \
      if (slist_empty(head2))                                      \
         (head2)->slh_last = &slist_first(head2);                  \
   } while (0)

/*
 * List declarations.
 */
#define list_head(name, type)                                      \
   struct name {                                                   \
      struct type *lh_first; /* first element */                   \
      struct type **lh_last; /* addr of last next element */       \
   }

#define list_head_initializer(head)                                \
    { NULL, &(head).lh_first }

#define list_entry(type)                                           \
   struct {                                                        \
      struct type *le_next;  /* next element */                    \
      struct type **le_prev; /* address of previous next element */\
   }

/*
 * List functions.
 */

#define list_concat(head1, head2, field)                           \
   do {                                                            \
      if (!list_empty(head2)) {                                    \
         *(head1)->lh_last = (head2)->lh_first;                    \
         (head2)->lh_first->field.le_prev = (head1)->lh_last;      \
         (head1)->lh_last = (head2)->lh_last;                      \
         list_init((head2));                                       \
      }                                                            \
   } while (0)

#define list_empty(head)   ((head)->lh_first == NULL)

#define list_first(head)   ((head)->lh_first)

#define list_foreach(var, head, field)                             \
   for ((var) = list_first((head));                                \
        (var);                                                     \
        (var) = list_next((var), field))

#define list_foreach_from(var, head, field)                        \
   for ((var) = ((var) ? (var) : list_first((head)));              \
        (var);                                                     \
        (var) = list_next((var), field))

#define list_foreach_safe(var, head, field, tvar)                  \
   for ((var) = list_first((head));                                \
        (var) && ((tvar) = list_next((var), field), 1);            \
        (var) = (tvar))

#define list_foreach_from_safe(var, head, field, tvar)             \
   for ((var) = ((var) ? (var) : list_first((head)));              \
        (var) && ((tvar) = list_next((var), field), 1);            \
        (var) = (tvar))

#define list_foreach_reverse(var, head, headname, field)           \
   for ((var) = list_last((head), headname);                       \
        (var);                                                     \
        (var) = list_prev((var), headname, field))

#define list_foreach_reverse_from(var, head, headname, field)      \
   for ((var) = ((var) ? (var) : list_last((head), headname));     \
        (var);                                                     \
        (var) = list_prev((var), headname, field))

#define list_foreach_reverse_safe(var, head, headname, field, tvar) \
   for ((var) = list_last((head), headname);                       \
        (var) && ((tvar) = list_prev((var), headname, field), 1);  \
        (var) = (tvar))

#define list_foreach_reverse_from_safe(var, head, headname, field, tvar) \
   for ((var) = ((var) ? (var) : list_last((head), headname));     \
        (var) && ((tvar) = list_prev((var), headname, field), 1);  \
        (var) = (tvar))

#define list_init(head)                                            \
   do {                                                            \
      list_first((head)) = NULL;                                   \
      (head)->lh_last = &list_first((head));                       \
   } while (0)

#define list_insert_after(head, listelm, elm, field)               \
   do {                                                            \
      if ((list_next((elm), field) = list_next((listelm), field)) != NULL) \
         list_next((elm), field)->field.le_prev =                  \
            &list_next((elm), field);                              \
      else {                                                       \
         (head)->lh_last = &list_next((elm), field);               \
      }                                                            \
      list_next((listelm), field) = (elm);                         \
      (elm)->field.le_prev = &list_next((listelm), field);         \
   } while (0)

#define list_insert_before(head, listelm, elm, field)              \
   do {                                                            \
      (elm)->field.le_prev = (listelm)->field.le_prev;             \
      list_next((elm), field) = (listelm);                         \
      *(listelm)->field.le_prev = (elm);                           \
      (listelm)->field.le_prev = &list_next((elm), field);         \
   } while (0)

#define list_insert_head(head, elm, field)                         \
   do {                                                            \
      if ((list_next((elm), field) = list_first((head))) != NULL)  \
         list_first((head))->field.le_prev =                       \
            &list_next((elm), field);                              \
      else                                                         \
         (head)->lh_last = &list_next((elm), field);               \
      list_first((head)) = (elm);                                  \
      (elm)->field.le_prev = &list_first((head));                  \
   } while (0)

#define list_insert_tail(head, elm, field)                         \
   do {                                                            \
      list_next((elm), field) = NULL;                              \
      (elm)->field.le_prev = (head)->lh_last;                      \
      *(head)->lh_last = (elm);                                    \
      (head)->lh_last = &list_next((elm), field);                  \
   } while (0)

#define list_last(head, headname)                                  \
   (*(((struct headname *)((head)->lh_last))->lh_last))

#define list_next(elm, field) ((elm)->field.le_next)

#define list_prev(elm, headname, field)                            \
    (*(((struct headname *)((elm)->field.le_prev))->lh_last))

#define list_remove(head, elm, field)                              \
   do {                                                            \
      if ((list_next((elm), field)) != NULL)                       \
         list_next((elm), field)->field.le_prev =                  \
            (elm)->field.le_prev;                                  \
      else {                                                       \
         (head)->lh_last = (elm)->field.le_prev;                   \
      }                                                            \
      *(elm)->field.le_prev = list_next((elm), field);             \
   } while (0)

#define list_swap(head1, head2, type, field)                       \
   do {                                                            \
      struct type *swap_first = (head1)->lh_first;                 \
      struct type **swap_last = (head1)->lh_last;                  \
      (head1)->lh_first = (head2)->lh_first;                       \
      (head1)->lh_last = (head2)->lh_last;                         \
      (head2)->lh_first = swap_first;                              \
      (head2)->lh_last = swap_last;                                \
      if ((swap_first = (head1)->lh_first) != NULL)                \
         swap_first->field.le_prev = &(head1)->lh_first;           \
      else                                                         \
         (head1)->lh_last = &(head1)->lh_first;                    \
      if ((swap_first = (head2)->lh_first) != NULL)                \
         swap_first->field.le_prev = &(head2)->lh_first;           \
      else                                                         \
         (head2)->lh_last = &(head2)->lh_first;                    \
   } while (0)

#endif /* !__LIST_H__ */
