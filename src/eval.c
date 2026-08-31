#include "spatter/eval.h"

static void collect_brackets_recursive(const ParseNode* node, Bracket* brackets, size_t* count, size_t max_count)
{
    if (!node || node->is_leaf) {
        return;
    }
    if (node->label != LBL_NONE && node->end_idx > node->start_idx + 1) {
        if (*count < max_count) {
            Bracket b = { .label = node->label, .start_idx = node->start_idx, .end_idx = node->end_idx };
            brackets[(*count)++] = b;
        }
    }
    const List* curr = node->children;
    while (curr) {
        collect_brackets_recursive((const ParseNode*)curr->head, brackets, count, max_count);
        curr = curr->tail;
    }
}

ParsevalMetrics eval_compare_trees(const ParseNode* gold, const ParseNode* pred)
{
    ParsevalMetrics m = {
        .gold_brackets = 0,
        .pred_brackets = 0,
        .matched_brackets = 0,
        .precision = 0.0,
        .recall = 0.0,
        .f1 = 0.0,
        .exact_match = false
    };

    if (!gold || !pred) {
        return m;
    }

    Bracket gold_brackets[256];
    Bracket pred_brackets[256];
    size_t gold_count = 0;
    size_t pred_count = 0;

    collect_brackets_recursive(gold, gold_brackets, &gold_count, 256);
    collect_brackets_recursive(pred, pred_brackets, &pred_count, 256);

    m.gold_brackets = gold_count;
    m.pred_brackets = pred_count;

    bool pred_matched[256] = { false };
    for (size_t g = 0; g < gold_count; g++) {
        for (size_t p = 0; p < pred_count; p++) {
            if (!pred_matched[p] && gold_brackets[g].label == pred_brackets[p].label && gold_brackets[g].start_idx == pred_brackets[p].start_idx && gold_brackets[g].end_idx == pred_brackets[p].end_idx) {
                pred_matched[p] = true;
                m.matched_brackets++;
                break;
            }
        }
    }

    m.precision = (pred_count > 0) ? ((double)m.matched_brackets / (double)pred_count) : 1.0;
    m.recall = (gold_count > 0) ? ((double)m.matched_brackets / (double)gold_count) : 1.0;
    if (m.precision + m.recall > SPATTER_EPSILON) {
        m.f1 = (2.0 * m.precision * m.recall) / (m.precision + m.recall);
    } else {
        m.f1 = 0.0;
    }

    m.exact_match = (gold_count == pred_count && m.matched_brackets == gold_count);
    return m;
}

void eval_metrics_print(ParsevalMetrics m, FILE* stream)
{
    fprintf(stream, "PARSEVAL: Gold=%zu, Pred=%zu, Matched=%zu | Precision: %.2f%%, Recall: %.2f%%, F1: %.2f%% (Exact: %s)\n", m.gold_brackets, m.pred_brackets, m.matched_brackets, m.precision * 100.0, m.recall * 100.0, m.f1 * 100.0, m.exact_match ? "YES" : "NO");
}
