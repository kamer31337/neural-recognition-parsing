#ifndef SPATTER_BEAM_SEARCH_H
#define SPATTER_BEAM_SEARCH_H

#include "spatter/common.h"
#include "spatter/list.h"
#include "spatter/tree.h"
#include "spatter/model.h"

typedef struct ParseState {
    const List* frontier;
    double accumulated_log_prob;
    size_t step_index;
    size_t sentence_len;
    bool is_complete;
} ParseState;

const ParseState* parse_state_create(const List* frontier, double log_prob, size_t step, size_t sentence_len, bool complete);
int parse_state_compare(const void* a, const void* b);
const List* beam_search_init(const StrView* words, size_t num_words, const SpatterModels* models, size_t k_tags);
const List* beam_search_step(const List* beam, const SpatterModels* models, size_t beam_width);
const ParseNode* beam_search_get_best_tree(const List* beam);
void parse_state_free(const ParseState* state);

#endif /* SPATTER_BEAM_SEARCH_H */
