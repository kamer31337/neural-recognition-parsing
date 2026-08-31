#include "spatter/feature.h"
#include <ctype.h>

static bool is_capitalized(StrView sv)
{
    if (sv.length == 0 || !sv.data) {
        return false;
    }
    return isupper((unsigned char)sv.data[0]);
}

static bool has_suffix(StrView sv, StrView suffix)
{
    if (sv.length < suffix.length || !sv.data || !suffix.data) {
        return false;
    }
    const char* p = sv.data + (sv.length - suffix.length);
    return memcmp(p, suffix.data, suffix.length) == 0;
}

bool question_eval(Question q, const Context* ctx)
{
    if (!ctx) {
        return false;
    }
    switch (q.type) {
        case Q_CURR_WORD:
            return str_view_equals(ctx->curr_word, q.str_param);
        case Q_PREV_WORD:
            return str_view_equals(ctx->prev_word, q.str_param);
        case Q_NEXT_WORD:
            return str_view_equals(ctx->next_word, q.str_param);
        case Q_CURR_TAG:
            return ctx->curr_tag == (Tag)q.int_param;
        case Q_PREV_TAG:
            return ctx->prev_tag == (Tag)q.int_param;
        case Q_PREV2_TAG:
            return ctx->prev2_tag == (Tag)q.int_param;
        case Q_NEXT_TAG:
            return ctx->next_tag == (Tag)q.int_param;
        case Q_NEXT2_TAG:
            return ctx->next2_tag == (Tag)q.int_param;
        case Q_PREV_LABEL:
            return ctx->prev_label == (Label)q.int_param;
        case Q_LEFT_CHILD_LABEL:
            return ctx->left_child_label == (Label)q.int_param;
        case Q_RIGHT_CHILD_LABEL:
            return ctx->right_child_label == (Label)q.int_param;
        case Q_HEAD_TAG:
            return ctx->head_tag == (Tag)q.int_param;
        case Q_IS_CAPITALIZED:
            return is_capitalized(ctx->curr_word);
        case Q_HAS_SUFFIX:
            return has_suffix(ctx->curr_word, q.str_param);
        case Q_SPAN_LEN_GREATER:
            return ctx->span_length > (size_t)q.int_param;
        case Q_IS_FIRST_WORD:
            return ctx->is_first_word;
        case Q_IS_LAST_WORD:
            return ctx->is_last_word;
        default:
            return false;
    }
}

Context context_create_empty(void)
{
    Context ctx = {
        .curr_word = str_view_create(NULL, 0),
        .prev_word = str_view_create(NULL, 0),
        .prev2_word = str_view_create(NULL, 0),
        .next_word = str_view_create(NULL, 0),
        .next2_word = str_view_create(NULL, 0),
        .curr_tag = TAG_UNKNOWN,
        .prev_tag = TAG_UNKNOWN,
        .prev2_tag = TAG_UNKNOWN,
        .next_tag = TAG_UNKNOWN,
        .next2_tag = TAG_UNKNOWN,
        .curr_label = LBL_NONE,
        .prev_label = LBL_NONE,
        .left_child_label = LBL_NONE,
        .right_child_label = LBL_NONE,
        .curr_extension = EXT_NONE,
        .prev_extension = EXT_NONE,
        .head_tag = TAG_UNKNOWN,
        .head_word = str_view_create(NULL, 0),
        .span_length = 1,
        .is_first_word = false,
        .is_last_word = false
    };
    return ctx;
}

