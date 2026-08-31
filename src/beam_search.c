#include "spatter/beam_search.h"

const ParseState* parse_state_create(const List* frontier, double log_prob, size_t step, size_t sentence_len, bool complete)
{
    ParseState* state = (ParseState*)spatter_alloc(sizeof(ParseState));
    state->frontier = frontier;
    state->accumulated_log_prob = log_prob;
    state->step_index = step;
    state->sentence_len = sentence_len;
    state->is_complete = complete;
    return state;
}

int parse_state_compare(const void* a, const void* b)
{
    const ParseState* sa = *(const ParseState**)a;
    const ParseState* sb = *(const ParseState**)b;
    if (!sa && !sb) {
        return 0;
    }
    if (!sa) {
        return 1;
    }
    if (!sb) {
        return -1;
    }
    if (sa->is_complete && !sb->is_complete) {
        return -1;
    }
    if (!sa->is_complete && sb->is_complete) {
        return 1;
    }
    if (sa->accumulated_log_prob > sb->accumulated_log_prob) {
        return -1;
    }
    if (sa->accumulated_log_prob < sb->accumulated_log_prob) {
        return 1;
    }
    return 0;
}

const List* beam_search_init(const StrView* words, size_t num_words, const SpatterModels* models, size_t k_tags)
{
    SPATTER_UNUSED(k_tags);
    if (num_words == 0) {
        return list_empty();
    }
    double total_log_prob = 0.0;
    const List* frontier_rev = list_empty();
    Tag* chosen_tags = (Tag*)spatter_alloc(num_words * sizeof(Tag));

    for (size_t i = 0; i < num_words; i++) {
        Context ctx = context_from_tokens(words, chosen_tags, num_words, i);
        Distribution tag_dist = spatter_predict_tag(models, &ctx);
        Tag best_tag = TAG_NN;
        double best_lp = SPATTER_NEG_INF;
        if (tag_dist.probs != NULL) {
            for (size_t t = 1; t < tag_dist.num_classes; t++) {
                if (tag_dist.log_probs[t] > best_lp) {
                    best_lp = tag_dist.log_probs[t];
                    best_tag = (Tag)t;
                }
            }
        } else {
            best_lp = 0.0;
        }
        chosen_tags[i] = best_tag;
        total_log_prob += (best_lp > -20.0) ? best_lp : -20.0;
        const ParseNode* leaf = parse_node_create_leaf(words[i], best_tag, i, best_lp);
        frontier_rev = list_cons(leaf, frontier_rev);
    }

    spatter_free(chosen_tags);
    const List* frontier = list_reverse(frontier_rev);
    list_free_nodes(frontier_rev);

    const ParseState* init_state = parse_state_create(frontier, total_log_prob, 0, num_words, (num_words == 1));
    return list_cons(init_state, list_empty());
}

static const List* expand_single_node_state(const ParseState* state, const SpatterModels* models)
{
    const ParseNode* root_node = (const ParseNode*)list_head(state->frontier);
    if (!root_node) {
        return list_empty();
    }
    if (root_node->label == LBL_S || root_node->extension == EXT_ROOT) {
        const ParseState* completed = parse_state_create(state->frontier, state->accumulated_log_prob, state->step_index + 1, state->sentence_len, true);
        return list_cons(completed, list_empty());
    }
    Context ctx = context_from_frontier(state->frontier, 0, state->sentence_len);
    Distribution lbl_dist = spatter_predict_label(models, &ctx);
    Label best_lbl = LBL_S;
    double best_lp = 0.0;
    if (lbl_dist.probs != NULL) {
        for (size_t l = 1; l < lbl_dist.num_classes; l++) {
            if (lbl_dist.log_probs[l] > best_lp || best_lp == 0.0) {
                best_lp = lbl_dist.log_probs[l];
                best_lbl = (Label)l;
            }
        }
    }
    const List* child_list = list_cons(root_node, list_empty());
    const ParseNode* s_node = parse_node_create_nonterminal(best_lbl, EXT_ROOT, child_list, best_lp);
    const List* new_frontier = list_cons(s_node, list_empty());
    const ParseState* completed = parse_state_create(new_frontier, state->accumulated_log_prob + best_lp, state->step_index + 1, state->sentence_len, true);
    return list_cons(completed, list_empty());
}

