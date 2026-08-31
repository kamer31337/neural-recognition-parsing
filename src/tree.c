#include "spatter/tree.h"

const char* tag_to_str(Tag tag)
{
    static const char* tag_names[] = {
        "UNK", "DT", "NN", "NNS", "NNP", "NNPS", "VB", "VBD", "VBG", "VBN", "VBP",
        "VBZ", "JJ", "JJR", "JJS", "RB", "RBR", "RBS", "IN", "CC", "PRP", "PRP$",
        "MD", "TO", "CD", "POS", "PUNCT"
    };
    if ((size_t)tag < sizeof(tag_names) / sizeof(tag_names[0])) {
        return tag_names[tag];
    }
    return "UNK";
}

Tag tag_from_str(StrView sv)
{
    for (size_t i = 1; i < TAG_COUNT; i++) {
        if (str_view_equals_cstr(sv, tag_to_str((Tag)i))) {
            return (Tag)i;
        }
    }
    if (str_view_equals_cstr(sv, ".") || str_view_equals_cstr(sv, ",") || str_view_equals_cstr(sv, "?") || str_view_equals_cstr(sv, "!")) {
        return TAG_PUNCT;
    }
    return TAG_UNKNOWN;
}

const char* extension_to_str(Extension ext)
{
    static const char* ext_names[] = { "NONE", "RIGHT", "LEFT", "UP", "UNARY", "ROOT" };
    if ((size_t)ext < sizeof(ext_names) / sizeof(ext_names[0])) {
        return ext_names[ext];
    }
    return "NONE";
}

Extension extension_from_str(StrView sv)
{
    for (size_t i = 1; i < EXT_COUNT; i++) {
        if (str_view_equals_cstr(sv, extension_to_str((Extension)i))) {
            return (Extension)i;
        }
    }
    return EXT_NONE;
}

const char* label_to_str(Label lbl)
{
    static const char* lbl_names[] = {
        "NONE", "S", "NP", "VP", "PP", "ADJP", "ADVP", "SBAR", "PRT", "QP", "CONJP"
    };
    if ((size_t)lbl < sizeof(lbl_names) / sizeof(lbl_names[0])) {
        return lbl_names[lbl];
    }
    return "NONE";
}

Label label_from_str(StrView sv)
{
    for (size_t i = 1; i < LBL_COUNT; i++) {
        if (str_view_equals_cstr(sv, label_to_str((Label)i))) {
            return (Label)i;
        }
    }
    return LBL_NONE;
}

const ParseNode* parse_node_create_leaf(StrView word, Tag tag, size_t idx, double log_prob)
{
    ParseNode* node = (ParseNode*)spatter_alloc(sizeof(ParseNode));
    node->is_leaf = true;
    node->word = word;
    node->tag = tag;
    node->extension = EXT_NONE;
    node->label = LBL_NONE;
    node->start_idx = idx;
    node->end_idx = idx + 1;
    node->log_prob = log_prob;
    node->children = list_empty();
    return node;
}

const ParseNode* parse_node_create_nonterminal(Label label, Extension ext, const List* children, double log_prob)
{
    ParseNode* node = (ParseNode*)spatter_alloc(sizeof(ParseNode));
    node->is_leaf = false;
    node->word = str_view_create(NULL, 0);
    node->tag = TAG_UNKNOWN;
    node->extension = ext;
    node->label = label;
    node->log_prob = log_prob;
    node->children = children;
    size_t min_idx = (size_t)-1;
    size_t max_idx = 0;
    const List* curr = children;
    while (curr) {
        const ParseNode* child = (const ParseNode*)curr->head;
        if (child) {
            if (child->start_idx < min_idx) {
                min_idx = child->start_idx;
            }
            if (child->end_idx > max_idx) {
                max_idx = child->end_idx;
            }
        }
        curr = curr->tail;
    }
    node->start_idx = (min_idx == (size_t)-1) ? 0 : min_idx;
    node->end_idx = max_idx;
    return node;
}

const ParseNode* parse_node_with_extension(const ParseNode* node, Extension ext, double log_prob_delta)
{
    ParseNode* copy = (ParseNode*)spatter_alloc(sizeof(ParseNode));
    *copy = *node;
    copy->extension = ext;
    copy->log_prob += log_prob_delta;
    return copy;
}

const ParseNode* parse_node_with_label(const ParseNode* node, Label label, double log_prob_delta)
{
    ParseNode* copy = (ParseNode*)spatter_alloc(sizeof(ParseNode));
    *copy = *node;
    copy->label = label;
    copy->log_prob += log_prob_delta;
    return copy;
}

void parse_node_to_bracketed(const ParseNode* node, FILE* stream)
{
    if (!node) {
        return;
    }
    if (node->is_leaf) {
        fprintf(stream, "(%s " SV_FMT ")", tag_to_str(node->tag), SV_ARG(node->word));
        return;
    }
    const char* lbl = (node->label != LBL_NONE) ? label_to_str(node->label) : "X";
    fprintf(stream, "(%s", lbl);
    const List* curr = node->children;
    while (curr) {
        fprintf(stream, " ");
        parse_node_to_bracketed((const ParseNode*)curr->head, stream);
        curr = curr->tail;
    }
    fprintf(stream, ")");
}

void parse_node_print_tree(const ParseNode* node, int indent, FILE* stream)
{
    if (!node) {
        return;
    }
    for (int i = 0; i < indent; i++) {
        fprintf(stream, "  ");
    }
    if (node->is_leaf) {
        fprintf(stream, "-- [%s] " SV_FMT " (ext: %s, lp: %.3f)\n", tag_to_str(node->tag), SV_ARG(node->word), extension_to_str(node->extension), node->log_prob);
        return;
    }
    fprintf(stream, "+- [%s] (ext: %s, span: [%zu,%zu), lp: %.3f)\n", label_to_str(node->label), extension_to_str(node->extension), node->start_idx, node->end_idx, node->log_prob);
    const List* curr = node->children;
    while (curr) {
        parse_node_print_tree((const ParseNode*)curr->head, indent + 1, stream);
        curr = curr->tail;
    }
}

void parse_node_free(const ParseNode* node)
{
    if (!node) {
        return;
    }
    const List* curr = node->children;
    while (curr) {
        parse_node_free((const ParseNode*)curr->head);
        curr = curr->tail;
    }
    list_free_nodes(node->children);
    spatter_free((void*)node);
}
