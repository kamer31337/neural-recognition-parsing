#ifndef SPATTER_EVAL_H
#define SPATTER_EVAL_H

#include "spatter/common.h"
#include "spatter/tree.h"

typedef struct Bracket {
    Label label;
    size_t start_idx;
    size_t end_idx;
} Bracket;

typedef struct ParsevalMetrics {
    size_t gold_brackets;
    size_t pred_brackets;
    size_t matched_brackets;
    double precision;
    double recall;
    double f1;
    bool exact_match;
} ParsevalMetrics;

ParsevalMetrics eval_compare_trees(const ParseNode* gold, const ParseNode* pred);
void eval_metrics_print(ParsevalMetrics m, FILE* stream);

#endif /* SPATTER_EVAL_H */
