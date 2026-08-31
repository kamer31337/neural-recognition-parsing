#include "spatter/treebank.h"
#include <ctype.h>

static void skip_whitespace(StrView* sv)
{
    while (sv->length > 0 && isspace((unsigned char)sv->data[0])) {
        sv->data++;
        sv->length--;
    }
}

static StrView read_token(StrView* sv)
{
    skip_whitespace(sv);
    if (sv->length == 0) {
        return str_view_create(NULL, 0);
    }
    size_t len = 0;
    while (len < sv->length && !isspace((unsigned char)sv->data[len]) && sv->data[len] != '(' && sv->data[len] != ')') {
        len++;
    }
    StrView tok = str_view_create(sv->data, len);
    sv->data += len;
    sv->length -= len;
    return tok;
}

static Result parse_sexpr_internal(StrView* sv, size_t* leaf_counter)
{
    skip_whitespace(sv);
    if (sv->length == 0 || sv->data[0] != '(') {
        return result_err("Expected '(' at start of S-expression");
    }
    sv->data++;
    sv->length--;
    skip_whitespace(sv);

    StrView symbol = read_token(sv);
    if (str_view_is_empty(symbol)) {
        return result_err("Expected symbol name after '('");
    }
    skip_whitespace(sv);

    if (sv->length > 0 && sv->data[0] != '(') {
        StrView word = read_token(sv);
        skip_whitespace(sv);
        if (sv->length == 0 || sv->data[0] != ')') {
            return result_err("Expected ')' after leaf token");
        }
        sv->data++;
        sv->length--;

        Tag tag = tag_from_str(symbol);
        size_t idx = (*leaf_counter)++;
        const ParseNode* leaf = parse_node_create_leaf(word, tag, idx, 0.0);
        return result_ok(leaf);
    }

    const List* children_rev = list_empty();
    while (sv->length > 0 && sv->data[0] == '(') {
        Result child_res = parse_sexpr_internal(sv, leaf_counter);
        if (result_is_err(child_res)) {
            list_free_nodes(children_rev);
            return child_res;
        }
        children_rev = list_cons(child_res.as.ok_val, children_rev);
        skip_whitespace(sv);
    }

    if (sv->length == 0 || sv->data[0] != ')') {
        list_free_nodes(children_rev);
        return result_err("Expected ')' after children list");
    }
    sv->data++;
    sv->length--;

    const List* children = list_reverse(children_rev);
    list_free_nodes(children_rev);

    Label lbl = label_from_str(symbol);
    const ParseNode* nonterminal = parse_node_create_nonterminal(lbl, EXT_NONE, children, 0.0);
    return result_ok(nonterminal);
}

Result treebank_parse_sexpr(StrView sexpr_str)
{
    StrView sv = sexpr_str;
    size_t leaf_counter = 0;
    return parse_sexpr_internal(&sv, &leaf_counter);
}

const List* treebank_load_file(const char* filepath)
{
    if (!filepath) {
        return list_empty();
    }
    FILE* f = fopen(filepath, "rb");
    if (!f) {
        return list_empty();
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize <= 0) {
        fclose(f);
        return list_empty();
    }

    char* buffer = (char*)spatter_alloc(fsize + 1);
    size_t read_bytes = fread(buffer, 1, fsize, f);
    buffer[read_bytes] = '\0';
    fclose(f);

    StrView sv = str_view_create(buffer, read_bytes);
    const List* trees_rev = list_empty();

    while (sv.length > 0) {
        skip_whitespace(&sv);
        if (sv.length == 0) {
            break;
        }
        if (sv.data[0] != '(') {
            read_token(&sv);
            continue;
        }
        size_t leaf_counter = 0;
        Result r = parse_sexpr_internal(&sv, &leaf_counter);
        if (result_is_ok(r)) {
            trees_rev = list_cons(r.as.ok_val, trees_rev);
        } else {
            break;
        }
    }

    const List* trees = list_reverse(trees_rev);
    list_free_nodes(trees_rev);
    return trees;
}

static void extract_samples_recursive(const ParseNode* node, const ParseNode* parent, size_t child_idx, size_t num_siblings, TrainingSample* pos_samples, size_t* num_pos, TrainingSample* ext_samples, size_t* num_ext, TrainingSample* lbl_samples, size_t* num_lbl)
{
    if (!node) {
        return;
    }
    Context ctx = context_create_empty();
    ctx.curr_word = node->word;
    ctx.curr_tag = node->tag;
    ctx.curr_label = node->label;
    ctx.span_length = (node->end_idx >= node->start_idx) ? (node->end_idx - node->start_idx) : 1;

    if (node->is_leaf) {
        if (pos_samples && num_pos) {
            TrainingSample s = { .context = ctx, .target_class = (int)node->tag, .weight = 1.0 };
            pos_samples[(*num_pos)++] = s;
        }
    }

    Extension ext = EXT_NONE;
    if (!parent) {
        ext = EXT_ROOT;
    } else if (num_siblings == 1) {
        ext = EXT_UNARY;
    } else if (child_idx == 0) {
        ext = EXT_RIGHT;
    } else if (child_idx == num_siblings - 1) {
        ext = EXT_LEFT;
    } else {
        ext = EXT_UP;
    }

    if (ext_samples && num_ext) {
        TrainingSample s = { .context = ctx, .target_class = (int)ext, .weight = 1.0 };
        ext_samples[(*num_ext)++] = s;
    }

    if (!node->is_leaf && node->label != LBL_NONE) {
        if (lbl_samples && num_lbl) {
            TrainingSample s = { .context = ctx, .target_class = (int)node->label, .weight = 1.0 };
            lbl_samples[(*num_lbl)++] = s;
        }
    }

    size_t n_children = list_length(node->children);
    const List* curr = node->children;
    size_t c_idx = 0;
    while (curr) {
        const ParseNode* child = (const ParseNode*)curr->head;
        extract_samples_recursive(child, node, c_idx, n_children, pos_samples, num_pos, ext_samples, num_ext, lbl_samples, num_lbl);
        c_idx++;
        curr = curr->tail;
    }
}