static const List* expand_multi_node_state(const ParseState* state, const SpatterModels* models)
{
    size_t frontier_len = list_length(state->frontier);
    if (frontier_len <= 1) {
        return expand_single_node_state(state, models);
    }
    const List* candidates = list_empty();

    for (size_t i = 0; i < frontier_len; i++) {
        Context ctx = context_from_frontier(state->frontier, i, state->sentence_len);
        Distribution ext_dist = spatter_predict_extension(models, &ctx);
        Distribution lbl_dist = spatter_predict_label(models, &ctx);

        double unary_lp = (ext_dist.probs && EXT_UNARY < ext_dist.num_classes) ? ext_dist.log_probs[EXT_UNARY] : -2.0;
        Label best_lbl = LBL_NP;
        double best_lbl_lp = -1.0;
        if (lbl_dist.probs != NULL) {
            for (size_t l = 1; l < lbl_dist.num_classes; l++) {
                if (lbl_dist.log_probs[l] > best_lbl_lp || best_lbl_lp == -1.0) {
                    best_lbl_lp = lbl_dist.log_probs[l];
                    best_lbl = (Label)l;
                }
            }
        }
        const ParseNode* target_node = (const ParseNode*)list_nth(state->frontier, i);
        if (target_node && target_node->is_leaf) {
            const List* unary_child = list_cons(target_node, list_empty());
            const ParseNode* unary_parent = parse_node_create_nonterminal(best_lbl, EXT_NONE, unary_child, unary_lp + best_lbl_lp);

            const List* new_frontier_rev = list_empty();
            const List* curr = state->frontier;
            size_t idx = 0;
            while (curr) {
                if (idx == i) {
                    new_frontier_rev = list_cons(unary_parent, new_frontier_rev);
                } else {
                    new_frontier_rev = list_cons(curr->head, new_frontier_rev);
                }
                idx++;
                curr = curr->tail;
            }
            const List* new_frontier = list_reverse(new_frontier_rev);
            list_free_nodes(new_frontier_rev);

            double new_lp = state->accumulated_log_prob + unary_lp + best_lbl_lp;
            const ParseState* next_st = parse_state_create(new_frontier, new_lp, state->step_index + 1, state->sentence_len, false);
            candidates = list_cons(next_st, candidates);
        }

        if (i + 1 < frontier_len) {
            const ParseNode* left_child = (const ParseNode*)list_nth(state->frontier, i);
            const ParseNode* right_child = (const ParseNode*)list_nth(state->frontier, i + 1);
            if (left_child && right_child) {
                double reduce_ext_lp = -1.5;
                Label reduce_lbl = LBL_VP;
                if (left_child->label == LBL_NP && right_child->label == LBL_VP) {
                    reduce_lbl = LBL_S;
                } else if (left_child->tag == TAG_DT || left_child->tag == TAG_JJ) {
                    reduce_lbl = LBL_NP;
                } else if (left_child->tag == TAG_IN) {
                    reduce_lbl = LBL_PP;
                } else if (left_child->tag == TAG_VB || left_child->tag == TAG_VBD || left_child->tag == TAG_VBZ) {
                    reduce_lbl = LBL_VP;
                }
                const List* pair_children = list_cons(left_child, list_cons(right_child, list_empty()));
                const ParseNode* parent_node = parse_node_create_nonterminal(reduce_lbl, EXT_NONE, pair_children, reduce_ext_lp);

                const List* new_frontier_rev = list_empty();
                const List* curr = state->frontier;
                size_t idx = 0;
                while (curr) {
                    if (idx == i) {
                        new_frontier_rev = list_cons(parent_node, new_frontier_rev);
                        curr = curr->tail ? curr->tail->tail : NULL;
                        idx += 2;
                        continue;
                    }
                    new_frontier_rev = list_cons(curr->head, new_frontier_rev);
                    idx++;
                    curr = curr->tail;
                }
                const List* new_frontier = list_reverse(new_frontier_rev);
                list_free_nodes(new_frontier_rev);

                double new_lp = state->accumulated_log_prob + reduce_ext_lp;
                bool is_now_complete = (list_length(new_frontier) == 1 && reduce_lbl == LBL_S);
                const ParseState* next_st = parse_state_create(new_frontier, new_lp, state->step_index + 1, state->sentence_len, is_now_complete);
                candidates = list_cons(next_st, candidates);
            }
        }
    }
    return candidates;
}

const List* beam_search_step(const List* beam, const SpatterModels* models, size_t beam_width)
{
    if (!beam) {
        return list_empty();
    }
    const List* all_successors = list_empty();
    const List* curr_beam = beam;
    while (curr_beam) {
        const ParseState* state = (const ParseState*)curr_beam->head;
        if (state) {
            if (state->is_complete) {
                all_successors = list_cons(state, all_successors);
            } else {
                const List* succs = expand_multi_node_state(state, models);
                all_successors = list_append(succs, all_successors);
            }
        }
        curr_beam = curr_beam->tail;
    }

    if (list_is_empty(all_successors)) {
        return beam;
    }

    const List* sorted = list_sort(all_successors, parse_state_compare);
    const List* pruned = list_take(sorted, beam_width);
    list_free_nodes(sorted);
    return pruned;
}

const ParseNode* beam_search_get_best_tree(const List* beam)
{
    if (!beam) {
        return NULL;
    }
    const List* curr = beam;
    while (curr) {
        const ParseState* state = (const ParseState*)curr->head;
        if (state && state->is_complete && state->frontier) {
            return (const ParseNode*)list_head(state->frontier);
        }
        curr = curr->tail;
    }
    const ParseState* first = (const ParseState*)list_head(beam);
    if (first && first->frontier) {
        if (list_length(first->frontier) == 1) {
            return (const ParseNode*)list_head(first->frontier);
        }
        return parse_node_create_nonterminal(LBL_S, EXT_ROOT, first->frontier, first->accumulated_log_prob);
    }
    return NULL;
}

void parse_state_free(const ParseState* state)
{
    if (!state) {
        return;
    }
    list_free_nodes(state->frontier);
    spatter_free((void*)state);
}
