#ifndef SPATTER_TREE_H
#define SPATTER_TREE_H

#include "spatter/common.h"
#include "spatter/str_view.h"
#include "spatter/list.h"

typedef enum Tag {
    TAG_UNKNOWN = 0,
    TAG_DT,
    TAG_NN,
    TAG_NNS,
    TAG_NNP,
    TAG_NNPS,
    TAG_VB,
    TAG_VBD,
    TAG_VBG,
    TAG_VBN,
    TAG_VBP,
    TAG_VBZ,
    TAG_JJ,
    TAG_JJR,
    TAG_JJS,
    TAG_RB,
    TAG_RBR,
    TAG_RBS,
    TAG_IN,
    TAG_CC,
    TAG_PRP,
    TAG_PRP_DOLLAR,
    TAG_MD,
    TAG_TO,
    TAG_CD,
    TAG_POS,
    TAG_PUNCT,
    TAG_COUNT
} Tag;

typedef enum Extension {
    EXT_NONE = 0,
    EXT_RIGHT,
    EXT_LEFT,
    EXT_UP,
    EXT_UNARY,
    EXT_ROOT,
    EXT_COUNT
} Extension;

typedef enum Label {
    LBL_NONE = 0,
    LBL_S,
    LBL_NP,
    LBL_VP,
    LBL_PP,
    LBL_ADJP,
    LBL_ADVP,
    LBL_SBAR,
    LBL_PRT,
    LBL_QP,
    LBL_CONJP,
    LBL_COUNT
} Label;

typedef struct ParseNode {
    bool is_leaf;
    StrView word;
    Tag tag;
    Extension extension;
    Label label;
    size_t start_idx;
    size_t end_idx;
    double log_prob;
    const List* children;
} ParseNode;

const char* tag_to_str(Tag tag);
Tag tag_from_str(StrView sv);
const char* extension_to_str(Extension ext);
Extension extension_from_str(StrView sv);
const char* label_to_str(Label lbl);
Label label_from_str(StrView sv);

const ParseNode* parse_node_create_leaf(StrView word, Tag tag, size_t idx, double log_prob);
const ParseNode* parse_node_create_nonterminal(Label label, Extension ext, const List* children, double log_prob);
const ParseNode* parse_node_with_extension(const ParseNode* node, Extension ext, double log_prob_delta);
const ParseNode* parse_node_with_label(const ParseNode* node, Label label, double log_prob_delta);
void parse_node_to_bracketed(const ParseNode* node, FILE* stream);
void parse_node_print_tree(const ParseNode* node, int indent, FILE* stream);
void parse_node_free(const ParseNode* node);

#endif /* SPATTER_TREE_H */
