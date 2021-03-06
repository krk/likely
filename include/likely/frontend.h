/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright 2013 Joshua C. Klontz                                           *
 *                                                                           *
 * Licensed under the Apache License, Version 2.0 (the "License");           *
 * you may not use this file except in compliance with the License.          *
 * You may obtain a copy of the License at                                   *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 * Unless required by applicable law or agreed to in writing, software       *
 * distributed under the License is distributed on an "AS IS" BASIS,         *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
 * See the License for the specific language governing permissions and       *
 * limitations under the License.                                            *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef LIKELY_FRONTEND_H
#define LIKELY_FRONTEND_H

#include <stddef.h>
#include <likely/runtime.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef enum
{
    // list type
    likely_ast_list = 0,

    // atom types
    likely_ast_unknown,
    likely_ast_operator,
    likely_ast_string,
    likely_ast_number,
    likely_ast_type
} likely_abstract_syntax_tree_type;

struct likely_abstract_syntax_tree;
typedef struct likely_abstract_syntax_tree *likely_ast;
typedef struct likely_abstract_syntax_tree const *likely_const_ast;

struct likely_abstract_syntax_tree
{
    union {
        struct { // type == likely_ast_list
            const likely_ast * const atoms;
            likely_size num_atoms;
        };
        struct { // type != likely_ast_list
            const char * const atom;
            likely_size atom_len;
        };
    };

    likely_size ref_count;
    likely_size begin_line, begin_column, end_line, end_column;
    likely_abstract_syntax_tree_type type;
};

typedef struct likely_error
{
    likely_const_ast where;
    const char *what;
} likely_error;
typedef void (*likely_error_callback)(likely_error error, void *context);

LIKELY_EXPORT likely_ast likely_new_atom(const char *str, likely_size len);
LIKELY_EXPORT likely_ast likely_new_list(const likely_const_ast *atoms, size_t num_atoms);
LIKELY_EXPORT likely_ast likely_copy_ast(likely_const_ast ast);
LIKELY_EXPORT likely_ast likely_retain_ast(likely_const_ast ast);
LIKELY_EXPORT void likely_release_ast(likely_const_ast ast);

LIKELY_EXPORT likely_ast likely_tokens_from_string(const char *str, bool GFM);
LIKELY_EXPORT void likely_insert_operator(const char *symbol, int precedence, int right_hand_atoms);
LIKELY_EXPORT likely_ast likely_ast_from_tokens(likely_const_ast tokens);
LIKELY_EXPORT likely_ast likely_ast_from_string(const char *str, bool GFM);
LIKELY_EXPORT likely_mat likely_ast_to_string(likely_const_ast ast);
LIKELY_EXPORT int likely_ast_compare(likely_const_ast a, likely_const_ast b);
LIKELY_EXPORT const char *likely_get_symbol_name(likely_const_ast ast); // return value valid for lifetime of ast

// Callback-style error handling
LIKELY_EXPORT void likely_set_error_callback(likely_error_callback callback, void *context);
LIKELY_EXPORT bool likely_throw(likely_const_ast where, const char *what);
LIKELY_EXPORT likely_mat likely_error_to_string(likely_error error);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LIKELY_FRONTEND_H