Context context_from_tokens(const StrView* words, const Tag* tags, size_t num_words, size_t curr_idx)
{
    Context ctx = context_create_empty();
    if (curr_idx < num_words && words) {
        ctx.curr_word = words[curr_idx];
    }
    if (curr_idx < num_words && tags) {
        ctx.curr_tag = tags[curr_idx];
    }
    if (curr_idx >= 1 && words) {
        ctx.prev_word = words[curr_idx - 1];
    }
    if (curr_idx >= 1 && tags) {
        ctx.prev_tag = tags[curr_idx - 1];
    }
    if (curr_idx >= 2 && words) {
        ctx.prev2_word = words[curr_idx - 2];
    }
    if (curr_idx >= 2 && tags) {
        ctx.prev2_tag = tags[curr_idx - 2];
    }
    if (curr_idx + 1 < num_words && words) {
        ctx.next_word = words[curr_idx + 1];
    }
    if (curr_idx + 1 < num_words && tags) {
        ctx.next_tag = tags[curr_idx + 1];
    }
    if (curr_idx + 2 < num_words && words) {
        ctx.next2_word = words[curr_idx + 2];
    }
    if (curr_idx + 2 < num_words && tags) {
        ctx.next2_tag = tags[curr_idx + 2];
    }
    ctx.is_first_word = (curr_idx == 0);
    ctx.is_last_word = (curr_idx + 1 >= num_words);
    ctx.span_length = 1;
    return ctx;
}

Context context_from_frontier(const List* frontier, size_t curr_idx, size_t sentence_len)
{
    Context ctx = context_create_empty();
    size_t frontier_len = list_length(frontier);
    const ParseNode* curr_node = (const ParseNode*)list_nth(frontier, curr_idx);
    if (!curr_node) {
        return ctx;
    }
    ctx.curr_word = curr_node->word;
    ctx.curr_tag = curr_node->tag;
    ctx.curr_label = curr_node->label;
    ctx.curr_extension = curr_node->extension;
    ctx.span_length = (curr_node->end_idx >= curr_node->start_idx) ? (curr_node->end_idx - curr_node->start_idx) : 1;
    ctx.is_first_word = (curr_node->start_idx == 0);
    ctx.is_last_word = (curr_node->end_idx >= sentence_len);
    if (curr_idx >= 1) {
        const ParseNode* prev = (const ParseNode*)list_nth(frontier, curr_idx - 1);
        if (prev) {
            ctx.prev_word = prev->word;
            ctx.prev_tag = prev->tag;
            ctx.prev_label = prev->label;
            ctx.prev_extension = prev->extension;
        }
    }
    if (curr_idx >= 2) {
        const ParseNode* prev2 = (const ParseNode*)list_nth(frontier, curr_idx - 2);
        if (prev2) {
            ctx.prev2_word = prev2->word;
            ctx.prev2_tag = prev2->tag;
        }
    }
    if (curr_idx + 1 < frontier_len) {
        const ParseNode* next = (const ParseNode*)list_nth(frontier, curr_idx + 1);
        if (next) {
            ctx.next_word = next->word;
            ctx.next_tag = next->tag;
        }
    }
    if (curr_idx + 2 < frontier_len) {
        const ParseNode* next2 = (const ParseNode*)list_nth(frontier, curr_idx + 2);
        if (next2) {
            ctx.next2_word = next2->word;
            ctx.next2_tag = next2->tag;
        }
    }
    if (curr_node->children) {
        const ParseNode* first_child = (const ParseNode*)list_head(curr_node->children);
        if (first_child) {
            ctx.left_child_label = first_child->label;
            ctx.head_tag = first_child->tag;
            ctx.head_word = first_child->word;
        }
        size_t n_children = list_length(curr_node->children);
        if (n_children > 1) {
            const ParseNode* last_child = (const ParseNode*)list_nth(curr_node->children, n_children - 1);
            if (last_child) {
                ctx.right_child_label = last_child->label;
            }
        } else {
            ctx.right_child_label = ctx.left_child_label;
        }
    }
    return ctx;
}

void question_print(Question q, FILE* stream)
{
    static const char* q_names[] = {
        "CURR_WORD", "PREV_WORD", "NEXT_WORD", "CURR_TAG", "PREV_TAG", "PREV2_TAG",
        "NEXT_TAG", "NEXT2_TAG", "PREV_LABEL", "LEFT_CHILD_LBL", "RIGHT_CHILD_LBL",
        "HEAD_TAG", "IS_CAPITALIZED", "HAS_SUFFIX", "SPAN_LEN_GT", "IS_FIRST", "IS_LAST"
    };
    if ((size_t)q.type < sizeof(q_names) / sizeof(q_names[0])) {
        fprintf(stream, "Q(%s, int=%d, str=" SV_FMT ")", q_names[q.type], q.int_param, SV_ARG(q.str_param));
    }
}