TreebankCorpus treebank_extract_corpus(const List* trees)
{
    TreebankCorpus corpus;
    size_t max_samples = list_length(trees) * 64 + 128;
    corpus.pos_samples = (TrainingSample*)spatter_alloc(max_samples * sizeof(TrainingSample));
    corpus.ext_samples = (TrainingSample*)spatter_alloc(max_samples * sizeof(TrainingSample));
    corpus.lbl_samples = (TrainingSample*)spatter_alloc(max_samples * sizeof(TrainingSample));
    corpus.num_pos = 0;
    corpus.num_ext = 0;
    corpus.num_lbl = 0;

    const List* curr = trees;
    while (curr) {
        const ParseNode* tree = (const ParseNode*)curr->head;
        extract_samples_recursive(tree, NULL, 0, 1, corpus.pos_samples, &corpus.num_pos, corpus.ext_samples, &corpus.num_ext, corpus.lbl_samples, &corpus.num_lbl);
        curr = curr->tail;
    }

    size_t max_questions = 128;
    corpus.questions = (Question*)spatter_alloc(max_questions * sizeof(Question));
    size_t q_idx = 0;

    corpus.questions[q_idx++] = (Question){ .type = Q_IS_FIRST_WORD, .int_param = 0, .str_param = str_view_create(NULL, 0) };
    corpus.questions[q_idx++] = (Question){ .type = Q_IS_LAST_WORD, .int_param = 0, .str_param = str_view_create(NULL, 0) };
    corpus.questions[q_idx++] = (Question){ .type = Q_IS_CAPITALIZED, .int_param = 0, .str_param = str_view_create(NULL, 0) };
    corpus.questions[q_idx++] = (Question){ .type = Q_SPAN_LEN_GREATER, .int_param = 1, .str_param = str_view_create(NULL, 0) };
    corpus.questions[q_idx++] = (Question){ .type = Q_SPAN_LEN_GREATER, .int_param = 2, .str_param = str_view_create(NULL, 0) };

    for (size_t t = 1; t < TAG_COUNT && q_idx < max_questions; t++) {
        corpus.questions[q_idx++] = (Question){ .type = Q_CURR_TAG, .int_param = (int)t, .str_param = str_view_create(NULL, 0) };
        corpus.questions[q_idx++] = (Question){ .type = Q_PREV_TAG, .int_param = (int)t, .str_param = str_view_create(NULL, 0) };
        corpus.questions[q_idx++] = (Question){ .type = Q_NEXT_TAG, .int_param = (int)t, .str_param = str_view_create(NULL, 0) };
    }

    for (size_t l = 1; l < LBL_COUNT && q_idx < max_questions; l++) {
        corpus.questions[q_idx++] = (Question){ .type = Q_LEFT_CHILD_LABEL, .int_param = (int)l, .str_param = str_view_create(NULL, 0) };
        corpus.questions[q_idx++] = (Question){ .type = Q_RIGHT_CHILD_LABEL, .int_param = (int)l, .str_param = str_view_create(NULL, 0) };
        corpus.questions[q_idx++] = (Question){ .type = Q_PREV_LABEL, .int_param = (int)l, .str_param = str_view_create(NULL, 0) };
    }

    for (size_t i = 0; i < corpus.num_pos && q_idx < max_questions; i++) {
        StrView w = corpus.pos_samples[i].context.curr_word;
        if (w.length > 0) {
            bool found = false;
            for (size_t k = 0; k < q_idx; k++) {
                if (corpus.questions[k].type == Q_CURR_WORD && str_view_equals(corpus.questions[k].str_param, w)) {
                    found = true;
                    break;
                }
            }
            if (!found && q_idx < max_questions) {
                corpus.questions[q_idx++] = (Question){ .type = Q_CURR_WORD, .int_param = 0, .str_param = w };
            }
        }
    }

    corpus.num_questions = q_idx;
    return corpus;
}

void treebank_corpus_free(TreebankCorpus corpus)
{
    spatter_free(corpus.pos_samples);
    spatter_free(corpus.ext_samples);
    spatter_free(corpus.lbl_samples);
    spatter_free(corpus.questions);
}
