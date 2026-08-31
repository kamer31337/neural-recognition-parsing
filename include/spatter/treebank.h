#ifndef SPATTER_TREEBANK_H
#define SPATTER_TREEBANK_H

#include "spatter/common.h"
#include "spatter/result.h"
#include "spatter/tree.h"
#include "spatter/decision_tree.h"

typedef struct TreebankCorpus {
    TrainingSample* pos_samples;
    size_t num_pos;
    TrainingSample* ext_samples;
    size_t num_ext;
    TrainingSample* lbl_samples;
    size_t num_lbl;
    Question* questions;
    size_t num_questions;
} TreebankCorpus;

Result treebank_parse_sexpr(StrView sexpr_str);
const List* treebank_load_file(const char* filepath);
TreebankCorpus treebank_extract_corpus(const List* trees);
void treebank_corpus_free(TreebankCorpus corpus);

#endif /* SPATTER_TREEBANK_H */
