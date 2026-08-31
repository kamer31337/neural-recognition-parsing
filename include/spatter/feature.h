#ifndef SPATTER_FEATURE_H
#define SPATTER_FEATURE_H

#include "spatter/common.h"
#include "spatter/str_view.h"
#include "spatter/tree.h"
#include "spatter/list.h"

typedef enum QuestionType {
    Q_CURR_WORD = 0,
    Q_PREV_WORD,
    Q_NEXT_WORD,
    Q_CURR_TAG,
    Q_PREV_TAG,
    Q_PREV2_TAG,
    Q_NEXT_TAG,
    Q_NEXT2_TAG,
    Q_PREV_LABEL,
    Q_LEFT_CHILD_LABEL,
    Q_RIGHT_CHILD_LABEL,
    Q_HEAD_TAG,
    Q_IS_CAPITALIZED,
    Q_HAS_SUFFIX,
    Q_SPAN_LEN_GREATER,
    Q_IS_FIRST_WORD,
    Q_IS_LAST_WORD,
    Q_TYPE_COUNT
} QuestionType;

typedef struct Question {
    QuestionType type;
    int int_param;
    StrView str_param;
} Question;

typedef struct Context {
    StrView curr_word;
    StrView prev_word;
    StrView prev2_word;
    StrView next_word;
    StrView next2_word;
    Tag curr_tag;
    Tag prev_tag;
    Tag prev2_tag;
    Tag next_tag;
    Tag next2_tag;
    Label curr_label;
    Label prev_label;
    Label left_child_label;
    Label right_child_label;
    Extension curr_extension;
    Extension prev_extension;
    Tag head_tag;
    StrView head_word;
    size_t span_length;
    bool is_first_word;
    bool is_last_word;
} Context;

bool question_eval(Question q, const Context* ctx);
Context context_create_empty(void);
Context context_from_tokens(const StrView* words, const Tag* tags, size_t num_words, size_t curr_idx);
Context context_from_frontier(const List* frontier, size_t curr_idx, size_t sentence_len);
void question_print(Question q, FILE* stream);

#endif /* SPATTER_FEATURE_H */
